/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Examine a directory entry with 64-bit sizes.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH3(LONG, Examine64,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                     lock, D1),
        AROS_LHA(struct FileInfoBlock64 *, fib,  D2),
        AROS_LHA(struct TagItem *,         tags, D3),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 13, Dos64)

/*  FUNCTION
        Fill a 64-bit FileInfoBlock with information about the object
        the given lock refers to.

        If the filesystem does not support 64-bit examination the
        request is delegated to the 32-bit Examine() and the result is
        widened; sizes are then limited to what the filesystem
        reports in 32 bits.

    INPUTS
        lock - lock on the object to examine
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
        dos.library/Examine(), ExNext64(), ExamineFH64(),
        AllocDosObject64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#if (__WORDSIZE == 64)
    struct FileLock *fl = BADDR(lock);
    SIPTR err = 0;
    LONG ret;

    ret = dos64_SendPkt(DOS64Base, fl ? fl->fl_Task : GetFileSysTask(),
                        ACTION_EXAMINE_OBJECT64, (SIPTR)lock,
                        (SIPTR)MKBADDR(fib), 0, 0, 0, &err);
    if (ret)
    {
        dos64_FixFIB64(fib);
        return ret;
    }
    if (!dos64_UnsupportedAction(err))
        return DOSFALSE;
#endif

    return dos64_Examine32(DOS64Base, lock, fib, ACTION_EXAMINE_OBJECT);

    AROS_LIBFUNC_EXIT
} /* Examine64 */
