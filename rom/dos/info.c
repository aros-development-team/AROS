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

	AROS_LH2(LONG, Info,

/*  SYNOPSIS */
	AROS_LHA(BPTR             , lock, D1),
	AROS_LHA(struct InfoData *, parameterBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 19, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

#warning Add documentation

    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_DISK_INFO, DOSBase);

    iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(lock))->fh_Device;
    iofs.IOFS.io_Unit   = (struct Unit *)parameterBlock;

    DoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* Info */
