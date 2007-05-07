/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Query a filesystem for interactiveness.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, IsInteractive,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 36, Dos)

/*  FUNCTION
	Check if file is bound to an interactive device such as a console
	or shell window.

    INPUTS
	file   - filehandle

    RESULT
	!= 0 if the file is interactive, 0 if it is not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_IS_INTERACTIVE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    /* Return */
    if(iofs.io_DosError != 0)
	return 0;
    else
	return iofs.io_Union.io_IS_INTERACTIVE.io_IsInteractive;

    AROS_LIBFUNC_EXIT
} /* IsInteractive */
