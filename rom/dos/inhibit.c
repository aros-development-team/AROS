/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, Inhibit,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,  D1),
	AROS_LHA(LONG,         onoff, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 121, Dos)

/*  FUNCTION

    Stop a filesystem from being used.

    INPUTS

    name   --  Name of the device to inhibit (including a ':')
    onoff  --  Specify whether to inhinit (DOSTRUE) or uninhibit (DOSFALSE)
               the device

    RESULT

    A boolean telling whether the action was carried out.

    NOTES

    After uninhibiting a device anything might have happened like the disk
    in the drive was removed.

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

    InitIOFS(&iofs, FSA_INHIBIT, DOSBase);
    iofs.io_Union.io_INHIBIT.io_Inhibit = onoff == DOSTRUE ? TRUE : FALSE;

    /* get the device */
    if ((dvp = GetDeviceProc(name, NULL)) == NULL)
        return DOSFALSE;

    /* we're only interested in real devices */
    if (dvp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }

    err = DoIOFS(&iofs, dvp, NULL, DOSBase);

    FreeDeviceProc(dvp);
    
    return err == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* Inhibit */
