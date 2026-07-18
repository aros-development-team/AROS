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
        AROS_LHA(struct FileInfoBlock64 *, fib,  D2),
        AROS_LHA(struct TagItem *,         tags, D3),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 15, Dos64)

/*  FUNCTION
        Fill a 64-bit FileInfoBlock with information about an open
        file.

        If the filesystem does not support 64-bit examination the
        request is delegated to the 32-bit ExamineFH() and the result
        is widened. In that case the file size is additionally
        corrected through the filesystem's 64-bit size query, when it
        provides one.

    INPUTS
        fh   - filehandle to examine
        fib  - FileInfoBlock64 to fill, allocated with
               AllocDosObject64(DOS64_FIB, NULL) or at least longword
               aligned
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

#if (__WORDSIZE == 64)
    ret = dos64_SendPkt(DOS64Base, handle->fh_Type, ACTION_EXAMINE_FH64,
                        handle->fh_Arg1, (SIPTR)MKBADDR(fib), 0, 0, 0, &err);
    if (ret)
    {
        dos64_FixFIB64(fib);
        return ret;
    }
    if (!dos64_UnsupportedAction(err))
        return DOSFALSE;
#endif

    ret = dos64_Examine32(DOS64Base, fh, fib, ACTION_EXAMINE_FH);
    if (ret)
    {
        /*
         * The 32-bit FileInfoBlock cannot represent sizes >= 4GB; if
         * the filesystem offers a 64-bit size query, use it to fix up
         * the widened result.
         */
#if (__WORDSIZE == 64)
        QUAD size = dos64_SendPkt(DOS64Base, handle->fh_Type,
                                  ACTION_GET_FILE_SIZE64,
                                  handle->fh_Arg1, 0, 0, 0, 0, &err);
#else
        QUAD size = dos64_SendPkt64OS4(DOS64Base, handle->fh_Type,
                                       ACTION_GET_FILE_SIZE64,
                                       handle->fh_Arg1, 0, 0, &err);
#endif
        if (size != -1)
            fib->fib_Size = size;
        SetIoErr(0);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* ExamineFH64 */
