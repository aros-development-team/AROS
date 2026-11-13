/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Lock several records of files, 64-bit version.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2(BOOL, LockRecords64,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock64 *, recArray, D1),
        AROS_LHA(ULONG,                 timeout,  D2),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 18, Dos64)

/*  FUNCTION
        Lock several records of files for exclusive access, with
        64-bit offsets. The array is terminated by an entry with
        rec_FH set to NULL. If any lock in the array fails, all
        already obtained locks are released again.

    INPUTS
        recArray - array of RecordLock64 entries to lock
        timeout  - timeout interval measured in ticks (may be 0),
                   applied to each record

    RESULT
        Success/failure indicator. On failure IoErr() gives additional
        information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecord64(), UnLockRecords64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RecordLock64 *rLock = recArray;

    while (BADDR(recArray->rec_FH) != NULL)
    {
        BPTR temp;

        if (!LockRecord64(recArray, timeout))
        {
            temp = recArray->rec_FH;
            recArray->rec_FH = BNULL;
            UnLockRecords64(rLock);
            recArray->rec_FH = temp;

            return DOSFALSE;
        }

        recArray++;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* LockRecords64 */
