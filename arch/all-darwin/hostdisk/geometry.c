/* This routine differs in different UNIX variants (using different IOCTLs) */

#include <aros/debug.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <proto/hostlib.h>

#include <sys/disk.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

ULONG Host_DeviceGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int ret, err;

    D(bug("hostdisk: Host_DeviceGeometry(%s)\n", Unit->filename));

    HostLib_Lock();
 
    ret = hdskBase->iface->ioctl(Unit->file, DKIOCGETBLOCKSIZE, &dg->dg_SectorSize);

    if (ret != -1)
	ret = hdskBase->iface->ioctl(Unit->file, DKIOCGETBLOCKCOUNT, &dg->dg_TotalSectors);

    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
    {
    	D(bug("hostdisk: Error %d\n", err));

    	return err;
    }

    D(bug("hostdisk: %u sectors per %u bytes\n", dg->dg_TotalSectors, dg->dg_SectorSize));

    /*
     * Huh... Darwin claims they dropped CHS completely... But is it so good here ?
     * FIXME: Partitions must end up on cylinder boundary to be processed correctly.
     * But with this approach it's not guaranteed to be so. We could flatten the
     * geometry to 1 head x 1 sector, this would give us one block per cylinder.
     * This would do, but this would overflow ULONG block number.
     */
    dg->dg_Heads        = DEF_HEADS;
    dg->dg_TrackSectors = DEF_TRACK_SECTORS;
    dg->dg_CylSectors   = dg->dg_Heads * dg->dg_TrackSectors;
    dg->dg_Cylinders    = dg->dg_TotalSectors / dg->dg_CylSectors;
    dg->dg_BufMemType   = MEMF_PUBLIC;
    dg->dg_DeviceType   = DG_DIRECT_ACCESS;
    dg->dg_Flags        = 0;

    return 0;
}
