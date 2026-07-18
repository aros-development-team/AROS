/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Change the size of a file, 64-bit version.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2QUAD1(QUAD, SetFileSize64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),
        AROS_LHA(LONG, mode, D2),
        AROS_LHA2(QUAD, offset, D3, D4),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 6, Dos64)

/*  FUNCTION
        Change the size of a file with full 64-bit range.

        If the filesystem does not support 64-bit operations the
        request is delegated to the 32-bit SetFileSize(), provided the
        offset is representable in 32 bits.

    INPUTS
        file   - filehandle
        mode   - OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END
        offset - relative size

    RESULT
        New size of the file or -1 in case of an error.
        IoErr() gives additional information in that case. If the size
        is not representable on the filesystem, IoErr() returns
        ERROR_OBJECT_TOO_LARGE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/SetFileSize(), Seek64(), GetFileSize64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(file);

    if (fh == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return -1;
    }

    return dos64_SetFileSize(DOS64Base, fh, offset, mode, TRUE);

    AROS_LIBFUNC_EXIT
} /* SetFileSize64 */
