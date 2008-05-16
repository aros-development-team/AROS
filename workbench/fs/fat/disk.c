/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2008 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define USE_INLINE_STDARG

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/errors.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>     /* for memset() */

#include "fat_fs.h"
#include "fat_protos.h"
#include "timer.h"

#define DEBUG DEBUG_MISC
#include "debug.h"

#ifndef ID_BUSY
#define ID_BUSY 0x42555359
#endif

/* TD64 commands */
#ifndef TD_READ64
#define TD_READ64  24
#define TD_WRITE64 25
#endif

void FillDiskInfo (struct InfoData *id) {
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);

    id->id_NumSoftErrors = 0;
    id->id_UnitNumber = glob->fssm->fssm_Unit;
    id->id_DiskState = ID_VALIDATED;

    if (glob->sb) {
        CountFreeClusters(glob->sb);

        id->id_NumBlocks = glob->sb->total_sectors;
        id->id_NumBlocksUsed = glob->sb->total_sectors - ((glob->sb->free_clusters * glob->sb->clustersize) >> glob->sb->sectorsize_bits);
        id->id_BytesPerBlock = glob->sb->sectorsize;

	id->id_DiskType = ID_DOS_DISK;

        id->id_VolumeNode = MKBADDR(glob->sb->doslist);
        id->id_InUse = glob->sb->doslist->dol_misc.dol_volume.dol_LockList ? DOSTRUE : DOSFALSE;
    }

    else {
        id->id_NumBlocks = de->de_Surfaces * de->de_BlocksPerTrack * (de->de_HighCyl+1 - de->de_LowCyl) / de->de_SectorPerBlock;
        id->id_NumBlocksUsed = id->id_NumBlocks;
        id->id_BytesPerBlock = de->de_SizeBlock << 2;

        id->id_DiskState = ID_VALIDATED;

        if (glob->disk_inhibited)
                id->id_DiskType = ID_BUSY;
        else if (glob->disk_inserted)
                id->id_DiskType = ID_NOT_REALLY_DOS; //ID_UNREADABLE_DISK;
        else
                id->id_DiskType = ID_NO_DISK_PRESENT;

        id->id_VolumeNode = NULL;
        id->id_InUse = DOSFALSE;
    }
}

void SendVolumePacket(struct DosList *vol, ULONG action) {
    struct DosPacket *dospacket;

    dospacket = AllocDosObject(DOS_STDPKT, TAG_DONE);
    dospacket->dp_Type = ACTION_DISK_CHANGE;
    dospacket->dp_Arg1 = ID_FAT_DISK;
    dospacket->dp_Arg2 = (ULONG) vol;
    dospacket->dp_Arg3 = action;
    dospacket->dp_Port = NULL;

    PutMsg(glob->ourport, dospacket->dp_Link);
}

