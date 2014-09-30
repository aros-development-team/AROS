/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This file is just a dummy nonfunctional template. These functions
 * need host-specific implementations.
 */

#include <devices/trackdisk.h>
#include <exec/errors.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

ULONG Host_Open(struct unit *Unit)
{
    return IOERR_NOCMD;
}

void Host_Close(struct unit *Unit)
{

}

LONG Host_Read(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    *ioerr = IOERR_NOCMD;
    return -1;
}

LONG Host_Write(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    *ioerr = IOERR_NOCMD;
    return -1;
}

ULONG Host_Seek(struct unit *Unit, ULONG pos)
{
    return IOERR_NOCMD;
}

ULONG Host_Seek64(struct unit *Unit, ULONG pos, ULONG pos_hi)
{
    return IOERR_NOCMD;
}

ULONG Host_GetGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    return IOERR_NOCMD;
}

int Host_ProbeGeometry(struct HostDiskBase *hdskBase, char *name, struct DriveGeometry *dg)
{
    return -1;
}
