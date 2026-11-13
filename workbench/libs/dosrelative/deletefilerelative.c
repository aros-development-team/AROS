/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Delete a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH2(BOOL, DeleteFileRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock, D1),
        AROS_LHA(CONST_STRPTR, name, D2),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 10, DosRelative)

/*  FUNCTION
        Tries to delete a file or directory, resolving a relative name
        against the given directory lock instead of the current
        directory of the calling process. May fail if the file is in
        use or protected from deletion.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/DeleteFile().

    INPUTS
        lock - directory lock to resolve a relative name against,
               or NULL for the current directory
        name - NUL terminated name of the file or directory.

    RESULT
        DOSTRUE if the object was deleted, DOSFALSE otherwise.
        IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/DeleteFile()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[DeleteFileRelative] lock=0x%p '%s'\n", BADDR(lock), name));

    if (name == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return DeleteFile(name);

    return dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_DELETE_OBJECT, 0, 0, TRUE);

    AROS_LIBFUNC_EXIT
} /* DeleteFileRelative */
