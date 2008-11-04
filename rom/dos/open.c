/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/
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
    if (name == NULL) return NULL;

    /* Create filehandle */
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);

    if(ret != NULL)
    {
	if(InternalOpen(name, accessMode, ret, MAX_SOFT_LINK_NESTING, DOSBase))
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
    return NULL;

    AROS_LIBFUNC_EXIT
} /* Open */

/* Try to open name recursively calling itself in case it's a soft link.
   Store result in handle. Return boolean value indicating result. */
LONG InternalOpen(CONST_STRPTR name, LONG accessMode, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret = DOSFALSE;
    LONG error = 0;
    LONG error2 = 0;
    LONG doappend = 0;
    BPTR con, ast;

    if(soft_nesting == 0)
    {
	SetIoErr(ERROR_TOO_MANY_LEVELS);
	return DOSFALSE;
    }

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_OPEN_FILE, DOSBase);

    switch(accessMode)
    {
	case MODE_OLDFILE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_OLDFILE;
	    ast = con = me->pr_CIS;
	    break;

	case MODE_NEWFILE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_NEWFILE;
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	case MODE_READWRITE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_READWRITE;
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	default:
	    /* See if the user requested append mode */
	    doappend   = accessMode & FMF_APPEND;

	    /* The append mode is all taken care by dos.library */
	    accessMode &= ~FMF_APPEND;

	    iofs.io_Union.io_OPEN_FILE.io_FileMode = accessMode;
	    ast = con = me->pr_CIS;
	    break;
    }

    iofs.io_Union.io_OPEN_FILE.io_Protection = 0;

    /* check for an empty filename. this supports this form that was
     * required pre-2.0, which didn't have OpenFromLock():
     *
     *     old = CurrentDir(lock);
     *     fh = Open("", MODE_OLDFILE);
     *     CurrentDir(old);
     */
    if (*name == '\0') {
        BPTR cur;
        struct FileHandle *fh;

        cur = me->pr_CurrentDir;
        if (!cur)
            cur = DOSBase->dl_SYSLock;

        if (cur) {
            fh = BADDR(cur);

            iofs.io_Union.io_OPEN_FILE.io_Filename = (STRPTR) "";

            iofs.IOFS.io_Device = fh->fh_Device;
            iofs.IOFS.io_Unit = fh->fh_Unit;

            DosDoIO(&iofs.IOFS);

            error = me->pr_Result2 = iofs.io_DosError;
        } else {
            error = ERROR_OBJECT_NOT_FOUND;
            SetIoErr(error);
        }
    }
    else if(!Stricmp(name, (STRPTR) "CONSOLE:"))
    {
	iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(con))->fh_Device;
	iofs.IOFS.io_Unit   = ((struct FileHandle *)BADDR(con))->fh_Unit;
	iofs.io_Union.io_OPEN_FILE.io_Filename = (STRPTR) "";
	DosDoIO(&iofs.IOFS);
	error = me->pr_Result2 = iofs.io_DosError;
    }
    else if(!Stricmp(name, (STRPTR) "*"))
    {
	iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(ast))->fh_Device;
	iofs.IOFS.io_Unit   = ((struct FileHandle *)BADDR(ast))->fh_Unit;
	iofs.io_Union.io_OPEN_FILE.io_Filename = (STRPTR) "";
	DosDoIO(&iofs.IOFS);
	error = me->pr_Result2 = iofs.io_DosError;
    }
    else 
    {
        struct DevProc *dvp = NULL;

        iofs.io_Union.io_OPEN_FILE.io_Filename = StripVolume(name);

        do {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL) {
                error = IoErr();
                break;
            }

            error = DoIOFS(&iofs, dvp, NULL, DOSBase);
        } while(error == ERROR_OBJECT_NOT_FOUND && accessMode != MODE_NEWFILE);

        if (error == ERROR_NO_MORE_ENTRIES)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

        if(error == ERROR_IS_SOFT_LINK)
        {
            ULONG buffer_size = 256;
            STRPTR softname;
            LONG continue_loop;
            LONG written;

            do
            {
                continue_loop = FALSE;
                if(!(softname = AllocVec(buffer_size, MEMF_ANY)))
                {
                    error2 = ERROR_NO_FREE_STORE;
                    break;
                }

                written = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, name, softname, buffer_size);
                if(written == -1)
                {
                    /* An error occured */
                    error2 = IoErr();
                }
                else if(written == -2)
                {
                    /* If there's not enough space in the buffer, increase
                       it and try again */
                    continue_loop = TRUE;
                    buffer_size *= 2;
                }
                else if(written >= 0)
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
    
    if(!error)
    {
        handle->fh_Device = iofs.IOFS.io_Device;
        handle->fh_Unit   = iofs.IOFS.io_Unit;
        if (doappend)
        {
            /* See if the handler supports FSA_SEEK */
            if (Seek(MKBADDR(handle), 0, OFFSET_END) != -1)
            {
        	/* if so then set the proper flag in the FileHandle struct */
        	handle->fh_Flags |= FHF_APPEND;
            }
        }
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
