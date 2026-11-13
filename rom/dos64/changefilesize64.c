/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Change the size of a file, 64-bit version (boolean result).
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2QUAD1(LONG, ChangeFileSize64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),
        AROS_LHA(LONG, mode, D2),
        AROS_LHA2(QUAD, size, D3, D4),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 10, Dos64)

/*  FUNCTION
        Change the size of a file with full 64-bit range. Behaves like
        SetFileSize64() but returns a boolean result instead of the
        new size, matching the AmigaOS 4 style API.

    INPUTS
        file - filehandle
        mode - OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END
        size - relative size

    RESULT
        Boolean success indicator; on DOSFALSE, IoErr() gives
        additional information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SetFileSize64(), GetFileSize64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(file);

    if (fh == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return DOSFALSE;
    }

    return (dos64_SetFileSize(DOS64Base, fh, size, mode, FALSE) == -1)
        ? DOSFALSE : DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeFileSize64 */
