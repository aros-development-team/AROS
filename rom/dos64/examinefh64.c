/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Examine an open file with 64-bit sizes.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH3(LONG, ExamineFH64,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                     fh,   D1),
        AROS_LHA(struct FileInfoBlock *,   fib,  D2),
        AROS_LHA(struct TagItem *,         tags, D3),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 15, Dos64)

/*  FUNCTION
        Fill a FileInfoBlock with information about an open file,
        including the 64-bit fib_Size64 and fib_NumBlocks64 fields.
        fib_Size is 0 when the size does not fit in 31 bits.

        If the filesystem does not support 64-bit examination the
        request is delegated to the 32-bit ExamineFH(). In that case
        the file size is additionally corrected through the
        filesystem's 64-bit size query, when it provides one.

    INPUTS
        fh   - filehandle to examine
        fib  - FileInfoBlock to fill, allocated with
               AllocDosObject(DOS_FIB, NULL) or
               AllocDosObject64(DOS64_FIB, NULL)
        tags - future extensions, pass NULL

    RESULT
        Boolean success indicator. On failure IoErr() gives additional
        information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Examine64(), GetFileSize64(), dos.library/ExamineFH()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *handle = BADDR(fh);
    SIPTR err = 0;
    LONG ret;

    if (handle == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return DOSFALSE;
    }

    dos64_PrepFIB64(fib);
    ret = dos64_SendPkt(DOS64Base, handle->fh_Type, ACTION_EXAMINE_FH64,
                        handle->fh_Arg1, (SIPTR)MKBADDR(fib), 0, 0, 0, &err);
    if (ret)
    {
        dos64_FixFIB64(fib);
        return ret;
    }
    if (!dos64_UnsupportedAction(err))
        return DOSFALSE;

    ret = dos64_Examine32(DOS64Base, fh, fib, ACTION_EXAMINE_FH);
    if (ret)
        dos64_FixSizeFH(DOS64Base, handle, fib);

    return ret;

    AROS_LIBFUNC_EXIT
} /* ExamineFH64 */
