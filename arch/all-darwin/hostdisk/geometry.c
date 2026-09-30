/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/* This routine differs in different UNIX variants (using different IOCTLs) */

#include <aros/debug.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <proto/hostlib.h>

/*
 * <sys/disk.h> pulls in <net/if.h>, which only declares the types it needs
 * when __APPLE__ is defined, and the AROS compiler doesn't define it. Take
 * just the two ioctls used here, as defined in <sys/disk.h>.
 */
#include <stdint.h>
#include <sys/ioccom.h>

#define DKIOCGETBLOCKSIZE   _IOR('d', 24, uint32_t)
#define DKIOCGETBLOCKCOUNT  _IOR('d', 25, uint64_t)

#include "hostdisk_host.h"
#include "hostdisk_device.h"

ULONG Host_DeviceGeometry(int file, struct DriveGeometry *dg, struct HostDiskBase *hdskBase)
{
    UQUAD sectors = 0;
    int ret, err;

    HostLib_Lock();
 
    ret = HOST_VACALL3(hdskBase->iface->ioctl, file, DKIOCGETBLOCKSIZE, &dg->dg_SectorSize);

    if (ret != -1)
        ret = HOST_VACALL3(hdskBase->iface->ioctl, file, DKIOCGETBLOCKCOUNT, &sectors);

    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
    {
        D(bug("hostdisk: Error %d\n", err));

        return err;
    }

    /* The block count is 64-bit; dg_TotalSectors is 32-bit. */
    dg->dg_TotalSectors = sectors > 0xFFFFFFFF ? 0xFFFFFFFF : (ULONG)sectors;

    D(bug("hostdisk: %u sectors per %u bytes\n", dg->dg_TotalSectors, dg->dg_SectorSize));

    /*
     * This is all we can do on Darwin. They dropped CHS completely,
     * so we stay with LBA (CylSectors == 1)
     */
    dg->dg_Cylinders = dg->dg_TotalSectors;

    return 0;
}
