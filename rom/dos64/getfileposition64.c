/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Read the current 64-bit position in a file.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH1(QUAD, GetFilePosition64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 7, Dos64)

/*  FUNCTION
        Read the current read/write position in a file with full
        64-bit range, without changing it. Buffered data on the
        filehandle is taken into account.

    INPUTS
        file - filehandle

    RESULT
        Current absolute position in bytes, or -1 if an error
        happened. IoErr() will give additional information in that
        case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Seek64(), GetFileSize64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(file);
    QUAD pos;

    if (fh == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return -1;
    }

    pos = dos64_Seek(DOS64Base, fh, 0, OFFSET_CURRENT);
    if (pos == -1)
        return -1;

    /* Account for buffered data not yet seen by the filesystem */
    if (fh->fh_Flags & FHF_WRITE)
        pos += fh->fh_Pos;
    else
        pos -= fh->fh_End - fh->fh_Pos;

    return pos;

    AROS_LIBFUNC_EXIT
} /* GetFilePosition64 */
