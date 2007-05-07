/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DeviceProc - Return a handle to a devices process.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(struct MsgPort *, DeviceProc,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 29, Dos)

/*  FUNCTION
	DeviceProc() is an obsolete function that returns the Process
	responsible for a DOS device. It has been updated to return a
	new filesystem device.

	DeviceProc() will fail if you ask for the Process of a device
	created with AssignPath() as there is no process to return.
	If the device requested is an assign, the IoErr() will contain
	the Lock to the directory (the function will return the device
	on which the lock is set).

    INPUTS
	name - The name of the DOS device, without the ':'.

    RESULT
	Either a pointer to the Device structure, or NULL.

    NOTES
	You should really use GetDeviceProc() as this function caters
	for all possible device types.

    EXAMPLE

    BUGS
	Does not support late- and non-bound assigns, or multiple
	path assigns very well.

    SEE ALSO
	GetDeviceProc(), FreeDeviceProc().

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *res = NULL;
    struct DosList *dl;

    dl = LockDosList(LDF_READ|LDF_ALL);
    dl = FindDosEntry(dl, name, LDF_ALL);
    if(dl != NULL)
    {
	/* If it is a device, return the Device */
	if(dl->dol_Type == DLT_DEVICE || dl->dol_Type == DLT_VOLUME ||
	   dl->dol_Type == DLT_DIRECTORY)
	{
	    res = (struct MsgPort *)dl->dol_Ext.dol_AROS.dol_Device;
	}

	/* If it is an assign, return device and lock */
	if(dl->dol_Type == DLT_DIRECTORY)
	{
	    SetIoErr((ULONG)dl->dol_Lock);
	}
    }
    UnLockDosList(LDF_READ|LDF_ALL);

    return res;
    AROS_LIBFUNC_EXIT
} /* DeviceProc */
