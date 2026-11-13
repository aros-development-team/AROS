/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Lock a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

static BPTR dosrel_Lock(struct DosRelativeBase *DOSRelativeBase, BPTR lock,
                        CONST_STRPTR name, LONG accessMode, LONG soft_nesting);

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(BPTR, LockRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,       D1),
        AROS_LHA(CONST_STRPTR, name,       D2),
        AROS_LHA(LONG,         accessMode, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 5, DosRelative)

/*  FUNCTION
        Gets a lock on a file or directory, resolving a relative name
        against the given directory lock instead of the current
        directory of the calling process.

        The name is handed to the filesystem together with the lock, so
        the object is located in a single filesystem operation: no path
        string is assembled, and a link in the path cannot be retargeted
        by another process between the lookup and the lock.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/Lock().

    INPUTS
        lock       - directory lock to resolve a relative name against,
                     or NULL for the current directory
        name       - NUL terminated name of the file or directory.
        accessMode - One of SHARED_LOCK
                            EXCLUSIVE_LOCK

    RESULT
        Handle to the file or directory or 0 if the object couldn't be
        locked. IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific. It is the equivalent of the POSIX
        *at() family's directory-relative name resolution.

        Soft links are followed as by Lock(), up to the same nesting
        limit; a link target without a volume specifier is itself
        resolved relative to lock.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Lock(), OpenRelative(), dos.library/UnLock()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (name == NULL)
        return BNULL;

    D(bug("[LockRelative] lock=0x%p '%s':%d\n", BADDR(lock), name, accessMode));

    return dosrel_Lock(DOSRelativeBase, lock, name, accessMode, DOSREL_MAX_SOFT_LINK_NESTING);

    AROS_LIBFUNC_EXIT
} /* LockRelative */

/* Lock name, following soft links by calling itself. */
static BPTR dosrel_Lock(struct DosRelativeBase *DOSRelativeBase, BPTR lock,
                        CONST_STRPTR name, LONG accessMode, LONG soft_nesting)
{
    BPTR   ret;
    LONG   error;
    STRPTR softname;

    /* Nothing to resolve relative to: dos.library already does this */
    if (lock == BNULL || dosrel_IsAbsolute(name))
        return Lock(name, accessMode);

    if (soft_nesting == 0)
    {
        SetIoErr(ERROR_TOO_MANY_LEVELS);
        return BNULL;
    }

    ret = (BPTR)dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_LOCATE_OBJECT,
                                   accessMode, 0, TRUE);
    if (ret != BNULL)
        return ret;

    error = IoErr();
    if (error != ERROR_IS_SOFT_LINK)
        return BNULL;

    /*
     * The link target is a path relative to the same lock (or an
     * absolute one, which the recursion hands to Lock()).
     */
    softname = dosrel_ReadSoftlink(DOSRelativeBase, lock, name);
    if (softname == NULL)
        return BNULL;

    ret = dosrel_Lock(DOSRelativeBase, lock, softname, accessMode, soft_nesting - 1);
    error = IoErr();
    FreeVec(softname);

    SetIoErr(ret ? 0 : error);
    return ret;
}
