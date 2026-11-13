/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Check if the handler behind a path is a filesystem.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH2(BOOL, IsFileSystemRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 8, DosRelative)

/*  FUNCTION
        Query whether the handler that would service a path is a
        filesystem. For a name relative to lock that is the handler
        owning the lock.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), or is the console alias "*", lock is ignored. If lock
        is NULL, the name is resolved against the current directory. In
        both cases the call behaves exactly like
        dos.library/IsFileSystem().

    INPUTS
        lock - directory lock to resolve a relative name against,
               or NULL for the current directory
        name - Name of the object or device to query.

    RESULT
        TRUE if the handler is a filesystem, FALSE otherwise.

    NOTES
        This call is AROS-specific.

        DF0:, HD0:, ... are filesystems. CON:, PIPE:, AUX:, ... are not.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/IsFileSystem()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (name == NULL)
    {
        SetIoErr(ERROR_OBJECT_NOT_FOUND);
        return DOSFALSE;
    }

    if (lock == BNULL || dosrel_IsAbsolute(name) || strcmp(name, "*") == 0)
        return IsFileSystem(name);

    return DoPkt(dosrel_LockPort(lock), ACTION_IS_FILESYSTEM, 0, 0, 0, 0, 0) ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* IsFileSystemRelative */
