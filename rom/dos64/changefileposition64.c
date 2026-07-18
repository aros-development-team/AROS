/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Change the current 64-bit position in a file (boolean result).
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2QUAD1(LONG, ChangeFilePosition64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),
        AROS_LHA(LONG, mode, D2),
        AROS_LHA2(QUAD, position, D3, D4),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 9, Dos64)

/*  FUNCTION
        Changes the current read/write position in a file with full
        64-bit range. Behaves like Seek64() but returns a boolean
        result instead of the previous position, matching the
        AmigaOS 4 style API.

    INPUTS
        file     - filehandle
        mode     - Where to count from. Either OFFSET_BEGINNING,
                   OFFSET_CURRENT or OFFSET_END.
        position - relative offset in bytes (positive, negative or 0).

    RESULT
        Boolean success indicator; on DOSFALSE, IoErr() gives
        additional information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Seek64(), GetFilePosition64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (dos64_SeekBuffered(DOS64Base, file, position, mode) == -1)
        ? DOSFALSE : DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeFilePosition64 */
