/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Set the protection bits of a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(LONG, SetProtectionRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,    D1),
        AROS_LHA(CONST_STRPTR, name,    D2),
        AROS_LHA(ULONG,        protect, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 14, DosRelative)

/*  FUNCTION
        Set the protection bits of a file or directory, resolving a
        relative name against the given directory lock instead of the
        current directory of the calling process.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/SetProtection().

    INPUTS
        lock    - directory lock to resolve a relative name against,
                  or NULL for the current directory
        name    - name of the file
        protect - new protection bits (FIBF_*).

    RESULT
        Boolean success indicator. IoErr() gives additional information
        upon failure.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/SetProtection()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[SetProtectionRelative] lock=0x%p '%s' 0x%08x\n", BADDR(lock), name, protect));

    if (name == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return SetProtection(name, protect);

    return dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_SET_PROTECT, (SIPTR)protect, 0, FALSE);

    AROS_LIBFUNC_EXIT
} /* SetProtectionRelative */
