/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <dos/filesystem.h>
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

    Lock a portion of a file for exclusive access. A timeout may be specified
    which is the maximum amount of time to wait for the record to be available.

    INPUTS

    fh       --  file handle for the file to lock a record of
    offset   --  starting position of the lock
    length   --  length of the record in bytes
    mode     --  lock type
    timeout  --  timeout interval measured in ticks (may be 0)

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

    struct IOFileSys iofs;
    struct FileHandle *fileH = fh;

    if (fh == NULL)
    {
	return DOSFALSE;
    }

    InitIOFS(&iofs, FSA_LOCK_RECORD, DOSBase);

    iofs.IOFS.io_Device = fileH->fh_Device;
    iofs.IOFS.io_Unit = fileH->fh_Unit;

    iofs.io_Union.io_RECORD.io_Offset = offset;
    iofs.io_Union.io_RECORD.io_Size = length;
    iofs.io_Union.io_RECORD.io_RecordMode = mode;
    iofs.io_Union.io_RECORD.io_Timeout = timeout;

    DosDoIO(&iofs.IOFS);
    
    SetIoErr(iofs.io_DosError);
    
    if (iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* LockRecord */
