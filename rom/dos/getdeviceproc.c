/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: GetDeviceProc() - Find the filesystem for a path.
*/

#include <aros/debug.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(struct DevProc *, GetDeviceProc,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(struct DevProc *, dp, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 107, Dos)

/*  FUNCTION
        GetDeviceProc() will search for the filesystem handler which
        you should send a command to for a specific path.

        By calling GetDeviceProc() multiple times, the caller will
        be able to handle multi-assign paths.

        The first call to GetDeviceProc() should have the |dp| parameter
        as NULL.

    INPUTS
        name - Name of the object to find.
        dp   - Previous result of GetDeviceProc() or NULL.

    RESULT
        A pointer to a DevProc structure containing the information
        required to send a command to a filesystem.

    NOTES

    EXAMPLE

    BUGS
        Currently doesn't return dvp_DevNode for locks which are
        relative to "PROGDIR:", ":", or the current directory.

    SEE ALSO
        FreeDeviceProc()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return GetDeviceProcRelative(BNULL, name, dp);

    AROS_LIBFUNC_EXIT
} /* GetDeviceProc */
