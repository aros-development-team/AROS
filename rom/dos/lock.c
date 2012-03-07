/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Locks a file or directory.
    Lang: English
*/

#define DLINK(x)

#include <aros/debug.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "dos_intern.h"
#include "fs_driver.h"

static LONG InternalLock(CONST_STRPTR name, LONG accessMode, 
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


/* Attempt to create a synthetic IN:, OUT:, ERR:,
 * STDIN:, STDOUT, or STDERR: lock
 */
BOOL pseudoLock(CONST_STRPTR name, LONG lockMode, BPTR *lock, LONG *ret, struct DosLibrary *DOSBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR fh = (BPTR)-1;

    ASSERT_VALID_PROCESS(me);

    /* IN:, STDIN: */
    if (!Stricmp(name, "IN:") || !Stricmp(name, "STDIN:")) {
        if (lockMode != ACCESS_READ) {
            SetIoErr(ERROR_OBJECT_IN_USE);
            *ret = DOSFALSE;
            return TRUE;
        }

        fh = me->pr_CIS;
    }

    /* OUT:, STDOUT: */
    if (!Stricmp(name, "OUT:") || !Stricmp(name, "STDOUT:")) {
        if (lockMode != ACCESS_WRITE) {
            SetIoErr(ERROR_OBJECT_IN_USE);
            *ret = DOSFALSE;
            return TRUE;
        }

        fh = me->pr_COS;
    }


    /* ERR:, STDERR: */
    if (!Stricmp(name, "ERR:") || !Stricmp(name, "STDERR:")) {
        if (lockMode != ACCESS_WRITE) {
            SetIoErr(ERROR_OBJECT_IN_USE);
            *ret = DOSFALSE;
            return TRUE;
        }

        fh = me->pr_CES;
    }

    if (fh == (BPTR)-1)
        return FALSE;

    if (fh == BNULL) {
        SetIoErr(ERROR_OBJECT_NOT_FOUND);
        *ret = DOSFALSE;
        return TRUE;
    }

    *lock = DupLockFromFH(fh);
    if (*lock) {
        struct FileLock *fl = BADDR(*lock);
        fl->fl_Access = lockMode;
    }
    *ret = (*lock != BNULL) ? DOSTRUE : DOSFALSE;
    return TRUE;
}


/* Try to lock name recursively calling itself in case it's a soft link. 
   Store result in handle. Return boolean value indicating result. */
static LONG InternalLock(CONST_STRPTR name, LONG accessMode, 
    BPTR *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR cur = BNULL;
    struct DevProc *dvp = NULL;
    LONG ret = DOSFALSE;
    LONG error = 0;
    STRPTR filename;

    ASSERT_VALID_PROCESS(me);
    D(bug("[Lock] Process: 0x%p \"%s\", Window: 0x%p, Name: \"%s\", \n", me, me->pr_Task.tc_Node.ln_Name, me->pr_WindowPtr, name));

    if(soft_nesting == 0)
    {
	SetIoErr(ERROR_TOO_MANY_LEVELS);
	return DOSFALSE;
    }

    /* Check for a pseudo-file lock
     * (ie IN:, STDOUT:, ERR:, etc)
     */
    if (pseudoLock(name, accessMode, handle, &ret, DOSBase))
        return ret;

    filename = strchr(name, ':');
    if (!filename)
    {
        struct MsgPort *port;
        BPTR lock;

	/* No ':' in the pathname, path is relative to current directory */
	cur = me->pr_CurrentDir;
	if (cur && cur != (BPTR)-1) {
	    port = ((struct FileLock *)BADDR(cur))->fl_Task;
	    lock = cur;
        } else {
            port = DOSBase->dl_Root->rn_BootProc;
            lock = BNULL;
        }

        error = fs_LocateObject(handle, port, lock, name, accessMode, DOSBase);
        SetIoErr(error);
    }
    else 
    {
    	filename++;
        do
        {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL) 
            {
                error = IoErr();
                break;
            }

	    error = fs_LocateObject(handle, dvp->dvp_Port, dvp->dvp_Lock, filename, accessMode, DOSBase);

        } while (error == ERROR_OBJECT_NOT_FOUND);

	/* FIXME: On Linux hosted we sometimes get ERROR_IS_SOFTLINK with dvp == NULL,
	 * which causes segfaults below if we don't change "error". Adding !dvp below
         * is probably a hack
         */
        if (error == ERROR_NO_MORE_ENTRIES || !dvp)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;
    }

    if (error == ERROR_IS_SOFT_LINK)
    {
        STRPTR softname = ResolveSoftlink(cur, dvp, name, DOSBase);

        if (softname)
        {
            BPTR olddir = BNULL;

            /*
             * ResolveSoftLink() gives us path relative to either 'cur' lock
             * (if on current volume), or 'dvp' volume root (if on different volume).
             * In the latter case we need to change current directory to volume's root
             * in order to follow the link correctly.
             */
            if (dvp)
            {
                olddir = me->pr_CurrentDir;
            	error = RootDir(dvp, DOSBase);
            }
            else
            	error = 0;

            if (!error)
            {
            	ret = InternalLock(softname, accessMode, handle, soft_nesting - 1, DOSBase);
            	error = ret ? 0 : IoErr();
            	D(bug("[Lock] Resolve error %d\n", error));

		if (olddir)
            	    UnLock(CurrentDir(olddir));
            }

	    FreeVec(softname);
        }
        else
            error = IoErr();
    }

    FreeDeviceProc(dvp);

    if (error)
    {
    	SetIoErr(error);
    	ret = DOSFALSE;
    }
    else
    	ret = DOSTRUE;

    return ret;
}

/*
 * Resolve a softlink.
 * Returns AllocVec()ed buffer with softlink contents.
 */
STRPTR ResolveSoftlink(BPTR cur, struct DevProc *dvp, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    ULONG buffer_size = 256;
    STRPTR softname;
    LONG continue_loop;
    LONG written;

    DLINK(bug("[Softlink] Resolving softlink %s...\n", name));

    do
    {
        continue_loop = FALSE;

        if (!(softname = AllocVec(buffer_size, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            break;
        }

        written = fs_ReadLink(cur, dvp, name, softname, buffer_size, DOSBase);

	switch (written)
	{
	case -1:
            /* An error occured */
            DLINK(bug("[Softlink] Error %d reading softlink\n", IoErr()));
            break;

        case -2:
            /* If there's not enough space in the buffer, increase it and try again */
            continue_loop = TRUE;
            buffer_size <<= 1;

            DLINK(bug("[Softlink] Increased buffer size up to %u\n", buffer_size));
            break;

	default:
            /* All OK */
            DLINK(bug("[Softlink] Resolved path: %s\n", softname));
            return softname;
        }
                
        FreeVec(softname);
    }
    while(continue_loop);

    return NULL;
}

/* Change to root directory of the specified device */
LONG RootDir(struct DevProc *dvp, struct DosLibrary *DOSBase)
{
    BPTR lock = BNULL;
    LONG error;

    /* We already have a DeviceProc structure, so just use internal routine. */
    error = fs_LocateObject(&lock, dvp->dvp_Port, dvp->dvp_Lock, "", SHARED_LOCK, DOSBase);

    if (!error)
    	CurrentDir(lock);

    return error;
}
