/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Locks a file or directory.
    Lang: English
*/

#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "dos_intern.h"
#include "fs_driver.h"

#define  DEBUG  0
#include <aros/debug.h>

LONG InternalLock(CONST_STRPTR name, LONG accessMode, 
    BPTR *handle, LONG soft_nesting, struct DosLibrary *DOSBase);

#define MAX_SOFT_LINK_NESTING 16 /* Maximum level of soft links nesting */

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BPTR, Lock,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,       D1),
	AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 14, Dos)

/*  FUNCTION
	Gets a lock on a file or directory. There may be more than one
	shared lock on a file but only one if it is an exclusive one.
	Locked files or directories may not be deleted.

    INPUTS
	name	   - NUL terminated name of the file or directory.
	accessMode - One of SHARED_LOCK
			    EXCLUSIVE_LOCK

    RESULT
	Handle to the file or directory or 0 if the object couldn't be locked.
	IoErr() gives additional information in that case.

    NOTES
	The lock structure returned by this function is different
	from that of AmigaOS (in fact it is identical to a filehandle).
	Do not try to read any internal fields.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    BPTR fl;

    /* Sanity check */
    if (name == NULL)
        return BNULL;
    
    ASSERT_VALID_PTR(name);

    D(bug("[Lock] '%s':%d\n", name, accessMode));

    if (InternalLock(name, accessMode, &fl, MAX_SOFT_LINK_NESTING, DOSBase))
    {
    	D(bug("[Lock] returned 0x%p\n", fl));
        return fl;
    }

    D(bug("[Lock] failed, err=%d\n", IoErr()));
    return BNULL;

    AROS_LIBFUNC_EXIT
} /* Lock */

/* Try to lock name recursively calling itself in case it's a soft link. 
   Store result in handle. Return boolean value indicating result. */
LONG InternalLock(CONST_STRPTR name, LONG accessMode, 
    BPTR *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DevProc *dvp;
    LONG ret = DOSFALSE;
    LONG error = 0;
    LONG error2 = 0;
    STRPTR filename;

    D(bug("[Lock] Process: 0x%p \"%s\", Window: 0x%p, Name: \"%s\", \n", me, me->pr_Task.tc_Node.ln_Name, me->pr_WindowPtr, name));

    if(soft_nesting == 0)
    {
	SetIoErr(ERROR_TOO_MANY_LEVELS);
	return DOSFALSE;
    }

    filename = strchr(name, ':');
    if (!filename)
    {
	/* No ':' in the pathname, path is relative to current directory */
	BPTR cur;

	cur = me->pr_CurrentDir;
	if (!cur)
	    cur = DOSBase->dl_SYSLock;

        if (cur && (cur != (BPTR)-1))
            error = fs_LocateObject(handle, cur, NULL, name, accessMode, DOSBase);
        else 
            error = ERROR_OBJECT_NOT_FOUND;

        SetIoErr(error);
    }
    else 
    {
    	filename++;
        dvp = NULL;
        do
        {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL) 
            {
                error = IoErr();
                break;
            }

	    error = fs_LocateObject(handle, BNULL, dvp, filename, accessMode, DOSBase);

        } while (error == ERROR_OBJECT_NOT_FOUND);

	/* FIXME: On Linux hosted we sometimes get ERROR_IS_SOFTLINK with dvp == NULL,
	 * which causes segfaults below if we don't change "error". Adding !dvp below
         * is probably a hack
         */
        if (error == ERROR_NO_MORE_ENTRIES || !dvp)
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
                if(!(softname = AllocVec(buffer_size, MEMF_PUBLIC)))
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
                    ret = InternalLock(softname, accessMode, handle, soft_nesting - 1, DOSBase);
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
	return DOSTRUE;
    else if (error == ERROR_IS_SOFT_LINK)
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
