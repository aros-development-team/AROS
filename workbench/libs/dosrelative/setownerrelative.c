/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Set the owner of a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(BOOL, SetOwnerRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,       D1),
        AROS_LHA(CONST_STRPTR, name,       D2),
        AROS_LHA(ULONG,        owner_info, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 13, DosRelative)

/*  FUNCTION
        Set the owner of a file or directory, resolving a relative name
        against the given directory lock instead of the current
        directory of the calling process.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/SetOwner().

    INPUTS
        lock       - directory lock to resolve a relative name against,
                     or NULL for the current directory
        name       - name of the file
        owner_info - the user id in the upper 16 bits and the group id
                     in the lower 16 bits.

    RESULT
        Boolean success indicator. IoErr() gives additional information
        upon failure.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/SetOwner()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[SetOwnerRelative] lock=0x%p '%s' 0x%08x\n", BADDR(lock), name, owner_info));

    if (name == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return SetOwner(name, owner_info);

    return dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_SET_OWNER, (SIPTR)owner_info, 0, FALSE);

    AROS_LIBFUNC_EXIT
} /* SetOwnerRelative */
