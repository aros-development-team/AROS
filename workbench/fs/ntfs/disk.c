/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#define USE_INLINE_STDARG

#include <exec/types.h>
#include <exec/errors.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/uuid.h>

#include <string.h>     /* for memset() */

#include "ntfs_fs.h"
#include "ntfs_protos.h"
#include "cache.h"
#include "support.h"

#include "debug.h"

#ifndef ID_BUSY
#define ID_BUSY 0x42555359
#endif

/* TD64 commands */
#ifndef TD_READ64
#define TD_READ64  24
#define TD_WRITE64 25
#endif

void FillDiskInfo (struct InfoData *id)
{
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    id->id_NumSoftErrors = 0;
    id->id_UnitNumber = glob->fssm->fssm_Unit;
    id->id_DiskState = ID_VALIDATED;

    if (glob->data) {
        id->id_NumBlocks = glob->data->total_sectors;
        id->id_NumBlocksUsed = glob->data->used_sectors;
        id->id_BytesPerBlock = glob->data->sectorsize;

	id->id_DiskType = ID_NTFS_DISK;

#if defined(NTFS_READONLY)
	 id->id_DiskState = ID_WRITE_PROTECTED;
#endif

        id->id_VolumeNode = MKBADDR(glob->data->doslist);
        id->id_InUse = (IsListEmpty(&glob->data->info->locks)
            && IsListEmpty(&glob->data->info->notifies)) ? DOSFALSE : DOSTRUE;
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

        id->id_VolumeNode = BNULL;
        id->id_InUse = DOSFALSE;
    }
}

void SendVolumePacket(struct DosList *vol, ULONG action)
{
    struct DosPacket *dospacket;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    dospacket = AllocDosObject(DOS_STDPKT, TAG_DONE);
    dospacket->dp_Type = ACTION_DISK_CHANGE;
    dospacket->dp_Arg1 = ID_NTFS_DISK;
    dospacket->dp_Arg2 = (IPTR)vol;
    dospacket->dp_Arg3 = action;
    dospacket->dp_Port = NULL;

    PutMsg(glob->ourport, dospacket->dp_Link);
}

