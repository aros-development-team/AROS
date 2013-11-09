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

        AROS_LH5(BOOL, LockRecord,

/*  SYNOPSIS */
        AROS_LHA(BPTR , fh, D1),
        AROS_LHA(ULONG, offset, D2),
        AROS_LHA(ULONG, length, D3),
        AROS_LHA(ULONG, mode, D4),
        AROS_LHA(ULONG, timeout, D5),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 45, Dos)

/*  FUNCTION
        Lock a portion of a file for exclusive access. A timeout may be
        specified which is the maximum amount of time to wait for the record
        to be available.

    INPUTS
        fh      - file handle for the file to lock a record of
        offset  - starting position of the lock
        length  - length of the record in bytes
        mode    - lock type
        timeout - timeout interval measured in ticks (may be 0)

    RESULT
        Success/failure indicator.

    NOTES
        Record locks are cooperative, meaning that they only affect other calls
        to LockRecord().

    EXAMPLE

    BUGS

    SEE ALSO
        LockRecords(), UnLockRecord()

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

    status = dopacket5(DOSBase, NULL, fileH->fh_Type, ACTION_LOCK_RECORD, fileH->fh_Arg1, offset, length, mode, timeout);

    return status;
 
    AROS_LIBFUNC_EXIT
} /* LockRecord */
