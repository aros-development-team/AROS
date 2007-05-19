/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
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
#include <devices/trackdisk.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>     /* for memset() */

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_MISC
#include <aros/debug.h>

#ifndef ID_BUSY
#define ID_BUSY 0x42555359
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

        id->id_DiskType = (glob->sb->type == 12) ? ID_FAT12_DISK :
                          (glob->sb->type == 16) ? ID_FAT16_DISK :
                          (glob->sb->type == 32) ? ID_FAT32_DISK :
                                                   ID_FAT12_DISK;

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

    if (glob->sb == NULL && (sb = AllocVecPooled(glob->mempool, sizeof(struct FSSuper)))) {
        memset(sb, 0, sizeof(struct FSSuper));

        if (ReadFATSuper(sb) == 0) {
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
                    sb->doslist->dol_Name = (BSTR)MKBADDR(&sb->volume.name);

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
        }

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
