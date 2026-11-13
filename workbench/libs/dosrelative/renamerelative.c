/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Rename a file, with names relative to directory locks.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*
 * Where a packet naming an object must go: the handler port, the lock
 * the name is relative to, and (for a name dos.library resolved for
 * us) the DevProc that owns that lock.
 */
struct dosrel_Target
{
    struct MsgPort *port;
    BPTR            lock;
    struct DevProc *dvp;
};

static BOOL dosrel_GetTarget(struct DosRelativeBase *DOSRelativeBase, BPTR lock, CONST_STRPTR name, struct dosrel_Target *t)
{
    t->dvp = NULL;

    if (lock != BNULL && !dosrel_IsAbsolute(name))
    {
        /* Relative to the caller's lock: the lock's handler, as given */
        t->port = dosrel_LockPort(lock);
        t->lock = lock;
        return TRUE;
    }

    /*
     * Absolute, or relative to the current directory: dos.library
     * finds the handler, starting it or resolving the assign when
     * needed, just as Rename() does. The name goes to the handler
     * unchanged - handlers accept it with its volume part, relative to
     * the device lock, exactly as dos.library sends it.
     */
    t->dvp = GetDeviceProc(name, NULL);
    if (t->dvp == NULL)
        return FALSE;

    t->port = t->dvp->dvp_Port;
    t->lock = t->dvp->dvp_Lock;
    return TRUE;
}

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH4(LONG, RenameRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         oldLock, D1),
        AROS_LHA(CONST_STRPTR, oldName, D2),
        AROS_LHA(BPTR,         newLock, D3),
        AROS_LHA(CONST_STRPTR, newName, D4),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 16, DosRelative)

/*  FUNCTION
        Renames a given file, resolving each relative name against its
        own directory lock instead of the current directory of the
        calling process. The old name and the new name must point to the
        same volume.

        Both names, with their locks, are handed to the filesystem in a
        single rename operation; no path string is assembled for either
        side.

        A name that is absolute (names a volume, device or assign, or
        begins with ':') ignores its lock. A name whose lock is NULL is
        resolved against the current directory. When this applies to
        both names the call behaves exactly like dos.library/Rename().

    INPUTS
        oldLock - directory lock to resolve a relative oldName against,
                  or NULL for the current directory
        oldName - Name of the file to rename
        newLock - directory lock to resolve a relative newName against,
                  or NULL for the current directory
        newName - New name of the file to rename

    RESULT
        boolean - DOSTRUE or DOSFALSE. IoErr() provides additional
        information on DOSFALSE.

    NOTES
        This call is AROS-specific.

        Relative names are passed to the filesystem as written; unlike
        dos.library/Rename(), no canonical path is built for a name
        holding a parent reference (a/b//c). The filesystem resolves
        such a reference against the lock it is given.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Rename()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct dosrel_Target oldt, newt;
    BSTR boldname, bnewname;
    LONG status = DOSFALSE;

    D(bug("[RenameRelative] 0x%p '%s' -> 0x%p '%s'\n", BADDR(oldLock), oldName, BADDR(newLock), newName));

    if (oldName == NULL || newName == NULL)
        return DOSFALSE;

    /* Nothing to resolve relative to on either side */
    if ((oldLock == BNULL || dosrel_IsAbsolute(oldName)) &&
        (newLock == BNULL || dosrel_IsAbsolute(newName)))
        return Rename(oldName, newName);

    if (!dosrel_GetTarget(DOSRelativeBase, oldLock, oldName, &oldt))
        return DOSFALSE;

    if (!dosrel_GetTarget(DOSRelativeBase, newLock, newName, &newt))
    {
        FreeDeviceProc(oldt.dvp);
        return DOSFALSE;
    }

    /*
     * Make sure they're on the same device.
     * XXX this is insufficient, see comments in dos/samedevice.c
     */
    if (oldt.port != newt.port)
    {
        SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
    }
    else
    {
        boldname = dosrel_C2BSTR(DOSRelativeBase, oldName);
        bnewname = dosrel_C2BSTR(DOSRelativeBase, newName);

        if (boldname != BNULL && bnewname != BNULL)
            status = DoPkt(oldt.port, ACTION_RENAME_OBJECT, (SIPTR)oldt.lock, (SIPTR)boldname,
                           (SIPTR)newt.lock, (SIPTR)bnewname, 0);
        else
            SetIoErr(ERROR_NO_FREE_STORE);

        dosrel_FreeBSTR(DOSRelativeBase, boldname);
        dosrel_FreeBSTR(DOSRelativeBase, bnewname);
    }

    FreeDeviceProc(oldt.dvp);
    FreeDeviceProc(newt.dvp);

    return status;

    AROS_LIBFUNC_EXIT
} /* RenameRelative */
