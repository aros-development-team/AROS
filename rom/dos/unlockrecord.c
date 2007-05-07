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

    fh      --  filehandle the lock was made on
    offset  --  starting position of the lock
    length  --  length of the record in bytes

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

    struct IOFileSys iofs;
    struct FileHandle *fileH = fh;

    if (fh == NULL)
    {
	return DOSFALSE;
    }

    InitIOFS(&iofs, FSA_UNLOCK_RECORD, DOSBase);

    iofs.IOFS.io_Device = fileH->fh_Device;
    iofs.IOFS.io_Unit = fileH->fh_Unit;
    
    iofs.io_Union.io_RECORD.io_Offset = offset;
    iofs.io_Union.io_RECORD.io_Size = length;

    DosDoIO(&iofs.IOFS);
    
    SetIoErr(iofs.io_DosError);
    
    if (iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* UnLockRecord */
