/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2015 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define USE_INLINE_STDARG

#include <exec/types.h>
#include <exec/errors.h>

#include <devices/newstyle.h>
#include <devices/trackdisk.h>

#include <proto/exec.h>

#include <string.h>
#include <stdio.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

/* TD64 commands */
#ifndef TD_READ64
#define TD_READ64  24
#define TD_WRITE64 25
#endif

void ProcessDiskChange(struct Globals *glob)
{
    D(bug("\nGot disk change request\n"));

    if (glob->disk_inhibited != 0)
    {
        D(bug("Disk is inhibited, ignoring disk change\n"));
        return;
    }

    glob->diskioreq->iotd_Req.io_Command = TD_CHANGESTATE;
    glob->diskioreq->iotd_Req.io_Data = NULL;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Flags = IOF_QUICK;
    DoIO((struct IORequest *)glob->diskioreq);

    if (glob->diskioreq->iotd_Req.io_Error == 0
        && glob->diskioreq->iotd_Req.io_Actual == 0)
    {
        /* Disk has been inserted. */
        D(bug("\tDisk has been inserted\n"));
        glob->disk_inserted = TRUE;
        DoDiskInsert(glob);
    }
    else
    {
        /* Disk has been removed. */
        D(bug("\tDisk has been removed\n"));
        glob->disk_inserted = FALSE;
        DoDiskRemove(glob);
    }

    D(bug("Done\n"));
}

void UpdateDisk(struct Globals *glob)
{
    if (glob->sb)
        Cache_Flush(glob->sb->cache);

    glob->diskioreq->iotd_Req.io_Command = CMD_UPDATE;
    DoIO((struct IORequest *)glob->diskioreq);

    /* Turn off motor (where applicable) if nothing has happened during the
     * last timer period */
    if (!glob->restart_timer)
    {
        D(bug("Stopping drive motor\n"));
        glob->diskioreq->iotd_Req.io_Command = TD_MOTOR;
        glob->diskioreq->iotd_Req.io_Length = 0;
        DoIO((struct IORequest *)glob->diskioreq);
    }
}

/* Probe the device to determine 64-bit support */
void Probe64BitSupport(struct Globals *glob)
{
    struct NSDeviceQueryResult nsd_query;
    UWORD *nsd_cmd;

    glob->readcmd = CMD_READ;
    glob->writecmd = CMD_WRITE;

    /* Probe TD64 */
    glob->diskioreq->iotd_Req.io_Command = TD_READ64;
    glob->diskioreq->iotd_Req.io_Offset = 0;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Actual = 0;
    glob->diskioreq->iotd_Req.io_Data = 0;

    if (DoIO((struct IORequest *)glob->diskioreq) != IOERR_NOCMD)
    {
        D(bug("Probe_64bit_support:"
            " device supports 64-bit trackdisk extensions\n"));
        glob->readcmd = TD_READ64;
        glob->writecmd = TD_WRITE64;
    }

    /* Probe NSD */
    glob->diskioreq->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
    glob->diskioreq->iotd_Req.io_Length =
        sizeof(struct NSDeviceQueryResult);
    glob->diskioreq->iotd_Req.io_Data = (APTR) &nsd_query;

    if (DoIO((struct IORequest *)glob->diskioreq) == 0)
        for (nsd_cmd = nsd_query.SupportedCommands; *nsd_cmd != 0;
            nsd_cmd++)
        {
            if (*nsd_cmd == NSCMD_TD_READ64)
            {
                D(bug("Probe_64bit_support:"
                    " device supports NSD 64-bit trackdisk extensions\n"));
                glob->readcmd = NSCMD_TD_READ64;
                glob->writecmd = NSCMD_TD_WRITE64;
                break;
            }
        }
}

/* N.B. returns an Exec error code, not a DOS error code! */
LONG AccessDisk(BOOL do_write, ULONG num, ULONG nblocks, ULONG block_size,
    UBYTE *data, APTR priv)
{
    struct Globals *glob = priv;
    UQUAD off;
    ULONG err;
    ULONG start, end;
    BOOL retry = TRUE;
    TEXT vol_name[100];

#if DEBUG_CACHESTATS > 1
    ErrorMessage("Accessing %lu sector(s) starting at %lu.\n"
        "First volume sector is %lu, sector size is %lu.\n", "OK", nblocks,
         num, glob->sb->first_device_sector, block_size);
#endif

    /* Adjust parameters if range is partially outside boundaries, or
     * warn user and bale out if completely outside boundaries */
    if (glob->sb)
    {
        start = glob->sb->first_device_sector;
        if (num + nblocks <= glob->sb->first_device_sector)
        {
            if (num != glob->last_num)
            {
                glob->last_num = num;
                ErrorMessage("A handler attempted to %s %lu sector(s)\n"
                    "starting from %lu, before the actual volume space.\n"
                    "First volume sector is %lu, sector size is %lu.\n"
                    "Either your disk is damaged or it is a bug in\n"
                    "the handler. Please check your disk and/or\n"
                    "report this problem to the developers team.", "OK",
                    (IPTR) (do_write ? "write" : "read"), nblocks, num,
                    glob->sb->first_device_sector, block_size);
            }
            return IOERR_BADADDRESS;
        }
        else if (num < start)
        {
            nblocks -= start - num;
            data += (start - num) * block_size;
            num = start;
        }

        end = glob->sb->first_device_sector + glob->sb->total_sectors;
        if (num >= end)
        {
            if (num != glob->last_num)
            {
                glob->last_num = num;
                ErrorMessage("A handler attempted to %s %lu sector(s)\n"
                    "starting from %lu, beyond the actual volume space.\n"
                    "Last volume sector is %lu, sector size is %lu.\n"
                    "Either your disk is damaged or it is a bug in\n"
                    "the handler. Please check your disk and/or\n"
                    "report this problem to the developers team.", "OK",
                    (IPTR) (do_write ? "write" : "read"), nblocks, num,
                    end - 1, block_size);
            }
            return IOERR_BADADDRESS;
        }
        else if (num + nblocks > end)
            nblocks = end - num;
    }

    off = ((UQUAD) num) * block_size;

    while (retry)
    {
        glob->diskioreq->iotd_Req.io_Offset = off & 0xFFFFFFFF;
        glob->diskioreq->iotd_Req.io_Actual = off >> 32;

        glob->diskioreq->iotd_Req.io_Length = nblocks * block_size;
        glob->diskioreq->iotd_Req.io_Data = data;
        glob->diskioreq->iotd_Req.io_Command =
            do_write ? glob->writecmd : glob->readcmd;

        err = DoIO((struct IORequest *)glob->diskioreq);

        if (err != 0)
        {
            if (glob->sb && glob->sb->volume.name[0] != '\0')
                snprintf(vol_name, 100, "Volume %s",
                    glob->sb->volume.name + 1);
            else
                snprintf(vol_name, 100, "Device %s",
                    AROS_BSTR_ADDR(glob->devnode->dol_Name));

            if (nblocks > 1)
                retry = ErrorMessage("%s\nhas a %s error\n"
                    "in the block range\n%lu to %lu",
                    "Retry|Cancel", (IPTR)vol_name,
                    (IPTR)(do_write ? "write" : "read"), num,
                    num + nblocks - 1);
            else
                retry = ErrorMessage("%s\nhas a %s error\n"
                    "on block %lu",
                    "Retry|Cancel", (IPTR)vol_name,
                    (IPTR)(do_write ? "write" : "read"), num);
        }
        else
            retry = FALSE;
    }

    return err;
}
