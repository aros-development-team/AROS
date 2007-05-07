/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the current mode of a console device.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, SetMode,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),
	AROS_LHA(LONG, mode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 71, Dos)

/*  FUNCTION
	SetMode() can be used to change a console handler between
	RAW: mode and CON: mode.

    INPUTS
	fh      -   The filehandle describing the console.
	mode    -   The new mode of the console:
			1   - RAW: mode
			0   - CON: mode

    RESULT
	This function will return whether it succeeded:

	== DOSTRUE  console mode changed
	!= DOSTRUE  console mode change failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys     iofs;
    struct FileHandle   *fha = (struct FileHandle *)BADDR(fh);

    InitIOFS(&iofs, FSA_CONSOLE_MODE, DOSBase);

    iofs.IOFS.io_Device = fha->fh_Device;
    iofs.IOFS.io_Unit   = fha->fh_Unit;

    iofs.io_Union.io_CONSOLE_MODE.io_ConsoleMode = mode;

    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    return iofs.io_DosError == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* SetMode */
