/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Create a new directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH2(BPTR, CreateDirRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 9, DosRelative)

/*  FUNCTION
        Creates a new directory under the given name, resolving a
        relative name against the given directory lock instead of the
        current directory of the calling process. If all went well, an
        exclusive lock on the new directory is returned.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/CreateDir().

    INPUTS
        lock - directory lock to resolve a relative name against,
               or NULL for the current directory
        name - NUL terminated name.

    RESULT
        Exclusive lock to the new directory or 0 if it couldn't be
        created. IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/CreateDir(), LockRelative()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[CreateDirRelative] lock=0x%p '%s'\n", BADDR(lock), name));

    if (name == NULL)
        return BNULL;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return CreateDir(name);

    return (BPTR)dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_CREATE_DIR, 0, 0, TRUE);

    AROS_LIBFUNC_EXIT
} /* CreateDirRelative */
