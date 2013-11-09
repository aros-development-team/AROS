/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/record.h>
#include <proto/dos.h>

        AROS_LH2(BOOL, LockRecords,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock *, recArray, D1),
        AROS_LHA(ULONG              , timeout, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 46, Dos)

/*  FUNCTION
        Lock several records at the same time. The timeout specified is applied
        to each lock to attempt. The array of RecordLock:s is terminated with
        an entry where rec_FH is equal to NULL.

    INPUTS
        recArray - array of records to lock
        timeout  - maximum number of ticks to wait for a lock to be ready

    RESULT
        Success/failure indication. In case of a success, all the record locks
        are locked. In case of failure, no record locks are locked.

    NOTES
        A set of records should always be locked in the same order so as to
        reduce possiblities of deadlock.

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockRecords()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RecordLock *rLock = recArray;

    while (BADDR(recArray->rec_FH) != NULL)
    {
        BPTR temp;

        if (!LockRecord(recArray->rec_FH, recArray->rec_Offset,
                        recArray->rec_Length, recArray->rec_Mode, timeout))
        {
            temp = recArray->rec_FH;
            UnLockRecords(rLock);
            recArray->rec_FH = temp;

            return DOSFALSE;
        }

        recArray++;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* LockRecords */
