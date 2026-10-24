/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetDeviceProcRelative() - Find the filesystem for a path
          relative to a directory lock.
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(struct DevProc *, GetDeviceProcRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,             lock, D1),
        AROS_LHA(CONST_STRPTR,     name, D2),
        AROS_LHA(struct DevProc *, dp,   D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 7, DosRelative)

/*  FUNCTION
        GetDeviceProcRelative() will find the filesystem handler which
        you should send a command to for a specific path, and the lock
        the path is relative to. For a relative name the result refers
        to the handler owning the given lock, with the lock itself as
        dvp_Lock; a packet built from it names the object exactly as
        LockRelative() would.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/GetDeviceProc(), including the
        iteration over the targets of a multi-directory assign.

        The first call should have the |dp| parameter as NULL. Passing
        a previous result continues an iteration; a result obtained for
        a name relative to a lock has no further entries.

    INPUTS
        lock - directory lock to resolve a relative name against,
               or NULL for the current directory
        name - Name of the object to find.
        dp   - Previous result of GetDeviceProcRelative() or NULL.

    RESULT
        A pointer to a DevProc structure containing the information
        required to send a command to a filesystem, or NULL with IoErr()
        set. Release it with dos.library/FreeDeviceProc().

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/GetDeviceProc(), dos.library/FreeDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileLock *fl;

    D(bug("[GetDeviceProcRelative] lock=0x%p '%s' dp=0x%p\n", BADDR(lock), name, dp));

    /*
     * Continuing an iteration. A DevProc made here is never a
     * multi-directory assign, so it has no further entries; one made by
     * GetDeviceProc() for an absolute name is handed back to it. The
     * test is the one GetDeviceProc() applies, done here as well so a
     * DevProc without a DosList node (ours, when the lock's handler has
     * no volume node) is not passed in to be dereferenced.
     */
    if (dp != NULL)
    {
        if (dp->dvp_DevNode == NULL ||
            dp->dvp_DevNode->dol_Type != DLT_DIRECTORY ||
            !(dp->dvp_Flags & DVPF_ASSIGN))
        {
            FreeDeviceProc(dp);
            SetIoErr(ERROR_NO_MORE_ENTRIES);
            return NULL;
        }

        return GetDeviceProc(name, dp);
    }

    /* Nothing to resolve relative to: dos.library already does this */
    if (lock == BNULL || name == NULL || dosrel_IsAbsolute(name))
        return GetDeviceProc(name, NULL);

    dp = AllocMem(sizeof(struct DevProc), MEMF_ANY | MEMF_CLEAR);
    if (dp == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return NULL;
    }

    fl = BADDR(lock);
    dp->dvp_Port    = fl->fl_Task;
    dp->dvp_Lock    = lock;
    dp->dvp_Flags   = 0;
    dp->dvp_DevNode = BADDR(fl->fl_Volume);

    SetIoErr(0);
    return dp;

    AROS_LIBFUNC_EXIT
} /* GetDeviceProcRelative */
