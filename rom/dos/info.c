/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
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

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_DISK_INFO, DOSBase);

    iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(lock))->fh_Device;
    iofs.IOFS.io_Unit   = (struct Unit *)parameterBlock;

    DoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if (iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* Info */
