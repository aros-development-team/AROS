/*
 * Define struct stat64 on Linux.
 * Define this before anything, since AROS includes
 * may include POSIX includes (for example aros/debug.h
 * includes string.h)
 */
#define _LARGEFILE64_SOURCE

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/trackdisk.h>
#include <proto/hostlib.h>

#include <linux/fs.h>
#include <linux/hdreg.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

ULONG Host_DeviceGeometry(int file, struct DriveGeometry *dg, struct HostDiskBase *hdskBase)
{
    int ret, err;
    struct hd_geometry geo;
    size_t blksize;
    unsigned long devsize;

    D(bug("hostdisk: Host_DeviceGeometry(%s)\n", Unit->filename));

    HostLib_Lock();
 
    ret = hdskBase->iface->ioctl(file, HDIO_GETGEO, &geo);

    if (ret != -1)
	ret = hdskBase->iface->ioctl(file, BLKSSZGET, &blksize);

    if (ret != -1)
	ret = hdskBase->iface->ioctl(file, BLKGETSIZE, &devsize);

    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
    {
    	D(bug("hostdisk: Error %d\n", err));

    	return err;
    }

    D(bug("hostdisk: Block size: %lu\n", blksize));
    D(bug("hostdisk: %lu blocks per 512 bytes\n", devsize));
    D(bug("hostdisk: Disk has %u heads, %u sectors, %u cylinders\n", geo.heads, geo.sectors, geo.cylinders));

    dg->dg_SectorSize   = blksize;
    dg->dg_TotalSectors = devsize / (blksize / 512);
    dg->dg_CylSectors   = geo.heads * geo.sectors;

    if (dg->dg_TotalSectors % dg->dg_CylSectors)
    {
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders  = dg->dg_TotalSectors;

	D(bug("hostdisk: Geometry does not fit, use LBA\n"));
    }
    else
    {
    	dg->dg_Heads        = geo.heads;
    	dg->dg_TrackSectors = geo.sectors;
    	dg->dg_Cylinders    = dg->dg_TotalSectors / dg->dg_CylSectors;

	D(bug("hostdisk: Device size: %u cylinders\n", dg->dg_Cylinders));
    }

    return 0;
}

/*
 * This checks if the system has /dev/hd* entries at all
 * and uses /dev/sd* if not.
 * It is assumed that we have at least /dev/hda.
 */
static int deviceProbe(struct HostDiskBase *hdskBase)
{
    struct stat64 st;
    int res;

    HostLib_Lock();
    res = hdskBase->iface->stat64("/dev/hda", &st);
    HostLib_Unlock();

    D(bug("hostdisk: /dev/hda check result: %d\n", res));
    if (res == -1)
	hdskBase->DiskDevice = "/dev/sd%lc";

    return TRUE;
}

ADD2INITLIB(deviceProbe, 10);