void DoDiskInsert(void) {
    struct FSSuper *sb;
    ULONG err;

    if (glob->sb == NULL && (sb = AllocVecPooled(glob->mempool, sizeof(struct FSSuper)))) {
        memset(sb, 0, sizeof(struct FSSuper));

	err = ReadFATSuper(sb);
	if (err == 0) {
            LONG found = FALSE;

            if (glob->sblist) {
                struct FSSuper *ptr = glob->sblist, *prev = NULL;

                while (ptr != NULL) {
                    if (CompareFATSuper(sb, ptr) == 0)
                        break;

                    prev = ptr;
                    ptr = ptr->next;
                }

                if (ptr) {
                    D(bug("\tFound FAT FS Super Block in spare list, freeing obsolete old one\n"));

                    sb->doslist = ptr->doslist;
                    ptr->doslist = NULL;

#ifdef AROS_FAST_BPTR
                    /* ReadFATSuper() sets a null byte * after the string, so
                     * this should be fine */
                    sb->doslist->dol_Name = (BSTR)MKBADDR(&(sb->volume.name[1]));
#else
                    sb->doslist->dol_Name = (BSTR)MKBADDR(&sb->volume.name);
#endif

                    if (prev)
                        prev->next = ptr->next;
                    else
                        glob->sblist = ptr->next;

                    FreeFATSuper(ptr);
                    FreeVecPooled(glob->mempool, ptr);

                    found = TRUE;
                }
            }

            if (!found) {
                struct DosList *newvol;

                D(bug("\tCreating new volume.\n"));

                if ((newvol = AllocVecPooled(glob->mempool, sizeof(struct DosList)))) {
                    newvol->dol_Next = NULL;
                    newvol->dol_Type = DLT_VOLUME;
                    newvol->dol_Task = glob->ourport;
                    newvol->dol_Lock = NULL;

                    CopyMem(&sb->volume.create_time, &newvol->dol_misc.dol_volume.dol_VolumeDate, sizeof(struct DateStamp));

                    newvol->dol_misc.dol_volume.dol_LockList = NULL;

                    newvol->dol_misc.dol_volume.dol_DiskType = (sb->type == 12) ? ID_FAT12_DISK :
                                                               (sb->type == 16) ? ID_FAT16_DISK :
                                                               (sb->type == 32) ? ID_FAT32_DISK :
                                                               ID_FAT12_DISK;

#ifdef AROS_FAST_BPTR
                    /* ReadFATSuper() sets a null byte * after the string, so
                     * this should be fine */
                    newvol->dol_Name = (BSTR)MKBADDR(&(sb->volume.name[1]));
#else
                    newvol->dol_Name = (BSTR)MKBADDR(&sb->volume.name);
#endif

                    sb->doslist = newvol;

                    SendVolumePacket(newvol, ACTION_VOLUME_ADD);
                }
            }

            else
                SendEvent(IECLASS_DISKINSERTED);

            glob->sb = sb;

            D(bug("\tDisk successfully initialised\n"));

            return;
	} else if (err == IOERR_BADADDRESS)
	    ErrorMessage("Your device does not support 64-bit\n"
			 "access to the disk while it is needed!\n"
			 "In order to prevent data damage access to\n"
			 "this disk was blocked.\n"
			 "Please upgrade your device driver.");

        FreeVecPooled(glob->mempool, sb);
    }

    SendEvent(IECLASS_DISKINSERTED);       

    return;
}

void DoDiskRemove(void) {

    if (glob->sb) {
        struct FSSuper *sb = glob->sb;

        glob->sb = NULL;

        if (sb->doslist->dol_misc.dol_volume.dol_LockList == NULL) { /* check if the device can be removed */
            D(bug("\tRemoving disk completely\n"));

            SendVolumePacket(sb->doslist, ACTION_VOLUME_REMOVE);

            sb->doslist = NULL;
            FreeFATSuper(sb);
            FreeVecPooled(glob->mempool, sb);
        }
        else {
            sb->next = glob->sblist;
            glob->sblist = sb;

            D(bug("\tMoved in-memory super block to spare list. Waiting for locks to be freed\n"));

            SendEvent(IECLASS_DISKREMOVED);
        }
    }
}
 
void ProcessDiskChange(void) {
    D(bug("\nGot disk change request\n"));
    
    if (glob->disk_inhibited) {
        D(bug("Disk is inhibited, ignoring disk change\n"));
        return;
    }

    glob->diskioreq->iotd_Req.io_Command = TD_CHANGESTATE;
    glob->diskioreq->iotd_Req.io_Data = NULL;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Flags = IOF_QUICK;
    DoIO((struct IORequest*) glob->diskioreq);

    if (glob->diskioreq->iotd_Req.io_Error == 0 && glob->diskioreq->iotd_Req.io_Actual == 0) {
        /* Disk has been inserted. */
        D(bug("\tDisk has been inserted\n"));
        glob->disk_inserted = TRUE;
        DoDiskInsert();
    }
    else {
        /* Disk has been removed. */
        D(bug("\tDisk has been removed\n"));
        glob->disk_inserted = FALSE;
        DoDiskRemove();
    }

    D(bug("Done\n"));
}

void UpdateDisk(void)
{
	struct cache *c;

	c = glob->sb->cache;
	if (c && (c->flags & CACHE_WRITEBACK))
		cache_flush(c);

	glob->diskioreq->iotd_Req.io_Command = CMD_UPDATE;
	DoIO((struct IORequest *)glob->diskioreq);

	glob->diskioreq->iotd_Req.io_Command = TD_MOTOR;
	glob->diskioreq->iotd_Req.io_Length = 0;
	DoIO((struct IORequest *)glob->diskioreq);
}

