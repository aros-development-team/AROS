/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Unlock several records of files, 64-bit version.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH1(BOOL, UnLockRecords64,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock64 *, recArray, D1),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 20, Dos64)

/*  FUNCTION
        Release the record locks obtained with LockRecords64(). The
        array is terminated by an entry with rec_FH set to NULL.

    INPUTS
        recArray - array of RecordLock64 entries to unlock, as passed
                   to LockRecords64()

    RESULT
        Success/failure indicator. Unlocking continues over failing
        entries; the result of the last failing unlock is returned and
        IoErr() gives additional information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecords64(), UnLockRecord64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL ret = DOSTRUE;

    while (BADDR(recArray->rec_FH) != NULL)
    {
        if (!UnLockRecord64(recArray))
            ret = DOSFALSE;

        recArray++;
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* UnLockRecords64 */
