/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

    struct IOFileSys iofs;
    struct DevProc *dvp;
    LONG err;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_RELABEL, DOSBase);
    iofs.io_Union.io_RELABEL.io_NewName = newname;
    iofs.io_Union.io_RELABEL.io_Result  = FALSE;

    /* get the device */
    if ((dvp = GetDeviceProc(drive, NULL)) == NULL)
        return DOSFALSE;

    /* we're only interested in real devices */
    if (dvp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }

    err = DoIOFS(&iofs, dvp, NULL, DOSBase);

    FreeDeviceProc(dvp);

    if (err != 0)
        return DOSFALSE;

    return iofs.io_Union.io_RELABEL.io_Result ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* Relabel */