void DoDiskInsert(void)
{
    struct FSData *fs_data;
    ULONG err;
    struct DosList *dl;
    struct VolumeInfo *vol_info = NULL;
    struct GlobalLock *global_lock;
    struct ExtFileLock *ext_lock;
    struct MinNode *lock_node;
    APTR pool;
    struct NotifyNode *nn;
    struct DosList *newvol = NULL;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (glob->data == NULL && (fs_data = _AllocVecPooled(glob->mempool, sizeof(struct FSData))))
    {
        memset(fs_data, 0, sizeof(struct FSData));
	
        err = ReadBootSector(fs_data);
        if (err == 0) {

            /* Scan volume list for a matching volume (would be better to
             * match by serial number) */
            dl = LockDosList(LDF_VOLUMES | LDF_WRITE);
            dl = FindDosEntry(dl, fs_data->volume.name + 1,
               LDF_VOLUMES | LDF_WRITE);
            UnLockDosList(LDF_VOLUMES | LDF_WRITE);

            if (dl != NULL)
	    {
                dl->dol_Task = glob->ourport;
                fs_data->doslist = dl;

                D(bug("[NTFS] %s: Found old volume.\n", __PRETTY_FUNCTION__));

                vol_info = BADDR(dl->dol_misc.dol_volume.dol_LockList);

#if 0 /* no point until we match volumes by serial number */
                /* update name */
#ifdef AROS_FAST_BPTR
                /* ReadBootSector() sets a null byte after the
                 * string, so this should be fine */
                CopyMem(fs_data->volume.name + 1, dl->dol_Name,
                    fs_data->volume.name[0] + 1);
#else
                CopyMem(fs_data->volume.name, dl->dol_Name,
                    fs_data->volume.name[0] + 2);
#endif
#endif

                /* patch locks and notifications to match this handler
                 * instance */
                ForeachNode(&vol_info->locks, global_lock) {
                    ForeachNode(&global_lock->locks, lock_node) {
                        ext_lock = LOCKFROMNODE(lock_node);
                        D(bug("[NTFS] %s: Patching adopted lock %p. old port = %p,"
                            " new port = %p\n", __PRETTY_FUNCTION__, ext_lock, ext_lock->fl_Task,
                            glob->ourport));
                        ext_lock->fl_Task = glob->ourport;
                        ext_lock->data = fs_data;
                        ext_lock->dir->ioh.data = fs_data;
                    }
                }

                ForeachNode(&vol_info->root_lock.locks, lock_node) {
                    ext_lock = LOCKFROMNODE(lock_node);
                    D(bug("[NTFS] %s: Patching adopted ROOT lock %p. old port = %p,"
                        " new port = %p\n", __PRETTY_FUNCTION__, ext_lock, ext_lock->fl_Task,
                        glob->ourport));
                    ext_lock->fl_Task = glob->ourport;
                    ext_lock->data = fs_data;
                    ext_lock->dir->ioh.data = fs_data;
                }

                ForeachNode(&vol_info->notifies, nn)
                    nn->nr->nr_Handler = glob->ourport;
            }
            else
	    {
                D(bug("[NTFS] %s: Creating new volume.\n", __PRETTY_FUNCTION__));

                /* create transferable core volume info */
                pool = CreatePool(MEMF_PUBLIC, DEF_POOL_SIZE, DEF_POOL_THRESHOLD);
                if (pool != NULL) {
                    vol_info = _AllocVecPooled(pool, sizeof(struct VolumeInfo));
                    if (vol_info != NULL) {
                        vol_info->mem_pool = pool;
			UUID_Copy((const uuid_t *)&fs_data->uuid, (uuid_t *)&vol_info->uuid);
                        NEWLIST(&vol_info->locks);
                        NEWLIST(&vol_info->notifies);

                        vol_info->root_lock.dir_cluster = FILE_ROOT * fs_data->mft_size;
                        vol_info->root_lock.dir_entry = -1;
                        vol_info->root_lock.access = SHARED_LOCK;
                        vol_info->root_lock.first_cluster = FILE_ROOT * fs_data->mft_size;
                        vol_info->root_lock.attr = ATTR_DIRECTORY;
                        vol_info->root_lock.size = 0;
                        CopyMem(fs_data->volume.name, vol_info->root_lock.name,
                            fs_data->volume.name[0] + 1);
                        NEWLIST(&vol_info->root_lock.locks);
                    }

                    if ((newvol = _AllocVecPooled(pool, sizeof(struct DosList)))) {
                        newvol->dol_Next = BNULL;
                        newvol->dol_Type = DLT_VOLUME;
                        newvol->dol_Task = glob->ourport;
                        newvol->dol_Lock = BNULL;

                        CopyMem(&fs_data->volume.create_time, &newvol->dol_misc.dol_volume.dol_VolumeDate, sizeof(struct DateStamp));

                        newvol->dol_misc.dol_volume.dol_LockList = MKBADDR(&vol_info->locks);

                        newvol->dol_misc.dol_volume.dol_DiskType = ID_NTFS_DISK;

                        if ((newvol->dol_Name = MKBADDR(_AllocVecPooled(pool, fs_data->volume.name[0] + 2)))) {
#ifdef AROS_FAST_BPTR
                            /* ReadBootSector() sets a null byte after the string, so this should be fine */
                            CopyMem(fs_data->volume.name + 1, newvol->dol_Name,
                                fs_data->volume.name[0] + 1);
#else
                            CopyMem(fs_data->volume.name, BADDR(newvol->dol_Name),
                                fs_data->volume.name[0] + 2);
#endif

                            fs_data->doslist = newvol;
                        }
                    }
                    if (vol_info == NULL || newvol == NULL)
                        DeletePool(pool);
                }
            }

            fs_data->info = vol_info;
            glob->data = fs_data;
            glob->last_num = -1;

            if (dl != NULL)
	    {
                SendEvent(IECLASS_DISKINSERTED);
	    }
            else
                SendVolumePacket(newvol, ACTION_VOLUME_ADD);

            D(bug("[NTFS] %s: Disk successfully initialised\n", __PRETTY_FUNCTION__));

            return;
        }
        else if (err == IOERR_BADADDRESS)
	{
	    ErrorMessageArgs("Your device does not support 64-bit\n"
			 "access to the disk while it is needed!\n"
			 "In order to prevent data damage access to\n"
			 "this disk was blocked.\n"
			 "Please upgrade your device driver.", NULL);
	}

        _FreeVecPooled(glob->mempool, fs_data);
    }

    SendEvent(IECLASS_DISKINSERTED);

    return;
}

BOOL AttemptDestroyVolume(struct FSData *fs_data)
{
    BOOL destroyed = FALSE;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    /* check if the volume can be removed */
    if (IsListEmpty(&fs_data->info->locks) && IsListEmpty(&fs_data->info->notifies)) {
        D(bug("[NTFS] %s: Removing volume completely\n", __PRETTY_FUNCTION__));

        if (fs_data == glob->data)
            glob->data = NULL;
        else
            Remove((struct Node *)fs_data);

        SendVolumePacket(fs_data->doslist, ACTION_VOLUME_REMOVE);

        FreeBootSector(fs_data);
        _FreeVecPooled(glob->mempool, fs_data);
        destroyed = TRUE;
    }

    return destroyed;
}

void DoDiskRemove(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (glob->data) {
        struct FSData *fs_data = glob->data;

        if(!AttemptDestroyVolume(fs_data)) {
            fs_data->doslist->dol_Task = NULL;
            glob->data = NULL;
            D(bug("\tMoved in-memory super block to spare list. "
                "Waiting for locks and notifications to be freed\n"));
            AddTail((struct List *)&glob->sblist, (struct Node *)fs_data);
            SendEvent(IECLASS_DISKREMOVED);
        }
    }
}
 
