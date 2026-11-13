/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Set the date of a file or directory relative to a directory lock.
*/

#include <proto/exec.h>
#include "dosrelative_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(BOOL, SetFileDateRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                    lock, D1),
        AROS_LHA(CONST_STRPTR,            name, D2),
        AROS_LHA(const struct DateStamp *, date, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 12, DosRelative)

/*  FUNCTION
        Set the modification date of a file or directory, resolving a
        relative name against the given directory lock instead of the
        current directory of the calling process.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), lock is ignored. If lock is NULL, the name is resolved
        against the current directory. In both cases the call behaves
        exactly like dos.library/SetFileDate().

    INPUTS
        lock - directory lock to resolve a relative name against,
               or NULL for the current directory
        name - name of the file
        date - new modification date.

    RESULT
        Boolean success indicator. IoErr() gives additional information
        upon failure.

    NOTES
        This call is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/SetFileDate(), dos.library/DateStamp()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[SetFileDateRelative] lock=0x%p '%s'\n", BADDR(lock), name));

    if (name == NULL)
        return DOSFALSE;

    if (lock == BNULL || dosrel_IsAbsolute(name))
        return SetFileDate(name, date);

    return dosrel_SendNamePkt(DOSRelativeBase, lock, name, ACTION_SET_DATE, (SIPTR)date, 0, FALSE);

    AROS_LIBFUNC_EXIT
} /* SetFileDateRelative */
