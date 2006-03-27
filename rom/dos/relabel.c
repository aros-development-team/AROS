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

	AROS_LH2(LONG, Relabel,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, drive, D1),
	AROS_LHA(CONST_STRPTR, newname, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 120, Dos)

/*  FUNCTION

    Change names of a volume.

    INPUTS

    drive    --  The name of the device to rename (including the ':').
    newname  --  The new name for the device (without the ':').

    RESULT

    A boolean telling whether the name change was successful or not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_RELABEL, DOSBase);

    iofs.IOFS.io_Device = GetDevice(drive, &iofs.IOFS.io_Unit, DOSBase);

    if(iofs.IOFS.io_Device == NULL)
    {
	return DOSFALSE;
    }

    iofs.io_Union.io_RELABEL.io_NewName = newname;
    iofs.io_Union.io_RELABEL.io_Result  = FALSE;

    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return iofs.io_Union.io_RELABEL.io_Result ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* Relabel */
