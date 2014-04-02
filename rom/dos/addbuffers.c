/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
        Add or remove cache memory to/from a filesystem. The amount of memory
        per cache buffer and the limit depends on the filesystem.

    INPUTS
        devicename - DOS device name (with trailing ':' and NUL terminated).
        numbuffers - Number of buffers to add. May be negative for decreasing.

    RESULT
        DOSTRUE on success (IoErr() gives the actual number of buffers).
        DOSFALSE on error (IoErr() gives the error code).

    NOTES
        Although some old filesystems return the new buffer count instead of
        a success indication, a work-around for that case is built into the
        AROS implementation of this function.

    EXAMPLE
        LONG res1, res2;
        res1 = AddBuffers("DF0:", 10);
        res2 = IoErr();

    BUGS

    SEE ALSO
        IoErr()

    INTERNALS

*****************************************************************************/

{
    AROS_LIBFUNC_INIT
    
    struct DevProc *dvp;
    LONG ret, error;

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

    if (ret > 0)
    {
        error = ret;
        ret = DOSTRUE;
    }
    else
        error = IoErr();

    FreeDeviceProc(dvp);

    SetIoErr(error);

    return ret;

    AROS_LIBFUNC_EXIT
} /* AddBuffers */
