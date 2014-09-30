/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* This routine differs in different UNIX variants (using different IOCTLs) */

#include <aros/debug.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <proto/hostlib.h>

#include <sys/disk.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

ULONG Host_DeviceGeometry(int file, struct DriveGeometry *dg, struct HostDiskBase *hdskBase)
{
    int ret, err;

    HostLib_Lock();
 
    ret = hdskBase->iface->ioctl(file, DKIOCGETBLOCKSIZE, &dg->dg_SectorSize);

    if (ret != -1)
	ret = hdskBase->iface->ioctl(file, DKIOCGETBLOCKCOUNT, &dg->dg_TotalSectors);

    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
    {
    	D(bug("hostdisk: Error %d\n", err));

    	return err;
    }

    D(bug("hostdisk: %u sectors per %u bytes\n", dg->dg_TotalSectors, dg->dg_SectorSize));

    /*
     * This is all we can do on Darwin. They dropped CHS completely,
     * so we stay with LBA (CylSectors == 1)
     */
    dg->dg_Cylinders = dg->dg_TotalSectors;

    return 0;
}
