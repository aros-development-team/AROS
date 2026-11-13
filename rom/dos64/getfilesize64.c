/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Read the 64-bit size of a file.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH1(QUAD, GetFileSize64,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 8, Dos64)

/*  FUNCTION
        Read the size of an open file with full 64-bit range.

        If the filesystem does not support 64-bit operations the size
        is obtained via ExamineFH() or, as a last resort, by seeking.

    INPUTS
        file - filehandle

    RESULT
        Size of the file in bytes, or -1 if an error happened. IoErr()
        will give additional information in that case.

    NOTES
        Pending buffered writes on the filehandle are flushed first.

    EXAMPLE

    BUGS

    SEE ALSO
        SetFileSize64(), GetFilePosition64(), ExamineFH64()

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

    if (fh->fh_Flags & FHF_WRITE)
        Flush(file);

    return dos64_GetFileSize(DOS64Base, fh);

    AROS_LIBFUNC_EXIT
} /* GetFileSize64 */
