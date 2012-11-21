/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Format a device.
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
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
        devicename      - Name of the device to format.
        volumename      - The name you wish the volume to be called.
        dostype         - The DOS type you wish on the disk.

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

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    if (!getdevpacketinfo(DOSBase, devicename, volumename, &phs))
        return DOSFALSE;
 
    status = dopacket2(DOSBase, NULL, phs.port, ACTION_FORMAT, phs.name, (IPTR)dostype);

    freepacketinfo(DOSBase, &phs);
    
    return status;

    AROS_LIBFUNC_EXIT
} /* Format */
