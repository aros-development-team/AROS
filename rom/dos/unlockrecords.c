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

        AROS_LH1(BOOL, UnLockRecords,

/*  SYNOPSIS */
        AROS_LHA(struct RecordLock *, recArray, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 48, Dos)

/*  FUNCTION
        Release an array of record locks obtained with LockRecords().

    INPUTS
        recArray - array of record locks (previously locked with LockRecords())

    RESULT
        success - Boolean success indicator.

    NOTES
        An array of records may not be modified when records are locked.

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecords()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    while (NULL != BADDR(recArray->rec_FH))
    {
        LONG success = UnLockRecord(recArray->rec_FH, recArray->rec_Offset,
                                    recArray->rec_Length);

        if (success != DOSTRUE)
        {
            return success;
        }

        /* everything OK -> advance to the next entry */
        recArray++;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* UnLockRecords */
