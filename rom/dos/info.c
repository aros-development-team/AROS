/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>
#include <dos/filesystem.h>

	AROS_LH2(BOOL, Info,

/*  SYNOPSIS */
	AROS_LHA(BPTR             , lock, D1),
	AROS_LHA(struct InfoData *, parameterBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 19, Dos)

/*  FUNCTION

    Get information about a volume in the system.

    INPUTS

    lock            --  a lock on any file on the volume for which information
                        should be supplied
    parameterBlock  --  pointer to an InfoData structure

    RESULT

    Boolean indicating success or failure. If TRUE (success) the
    'parameterBlock' is filled with information on the volume.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    <dos/dos.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct IOFileSys iofs;
    struct FileHandle *fh = (struct FileHandle *)lock;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_DISK_INFO, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_INFO.io_Info = parameterBlock;

    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if (iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* Info */
