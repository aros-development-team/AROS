/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Lock a portion of a file, 64-bit version.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2(BOOL, LockRecord64,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock64 *, rec,     D1),
        AROS_LHA(ULONG,                 timeout, D2),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 17, Dos64)

/*  FUNCTION
        Lock a portion of a file for exclusive access, with 64-bit
        offsets. The record is described by a RecordLock64 structure
        holding the filehandle, offset, length and locking mode. A
        timeout may be specified which is the maximum amount of time
        to wait for the record to become available.

        If the filesystem does not support 64-bit record locking the
        request is delegated to the 32-bit LockRecord(), provided the
        record lies within the first 4GB of the file.

    INPUTS
        rec     - the record to lock (rec_FH, rec_Offset, rec_Length
                  and rec_Mode must be filled in)
        timeout - timeout interval measured in ticks (may be 0)

    RESULT
        Success/failure indicator. On failure IoErr() gives additional
        information; ERROR_OBJECT_TOO_LARGE indicates the record is
        not representable on the filesystem.

    NOTES
        Record locks are cooperative, meaning that they only affect
        other calls to LockRecord()/LockRecord64().

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecords64(), UnLockRecord64(), dos.library/LockRecord()

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

        ret = dos64_SendPkt(DOS64Base, fileH->fh_Type, ACTION_LOCK_RECORD64,
                            fileH->fh_Arg1, rec->rec_Offset, rec->rec_Length,
                            rec->rec_Mode, timeout, &err);
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

    return LockRecord(rec->rec_FH, (ULONG)rec->rec_Offset,
                      (ULONG)rec->rec_Length, rec->rec_Mode, timeout);

    AROS_LIBFUNC_EXIT
} /* LockRecord64 */
