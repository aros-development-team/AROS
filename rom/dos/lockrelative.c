/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    Desc: Locks a file or directory.
*/

#define DLINK(x)

#include <aros/debug.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "dos_intern.h"
#include "fs_driver.h"

static LONG InternalLockRelative(
    BPTR baselock,
    CONST_STRPTR name,
    LONG accessMode,
    BPTR *retlock,
    LONG soft_nesting,
    struct DosLibrary *DOSBase);

#define MAX_SOFT_LINK_NESTING 16 /* Maximum level of soft links nesting */

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BPTR, LockRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,       D1),
        AROS_LHA(CONST_STRPTR, name,       D2),
        AROS_LHA(LONG,         accessMode, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 227, Dos)

/*  FUNCTION
        Gets a lock on a file or directory relative to another lock. If name is
        absolute, lock is ignored. If name is relative and lock is NULL, the
        current directory is used. Otherwise, the name is computed relative to
        the given lock.

        There may be more than one shared lock on a file but only one if it is
        an exclusive one  Locked files or directories may not be deleted.

    INPUTS
        lock       - existing lock to compute filename from
        name       - NUL terminated name of the file or directory.
        accessMode - One of SHARED_LOCK
                            EXCLUSIVE_LOCK

    RESULT
        Handle to the file or directory or 0 if the object couldn't be locked.
        IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    BPTR retlock;

    /* Sanity check */
    if (name == NULL)
        return BNULL;

    ASSERT_VALID_PTR(name);

    D(bug("[LockRelative] baselock=0x%p name=%s mode=%d\n", lock, name, accessMode));

    if (InternalLockRelative(lock, name, accessMode, &retlock, MAX_SOFT_LINK_NESTING, DOSBase))
    {
        D(bug("[LockRelative] returned 0x%p\n", retlock));
        return retlock;
    }

    D(bug("[LockRelative] failed, err=%d\n", IoErr()));
    return BNULL;

    AROS_LIBFUNC_EXIT
} /* LockRelative */


/* Attempt to create a synthetic IN:, OUT:, ERR:,
 * STDIN:, STDOUT, or STDERR: lock
 */
BOOL pseudoLock(CONST_STRPTR name, LONG lockMode, BPTR *retlock, LONG *ret, struct DosLibrary *DOSBase)
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

    *retlock = DupLockFromFH(fh);
    if (*retlock) {
        struct FileLock *fl = BADDR(*retlock);
        fl->fl_Access = lockMode;
    }
    *ret = (*retlock != BNULL) ? DOSTRUE : DOSFALSE;
    return TRUE;
}


/* Try to lock name recursively calling itself in case it's a soft link.
   Store result in handle. Return boolean value indicating result. */
static LONG InternalLockRelative(
    BPTR baselock,
    CONST_STRPTR name,
    LONG accessMode,
    BPTR *retlock,
    LONG soft_nesting,
    struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR lock = BNULL;
    struct DevProc *dvp = NULL;
    LONG ret = DOSFALSE;
    LONG error = 0;
    STRPTR filename;

    ASSERT_VALID_PROCESS(me);
    D(bug("[LockRelative] Process: 0x%p \"%s\", Window: 0x%p, Name: \"%s\", \n", me, me->pr_Task.tc_Node.ln_Name, me->pr_WindowPtr, name));

    if(soft_nesting == 0)
    {
        SetIoErr(ERROR_TOO_MANY_LEVELS);
        return DOSFALSE;
    }

    /* Check for a pseudo-file lock
     * (ie IN:, STDOUT:, ERR:, etc)
     */
    if (pseudoLock(name, accessMode, retlock, &ret, DOSBase))
        return ret;

    filename = strchr(name, ':');
    if (!filename)
    {
        /* No ':' in the pathname, path is relative */
        struct MsgPort *port;

        if (baselock) {
            /* baselock provided; relative to it */
            port = ((struct FileLock *)BADDR(baselock))->fl_Task;
            lock = baselock;
        }
        else {
            /* no lock provided; relative to current directory */
            lock = me->pr_CurrentDir;
            if (lock && lock != (BPTR)-1) {
                port = ((struct FileLock *)BADDR(lock))->fl_Task;
            }
            else {
                /* no current directory? go from the root */
                port = DOSBase->dl_Root->rn_BootProc;
                lock = BNULL;
            }
        }

        error = fs_LocateObject(retlock, port, lock, name, accessMode, DOSBase);
    }
    else
    {
        /* absolute path; get the device and look it up against that */
        filename++;
        do
        {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL)
            {
                error = IoErr();
                break;
            }

            error = fs_LocateObject(retlock, dvp->dvp_Port, dvp->dvp_Lock, filename, accessMode, DOSBase);

        } while (error == ERROR_OBJECT_NOT_FOUND);

        if (error == ERROR_NO_MORE_ENTRIES)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

#ifndef __mc68000
        /* FIXME: On Linux hosted we sometimes get ERROR_IS_SOFTLINK with dvp == NULL,
         * which causes segfaults below if we don't change "error". Adding !dvp below
         * is probably a hack.
         *
         * This is wrong, GetDeviceProc() can return other errors than ERROR_OBJECT_NOT_FOUND.
         */
        if (!dvp)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;
#endif
    }

    if (error == ERROR_IS_SOFT_LINK)
    {
        STRPTR softname = ResolveSoftlink(lock, dvp, name, DOSBase);

        if (softname)
        {
            if (dvp)
            {
                /*
                 * ResolveSoftlink() gives us path relative to either 'lock' lock
                 * (if on current volume), or 'dvp' volume root (if on different volume).
                 * In the latter case we need to do the lookup against the volume root lock.
                 */
                lock = dvp->dvp_Lock;
            }

            ret = InternalLockRelative(lock, softname, accessMode, retlock, soft_nesting - 1, DOSBase);
            if (ret) {
                error = 0;
            }
            else {
                error = IoErr();
                D(bug("[LockRelative] Resolve error %d\n", error));
            }

            FreeVec(softname);
        }
        else {
            error = IoErr();
        }
    }

    FreeDeviceProc(dvp);

    if (error)
    {
        SetIoErr(error);
        return DOSFALSE;
    }

    return DOSTRUE;
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
