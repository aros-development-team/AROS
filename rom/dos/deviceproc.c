/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DeviceProc - Return a handle to a device's process.
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
        DeviceProc() is an obsolete function that returns the
        MsgPort responsible for a DOS device.

        DeviceProc() will fail if you ask for the MsgPort of a device
        created with AssignPath() as there is no process to return.
        If the device requested is an assign, the IoErr() will contain
        the Lock to the directory (the function will return the device
        on which the lock is set).

    INPUTS
        name - The name of the DOS device, INCLUDING the ':'.

    RESULT
        Either a pointer to the MsgPort, or NULL.

    NOTES
        You should really use GetDeviceProc(), as that function
        returns a more useful structure (DevProc), that will
        persist until FreeDeviceProc() is called on it.

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

    struct MsgPort *res;
    struct DevProc *dvp;
    SIPTR err;

    /* just use GetDeviceProc(), it knows everything useful anyway */
    if ((dvp = GetDeviceProc(name, NULL)) == NULL)
        return NULL;

    /* if GetDeviceProc() had to create the lock (ie non-binding assigns), we
     * can't return it as there's no cleanup function, so we have to error */
    if (dvp->dvp_Flags & DVPF_UNLOCK)
    {
        res = NULL;
        err = ERROR_DEVICE_NOT_MOUNTED;
    }
    else
    {
        /* all good. get the lock and device */
        res = dvp->dvp_Port;
        err = (SIPTR)dvp->dvp_Lock;
    }

    FreeDeviceProc(dvp);

    SetIoErr(err);
    return res;

    AROS_LIBFUNC_EXIT
} /* DeviceProc */