/* probe the device to determine 64-bit support */
void Probe_64bit_support(void) {
    struct NSDeviceQueryResult nsd_query;
    UWORD *nsd_cmd;

    glob->readcmd = CMD_READ;
    glob->writecmd = CMD_WRITE;

    /* probe TD64 */
    glob->diskioreq->iotd_Req.io_Command = TD_READ64;
    glob->diskioreq->iotd_Req.io_Offset = 0;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Actual = 0;
    glob->diskioreq->iotd_Req.io_Data = 0;

    if (DoIO((struct IORequest *) glob->diskioreq) != IOERR_NOCMD) {
	D(bug("Probe_64bit_support: device supports 64-bit trackdisk extensions\n"));
	glob->readcmd = TD_READ64;
	glob->writecmd = TD_WRITE64;
    }

    /* probe NSD */
    glob->diskioreq->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
    glob->diskioreq->iotd_Req.io_Length = sizeof(struct NSDeviceQueryResult);
    glob->diskioreq->iotd_Req.io_Data = (APTR) &nsd_query;

    if (DoIO((struct IORequest *) glob->diskioreq) == 0)
        for (nsd_cmd = nsd_query.SupportedCommands; *nsd_cmd != 0; nsd_cmd++) {
            if (*nsd_cmd == NSCMD_TD_READ64) {
		D(bug("Probe_64bit_support: device supports NSD 64-bit trackdisk extensions\n"));
		glob->readcmd = NSCMD_TD_READ64;
		glob->writecmd = NSCMD_TD_WRITE64;
                break;
            }
        }
}

ULONG AccessDisk(BOOL do_write, ULONG num, ULONG nblocks, ULONG block_size, UBYTE *data)
{
    UQUAD off;
    ULONG err;
    ULONG end;

    if (glob->sb) {
	if (num < glob->sb->first_device_sector) {
	    if (num != glob->last_num) {
		glob->last_num = num;
	        ErrorMessage("A handler attempted to %s %lu sector(s) starting\n"
		    	     "from %lu before the actual volume space.\n"
		    	     "First volume sector is %lu, sector size is %lu.\n"
		    	     "Either your disk is damaged or it is a bug in\n"
		    	     "the handler. Please check your disk and/or\n"
		    	     "report this problem to the developers team.",
		    	     do_write ? "write" : "read", nblocks, num,
		    	     glob->sb->first_device_sector, block_size);
	    }
	    return IOERR_BADADDRESS;
	}
	end = glob->sb->first_device_sector + glob->sb->total_sectors;
	if (num + nblocks > end) {
	    if (num != glob->last_num) {
		glob->last_num = num;
	        ErrorMessage("A handler attempted to %s %lu sector(s) starting\n"
		    	     "from %lu beyond the actual volume space.\n"
		    	     "Last volume sector is %lu, sector size is %lu.\n"
		    	     "Either your disk is damaged or it is a bug in\n"
		    	     "the handler. Please check your disk and/or\n"
		    	     "report this problem to the developers team.",
		    	     do_write ? "write" : "read", nblocks, num,
		    	     end - 1, block_size);
	    }
	    return IOERR_BADADDRESS;
	}
    }

    off = ((UQUAD) num) * block_size;

    glob->diskioreq->iotd_Req.io_Offset = off & 0xFFFFFFFF;
    glob->diskioreq->iotd_Req.io_Actual = off >> 32;

    if (glob->diskioreq->iotd_Req.io_Actual && (glob->readcmd == CMD_READ))
	return IOERR_BADADDRESS;

    glob->diskioreq->iotd_Req.io_Length = nblocks * block_size;
    glob->diskioreq->iotd_Req.io_Data = data;
    glob->diskioreq->iotd_Req.io_Command = do_write ? glob->writecmd : glob->readcmd;

    err = DoIO((struct IORequest *) glob->diskioreq);
    RestartTimer();

    return err;
}

