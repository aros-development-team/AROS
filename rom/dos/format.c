/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Format a device.
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, Format,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, devicename, D1),
	AROS_LHA(CONST_STRPTR, volumename, D2),
	AROS_LHA(ULONG,        dostype,    D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 119, Dos)

/*  FUNCTION
	Initialise a filesystem for use by the system. This instructs
	a filesystem to write out the data that it uses to describe the
	device.

	The device should already have been formatted.

    INPUTS
	devicename	- Name of the device to format.
	volumename	- The name you wish the volume to be called.
	dostype		- The DOS type you wish on the disk.

    RESULT
	!= 0 if the format was successful, 0 otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DevProc *dvp;
    LONG err;

    /* Get space for I/O request. Use stackspace for now. */
    struct IOFileSys iofs;
    
    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_FORMAT, DOSBase);
    iofs.io_Union.io_FORMAT.io_VolumeName = volumename;
    iofs.io_Union.io_FORMAT.io_DosType = dostype;
    
    /* get the device */
    if ((dvp = GetDeviceProc(devicename, NULL)) == NULL)
        return DOSFALSE;

    /* we're only interested in real devices */
    if (dvp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }

    err = DoIOFS(&iofs, dvp, NULL, DOSBase);

    FreeDeviceProc(dvp);
    
    return err == 0 ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
} /* Format */
