/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add or remove cache memory from a filesystem.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, AddBuffers,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, devicename, D1),
        AROS_LHA(LONG,         numbuffers, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 122, Dos)

/*  FUNCTION
        Add or remove cache memory from a filesystem.

    INPUTS
        devicename  --  NUL terminated dos device name.
        numbuffers  --  Number of buffers to add. May be negative.

    RESULT
        != 0 on success (IoErr() gives the actual number of buffers),
        0 else (IoErr() gives the error code).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    The error value in case of a filesystem error will be reported in
    the io_MORE_CACHE.io_NumBuffers field.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT
    
    struct DevProc *dvp;
    LONG ret;

    /* get the device */
    if ((dvp = GetDeviceProc(devicename, NULL)) == NULL)
        return DOSFALSE;

    /* we're only interested in real devices */
    if (dvp->dvp_DevNode == NULL ||
        dvp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }

    ret = dopacket1(DOSBase, NULL, dvp->dvp_Port, ACTION_MORE_CACHE, numbuffers);

    FreeDeviceProc(dvp);

    return ret;

    AROS_LIBFUNC_EXIT
} /* AddBuffers */
