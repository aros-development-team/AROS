/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Change the current 64-bit read/write position in a file.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2QUAD1(QUAD, Seek64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),
        AROS_LHA(LONG, mode, D2),
        AROS_LHA2(QUAD, position, D3, D4),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 5, Dos64)

/*  FUNCTION
        Changes the current read/write position in a file with full
        64-bit range, and/or reads the current position, e.g. to get
        the current position do a Seek64(file, 0, OFFSET_CURRENT).

        If the filesystem does not support 64-bit operations the
        request is delegated to the 32-bit Seek(), provided the
        position is representable in 32 bits.

    INPUTS
        file     - filehandle
        mode     - Where to count from. Either OFFSET_BEGINNING,
                   OFFSET_CURRENT or OFFSET_END.
        position - relative offset in bytes (positive, negative or 0).

    RESULT
        Absolute position in bytes before the Seek64(), -1 if an error
        happened. IoErr() will give additional information in that
        case. If the position is not representable on the filesystem,
        IoErr() returns ERROR_OBJECT_TOO_LARGE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Seek(), SetFileSize64(), GetFilePosition64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return dos64_SeekBuffered(DOS64Base, file, position, mode);

    AROS_LIBFUNC_EXIT
} /* Seek64 */
