/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Format a device.
    Lang: english
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
	AROS_LHA(STRPTR, devicename, D1),
	AROS_LHA(STRPTR, volumename, D2),
	AROS_LHA(ULONG,  dostype,    D3),

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);
    struct DosList *dl;
    BOOL success=0;

    dl=LockDosList(LDF_DEVICES|LDF_READ);
    dl=FindDosEntry(dl,devicename,LDF_DEVICES);
    if(dl!=NULL)
    {

	/* Get pointer to I/O request. Use stackspace for now. */
	struct IOFileSys io,*iofs=&io;

	/* Prepare I/O request. */
	iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
	iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
	iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
	iofs->IOFS.io_Device =dl->dol_Device;
	iofs->IOFS.io_Unit   =dl->dol_Unit;
	iofs->IOFS.io_Command=FSA_FORMAT;
	iofs->IOFS.io_Flags  =0;
	iofs->io_Union.io_FORMAT.io_VolumeName=volumename;
	iofs->io_Union.io_FORMAT.io_DosType=dostype;

	/* Send the request. */
	DoIO(&iofs->IOFS);

	/* Set error code */
	if(!iofs->io_DosError)
	    success=1;
	else
	    SetIoErr(iofs->io_DosError);
    }else
	SetIoErr(ERROR_DEVICE_NOT_MOUNTED);

    /* All Done. */
    UnLockDosList(LDF_DEVICES|LDF_READ);
    return success;
    AROS_LIBFUNC_EXIT
} /* Format */
