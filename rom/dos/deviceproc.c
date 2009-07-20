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
	GetDeviceProc(), FreeDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *res = NULL;
    struct DevProc *dvp;
    char namebuffer[32];
    char *tarptr = namebuffer;
    ULONG len = 30;

    /* append a colon to the name, GetDeviceProc() needs a full path,
       while DeviceProc() has it without colon */
    while((*tarptr++ = *name++) && (--len));
    if(tarptr[-1] == 0) --tarptr;
    *tarptr++ = ':';
    *tarptr = 0;

    /* just use GetDeviceProc(), it knows everything useful anyway */
    if ((dvp = GetDeviceProc(namebuffer, NULL)) == NULL)
        return NULL;

    /* if GetDeviceProc() had to create the lock (ie non-binding assigns), we
     * can't return it as there's no cleanup function, so we have to error */
    if (dvp->dvp_Flags & DVPF_UNLOCK) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }

    /* all good. get the lock and device */
    SetIoErr(dvp->dvp_Lock);
    res = dvp->dvp_Port;

    FreeDeviceProc(dvp);

    return res;

    AROS_LIBFUNC_EXIT
} /* DeviceProc */
