/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

LONG InternalOpen(CONST_STRPTR name, LONG accessMode, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase);

#define MAX_SOFT_LINK_NESTING 16 /* Maximum level of soft links nesting */

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BPTR, Open,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,       D1),
	AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 5, Dos)

/*  FUNCTION
	Opens a file for read and/or write depending on the accessmode given.

    INPUTS
	name	   - NUL terminated name of the file.
	accessMode - One of MODE_OLDFILE   - open existing file
			    MODE_NEWFILE   - delete old, create new file
					     exclusive lock
			    MODE_READWRITE - open new one if it doesn't exist

    RESULT
	Handle to the file or 0 if the file couldn't be opened.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *ret;
    LONG error;

    /* Sanity check */
    if (name == NULL) return BNULL;

    /* Create filehandle */
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);

    if(ret != NULL)
    {
	LONG ok = InternalOpen(name, accessMode, ret, MAX_SOFT_LINK_NESTING, DOSBase);
	D(bug("[Open] = %d Error = %d\n", ok, IoErr()));
	if (ok)
	{
	    return MKBADDR(ret);	    
	}
	else
	{
	    error = IoErr();
	    FreeDosObject(DOS_FILEHANDLE,ret);
	}
    }
    else
	error = ERROR_NO_FREE_STORE;

    SetIoErr(error);
    return BNULL;

    AROS_LIBFUNC_EXIT
} /* Open */

/* Try to open name recursively calling itself in case it's a soft link.
   Store result in handle. Return boolean value indicating result. */
LONG InternalOpen(CONST_STRPTR name, LONG accessMode, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret = DOSFALSE;
    LONG error = 0;
    LONG error2 = 0;
    BPTR con, ast;

    D(bug("[Open] Process: 0x%p \"%s\", Window: 0x%p, Name: \"%s\" FH: 0x%p\n", me, me->pr_Task.tc_Node.ln_Name, me->pr_WindowPtr, name, handle));

    if(soft_nesting == 0)
    {
	SetIoErr(ERROR_TOO_MANY_LEVELS);
	return DOSFALSE;
    }

    switch(accessMode)
    {
	case MODE_NEWFILE:
	case MODE_READWRITE:
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	case MODE_OLDFILE:
	    ast = con = me->pr_CIS;
	    break;

	default:
	    if (accessMode & (FMF_CREATE|FMF_CLEAR))
	    {
	    	con = me->pr_COS;
	    	ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    }
	    else
	    	ast = con = me->pr_CIS;
	    break;
    }

    if (!Stricmp(name, "CONSOLE:"))
    	error = fs_Open(handle, REF_CONSOLE, con, accessMode, name, DOSBase);
    else if (!Stricmp(name, "*"))
    	error = fs_Open(handle, REF_CONSOLE, ast, accessMode, name, DOSBase);
#ifdef AROS_DOS_PACKETS
    /* Special case for NIL:, unsupported by IOFS */
    else if (!Stricmp(name, "NIL:"))
    {
    	SetIoErr(0);

    	handle->fh_Type = BNULL;
    	/* NIL: is considered interactive */
    	handle->fh_Port = (struct MsgPort*)DOSTRUE;
        return DOSTRUE;
    }
#endif
    else
    {
    	STRPTR filename = strchr(name, ':');

	if (!filename)
	{
	    /* No ':', pathname relative to current dir */
            BPTR cur = me->pr_CurrentDir;

            if (!cur)
            	cur = DOSBase->dl_SYSLock;

	    if (cur)
	    	error = fs_Open(handle, REF_LOCK, cur, accessMode, name, DOSBase);
	    else
	    	/*
	    	 * This can be reached if we attempt to load disk-based library or
	    	 * device before dl_SYSLock is assigned. This can happen, for example,
	    	 * when attempting to mount a handler at boottime which is missing
	    	 * from the kickstart.
	    	 */
	    	error = ERROR_OBJECT_NOT_FOUND;
	}
    	else 
    	{
            struct DevProc *dvp = NULL;

	    filename++;
	    do
	    {
            	if ((dvp = GetDeviceProc(name, dvp)) == NULL)
            	{
                    error = IoErr();
                    break;
		}

	    	error = fs_Open(handle, REF_DEVICE, MKBADDR(dvp), accessMode, filename, DOSBase);
            } while(error == ERROR_OBJECT_NOT_FOUND && accessMode != MODE_NEWFILE);

	    if (error == ERROR_NO_MORE_ENTRIES)
        	error = ERROR_OBJECT_NOT_FOUND;

	    if (error == ERROR_IS_SOFT_LINK)
            {
            	ULONG buffer_size = 256;
            	STRPTR softname;
            	LONG continue_loop;
            	LONG written;

            	do
            	{
                    continue_loop = FALSE;
                    if (!(softname = AllocVec(buffer_size, MEMF_ANY)))
                    {
                    	error2 = ERROR_NO_FREE_STORE;
                    	break;
                    }

                    written = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, name, softname, buffer_size);
                    if (written == -1)
                    {
                    	/* An error occured */
                    	error2 = IoErr();
                    }
                    else if (written == -2)
                    {
                    	/* If there's not enough space in the buffer, increase
                       	   it and try again */
                    	continue_loop = TRUE;
                    	buffer_size *= 2;
                    }
                    else if (written >= 0)
                    {
                    	/* All OK */
                    	BPTR olddir;

                    	olddir = CurrentDir(dvp->dvp_Lock);
                    	ret = InternalOpen(softname, accessMode, handle, soft_nesting - 1, DOSBase);
                    	error2 = IoErr();
                    	CurrentDir(olddir);
                    }
                    else
                    	error2 = ERROR_UNKNOWN;

                    FreeVec(softname);
	        }
            	while(continue_loop);
            }
            FreeDeviceProc(dvp);
	}
    }

    if(!error)
    {
        if (IsInteractive(MKBADDR(handle)))
            SetVBuf(MKBADDR(handle), NULL, BUF_LINE, -1);
        
        return DOSTRUE;
    }
    else if(error == ERROR_IS_SOFT_LINK)
    {
	if(!ret)
	    SetIoErr(error2);
	else
	    SetIoErr(0);
	return ret;
    }
    else
    {
	SetIoErr(error);
	return DOSFALSE;
    }
}
