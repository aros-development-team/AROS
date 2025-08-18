/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.

    Desc: Check if a device is a filesystem.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/utility.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, IsFileSystemRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR, lock, D1),
        AROS_LHA(CONST_STRPTR, devicename, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 230, Dos)

/*  FUNCTION
        Query the device whether it is a filesystem.

    INPUTS
        lock       - existing lock to compute filename from
        devicename - Name of the device to query.

    RESULT
        TRUE if the device is a filesystem, FALSE otherwise.

    NOTES
        DF0:, HD0:, ... are filesystems.
        CON:, PIPE:, AUX:, ... are not

        In AmigaOS if devicename contains no ":" then result
        is always TRUE. Also volume and assign names return
        TRUE.

        This call is AROS-specific.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG err = ERROR_OBJECT_NOT_FOUND;
    LONG code = DOSFALSE;
    struct DevProc *dvp = NULL;

    /* The Open() aliases '*' and 'CONSOLE:'
     * are never filesystems
     */
    if (Stricmp(devicename, "*") == 0 ||
        Stricmp(devicename, "CONSOLE:") == 0) {
        SetIoErr(err);
        return code;
    }

    /* We can't call GetDeviceProc() on CON: nor RAW:,
     * since that will (implicitly) cause a window to
     * open.
     */
    if (Stricmp(devicename, "CON:") == 0 ||
        Stricmp(devicename, "RAW:") == 0) {
        SetIoErr(err);
        return code;
    }


    if ((dvp = GetDeviceProcRelative(lock, devicename, dvp))) {
        if (dvp->dvp_Port != NULL) // No port? Not a filesystem
            code = dopacket0(DOSBase, NULL, dvp->dvp_Port, ACTION_IS_FILESYSTEM);
        FreeDeviceProc(dvp);
    } else {
        SetIoErr(err);
    }
   
    return code;
    
    AROS_LIBFUNC_EXIT
} /* IsFileSystemRelative */
