/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Unlock a record of a file, 64-bit version.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH1(BOOL, UnLockRecord64,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock64 *, rec, D1),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 19, Dos64)

/*  FUNCTION
        Release a record lock obtained with LockRecord64(). The
        rec_FH, rec_Offset, rec_Length values must match the lock
        request exactly.

    INPUTS
        rec - the record to unlock

    RESULT
        Success/failure indicator. On failure IoErr() gives additional
        information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecord64(), UnLockRecords64(), dos.library/UnLockRecord()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fileH;

    if (rec == NULL || (fileH = BADDR(rec->rec_FH)) == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return DOSFALSE;
    }

#if (__WORDSIZE == 64)
    {
        SIPTR err = 0;
        LONG  ret;

        ret = dos64_SendPkt(DOS64Base, fileH->fh_Type, ACTION_FREE_RECORD64,
                            fileH->fh_Arg1, rec->rec_Offset, rec->rec_Length,
                            0, 0, &err);
        if (!(ret == DOSFALSE && dos64_UnsupportedAction(err)))
            return ret;
    }
#endif

    /* 32-bit delegation */
    if (rec->rec_Offset > 0xFFFFFFFFULL || rec->rec_Length > 0xFFFFFFFFULL
        || rec->rec_Offset + rec->rec_Length > 0xFFFFFFFFULL)
    {
        SetIoErr(ERROR_OBJECT_TOO_LARGE);
        return DOSFALSE;
    }

    return UnLockRecord(rec->rec_FH, (ULONG)rec->rec_Offset,
                        (ULONG)rec->rec_Length);

    AROS_LIBFUNC_EXIT
} /* UnLockRecord64 */
