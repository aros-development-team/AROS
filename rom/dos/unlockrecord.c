/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BOOL, UnLockRecord,

/*  SYNOPSIS */
        AROS_LHA(BPTR , fh, D1),
        AROS_LHA(ULONG, offset, D2),
        AROS_LHA(ULONG, length, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 47, Dos)

/*  FUNCTION
        Release a lock made with LockRecord().

    INPUTS
        fh     - filehandle the lock was made on
        offset - starting position of the lock
        length - length of the record in bytes

    RESULT

    NOTES
        The length and offset must match the corresponding LockRecord()
        call.

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecord(), UnLockRecords()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG status;
    struct FileHandle *fileH = BADDR(fh);

    if (fh == BNULL)
    {
        return DOSFALSE;
    }

    status = dopacket3(DOSBase, NULL, fileH->fh_Type, ACTION_FREE_RECORD, fileH->fh_Arg1, offset, length);

    return status;

    AROS_LIBFUNC_EXIT
} /* UnLockRecord */