void ProcessDiskChange(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (glob->disk_inhibited) {
        D(bug("[NTFS] %s: Disk is inhibited, ignoring disk change\n", __PRETTY_FUNCTION__));
        return;
    }

    glob->diskioreq->iotd_Req.io_Command = TD_CHANGESTATE;
    glob->diskioreq->iotd_Req.io_Data = NULL;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Flags = IOF_QUICK;
    DoIO((struct IORequest*) glob->diskioreq);

    if (glob->diskioreq->iotd_Req.io_Error == 0 && glob->diskioreq->iotd_Req.io_Actual == 0) {
        /* Disk has been inserted. */
        D(bug("[NTFS] %s: Disk INSERTED\n", __PRETTY_FUNCTION__));
        glob->disk_inserted = TRUE;
        DoDiskInsert();
    }
    else {
        /* Disk has been removed. */
        D(bug("[NTFS] %s: Disk REMOVED\n", __PRETTY_FUNCTION__));
        glob->disk_inserted = FALSE;
        DoDiskRemove();
    }

    D(bug("[NTFS] %s: Done\n", __PRETTY_FUNCTION__));
}

void UpdateDisk(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (glob->data)
    {
        Cache_Flush(glob->data->cache);
    }

    glob->diskioreq->iotd_Req.io_Command = CMD_UPDATE;
    DoIO((struct IORequest *)glob->diskioreq);

    /* Turn off motor (where applicable) if nothing has happened during the
     * last timer period */
    if (!glob->restart_timer) {
    D(bug("[NTFS] %s: Stopping drive motor\n", __PRETTY_FUNCTION__));
        glob->diskioreq->iotd_Req.io_Command = TD_MOTOR;
        glob->diskioreq->iotd_Req.io_Length = 0;
        DoIO((struct IORequest *)glob->diskioreq);
    }
}

/* probe the device to determine 64-bit support */
void Probe_64bit_support(void)
{
    struct NSDeviceQueryResult nsd_query;
    UWORD *nsd_cmd;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    glob->readcmd = CMD_READ;
    glob->writecmd = CMD_WRITE;

    /* probe TD64 */
    glob->diskioreq->iotd_Req.io_Command = TD_READ64;
    glob->diskioreq->iotd_Req.io_Offset = 0;
    glob->diskioreq->iotd_Req.io_Length = 0;
    glob->diskioreq->iotd_Req.io_Actual = 0;
    glob->diskioreq->iotd_Req.io_Data = 0;

    if (DoIO((struct IORequest *) glob->diskioreq) != IOERR_NOCMD) {
	D(bug("[NTFS] %s: 64-bit trackdisk extensions supported\n", __PRETTY_FUNCTION__));
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
		D(bug("[NTFS] %s: NSD 64-bit trackdisk extensions supported\n", __PRETTY_FUNCTION__));
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
    ULONG start, end;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

#if DEBUG_CACHESTATS > 1
    ErrorMessage("Accessing %lu sector(s) starting at %lu.\n"
        "First volume sector is %lu, sector size is %lu.\n", nblocks, num,
	glob->data->first_device_sector, block_size);
#endif

    /* Adjust parameters if range is partially outside boundaries, or
     * warn user and bale out if completely outside boundaries */
    if (glob->data) {
        start = glob->data->first_device_sector;
	if (num + nblocks <= glob->data->first_device_sector) {
	    if (num != glob->last_num) {
		glob->last_num = num;
	        ErrorMessage("A handler attempted to %s %lu sector(s) starting\n"
		    	     "from %lu, before the actual volume space.\n"
		    	     "First volume sector is %lu, sector size is %lu.\n"
		    	     "Either your disk is damaged or it is a bug in\n"
		    	     "the handler. Please check your disk and/or\n"
		    	     "report this problem to the developers team.",
		    	     (IPTR)(do_write ? "write" : "read"), nblocks, num,
		    	     glob->data->first_device_sector, block_size);
	    }
	    return IOERR_BADADDRESS;
	}
        else if (num < start) {
            nblocks -= start - num;
            data += (start - num) * block_size;
            num = start;
        }

	end = glob->data->first_device_sector + glob->data->total_sectors;
	if (num >= end) {
	    if (num != glob->last_num) {
		glob->last_num = num;
	        ErrorMessage("A handler attempted to %s %lu sector(s) starting\n"
		    	     "from %lu, beyond the actual volume space.\n"
		    	     "Last volume sector is %lu, sector size is %lu.\n"
		    	     "Either your disk is damaged or it is a bug in\n"
		    	     "the handler. Please check your disk and/or\n"
		    	     "report this problem to the developers team.",
		    	     (IPTR)(do_write ? "write" : "read"), nblocks, num,
		    	     end - 1, block_size);
	    }
	    return IOERR_BADADDRESS;
	}
        else if (num + nblocks > end)
            nblocks = end - num;
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

    return err;
}
