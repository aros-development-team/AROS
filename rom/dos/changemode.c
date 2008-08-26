/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the mode of a filehandle or lock.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, ChangeMode,

/*  SYNOPSIS */
	AROS_LHA(ULONG, type,    D1),
	AROS_LHA(BPTR,  object,  D2),
	AROS_LHA(ULONG, newmode, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 75, Dos)

/*  FUNCTION
	Try to change the access mode of a lock or filehandle.

    INPUTS
	type    - CHANGE_FH or CHANGE_LOCK.
	object  - Filehandle or lock.
	newmode - New mode, either SHARED_LOCK or EXCLUSIVE_LOCK.

    RESULT
	!= 0 if all went well, otherwise 0. IoErr() gives additional
	information in the latter case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Since filehandles and locks are identical under AROS the type
	argument is ignored.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(object);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_FILE_MODE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_FILE_MODE.io_FileMode =
        (newmode == EXCLUSIVE_LOCK) ? FMF_LOCK : 0;
    iofs.io_Union.io_FILE_MODE.io_Mask = FMF_LOCK;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    /* Set error code and return */
    if (iofs.io_DosError != 0)
    {
        SetIoErr(iofs.io_DosError);
	return DOSFALSE;
    }
    
    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ChangeMode */
