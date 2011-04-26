/* This routine differs in different UNIX variants (using different IOCTLs) */

#include <devices/trackdisk.h>
#include <exec/errors.h>

#include "hostdisk_host.h"

ULONG Host_DeviceGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    /* Not implemented for generic UNIX */
    return IOERR_NOCMD;
}
