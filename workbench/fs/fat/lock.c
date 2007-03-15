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

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define sb glob->sb

LONG TryLockObj(struct ExtFileLock *fl, UBYTE *name, LONG namelen, LONG access, BPTR *result) {
    LONG err = ERROR_OBJECT_NOT_FOUND;
    ULONG cluster, entry, start_cluster;

    if (fl && (fl->attr & ATTR_DIRECTORY) == 0)
        return ERROR_OBJECT_WRONG_TYPE;

    start_cluster = (fl) ? fl->first_cluster : 0;
    
    kprintf("\tSearching for: "); knprints(name, namelen);
    SkipColon(name, namelen);

    err = FindEntryByPath(start_cluster, name, namelen, &cluster, &entry);

    if (err == 0) {
        if (entry != FAT_ROOTDIR_MARK)
            err = LockFile(entry, cluster, access, result);
        else
            err = LockRoot(access, result);
    }

    return err;
}

LONG LockFile(ULONG entry, ULONG cluster, LONG axs, BPTR *res) {
    struct ExtFileLock *fl;

    kprintf("\tLockFile entry %ld cluster %ld\n", entry, cluster);

    if ((fl = FS_AllocMem(sizeof(struct ExtFileLock)))) {
        struct Extent ext;
        struct DirCache dc;
        struct DirEntry *de;

        SetupDirCache(sb, &dc, &ext, cluster);
        GetDirCacheEntry(sb, &dc, entry, &de);

        fl->fl_Access = axs;
        fl->fl_Task = glob->ourport;
        fl->fl_Volume = MKBADDR(sb->doslist);
        fl->fl_Link = sb->doslist->dol_misc.dol_volume.dol_LockList;
        fl->magic = ID_FAT_DISK;

        sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

        fl->entry = entry;
        fl->cluster = cluster;
        fl->attr = de->attr | ATTR_REALENTRY;
        fl->first_cluster = GetFirstCluster(de);
        fl->size = AROS_LE2LONG(de->file_size);

        GetShortName(de, &fl->name[1], &fl->name[0]);
        GetLongName(sb, &dc, de, entry, &fl->name[1], &fl->name[0]); /* replaces short name only if long name has been found */

        fl->dircache_active = FALSE;

        FreeDirCache(sb, &dc);

        *res = MKBADDR(fl);
        return 0;
    }

    return ERROR_NO_FREE_STORE;
}

LONG LockRoot(LONG axs, BPTR *res) {
    struct ExtFileLock *fl;

    kprintf("\tLockRoot()\n");

    if ((fl = FS_AllocMem(sizeof(struct ExtFileLock)))) {
        fl->fl_Access = axs;
        fl->fl_Task = glob->ourport;
        fl->fl_Volume = MKBADDR(sb->doslist);
        fl->fl_Link = sb->doslist->dol_misc.dol_volume.dol_LockList;
        fl->magic = ID_FAT_DISK;

        sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

        fl->entry = FAT_ROOTDIR_MARK;
        fl->cluster = FAT_ROOTDIR_MARK;
        fl->attr = ATTR_DIRECTORY | ATTR_ROOTDIR;
        fl->first_cluster = 0;
        fl->size = 0;

        memcpy(fl->name, sb->volume.name, 32);

        fl->dircache_active = FALSE;

        *res = MKBADDR(fl);
        return 0;
    }

    return ERROR_NO_FREE_STORE;
}

LONG CopyLock(struct ExtFileLock *src_fl, BPTR *res) {
    struct ExtFileLock *fl;

    if (src_fl->fl_Access == EXCLUSIVE_LOCK)
        return ERROR_OBJECT_IN_USE;

    if ((fl = FS_AllocMem(sizeof(struct ExtFileLock)))) {
        fl->fl_Access = src_fl->fl_Access;
        fl->fl_Task = glob->ourport;
        fl->fl_Volume = MKBADDR(sb->doslist);
        fl->fl_Link = sb->doslist->dol_misc.dol_volume.dol_LockList;
        fl->magic = ID_FAT_DISK;

        sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

        fl->entry = src_fl->entry;
        fl->cluster = src_fl->cluster;
        fl->attr = src_fl->attr;
        fl->first_cluster = src_fl->first_cluster;
        fl->size = src_fl->size;

        memcpy(fl->name, src_fl->name, 108);
        fl->dircache_active = FALSE;

        *res = MKBADDR(fl);
        return 0;
    }

    return ERROR_NO_FREE_STORE;
}

LONG LockParent(struct ExtFileLock *fl, LONG axs, BPTR *res) {
    LONG err;
    struct Extent ext;
    struct DirCache dc;
    struct DirEntry *de;

    if (fl->cluster == 0) /* for entries from root directory */
        return LockRoot(axs, res);

    SetupDirCache(sb, &dc, &ext, fl->cluster);

    if ((err = GetDirCacheEntry(sb, &dc, 1, &de)) == 0) {
        if (strncmp("..         ", de->name, 11) == 0) {
            ULONG cluster, entry;

            cluster = GetFirstCluster(de);     
            
            FreeDirCache(sb, &dc);
            SetupDirCache(sb, &dc, &ext, cluster);

            for (entry=0; entry < 65535; entry++) { /* enries in root directory start from 0 */
                if ((err = GetDirCacheEntry(sb, &dc, entry, &de)) != 0)
                    break;

                if (de->name[0] != 0xE5 && (de->attr & ATTR_VOLUME_ID) == 0 && de->attr & ATTR_DIRECTORY && GetFirstCluster(de) == fl->cluster)
                    break;
            }

            FreeDirCache(sb, &dc);

            if (err == 0)
                err = LockFile(entry, cluster, axs, res);
        }
    }
    else
        FreeDirCache(sb, &dc);

    return err;
}

#undef sb

LONG FreeLockSB(struct ExtFileLock *fl, struct FSSuper *sb) {
    LONG found = FALSE;

    if (sb == NULL)
        return ERROR_OBJECT_NOT_FOUND;
    if (fl->magic != ID_FAT_DISK)
        return ERROR_OBJECT_WRONG_TYPE;

    if (fl == BADDR(sb->doslist->dol_misc.dol_volume.dol_LockList)) {
        sb->doslist->dol_misc.dol_volume.dol_LockList = fl->fl_Link;
        found = TRUE;
    }
    else {
        struct ExtFileLock *prev = NULL, *ptr = BADDR(sb->doslist->dol_misc.dol_volume.dol_LockList);

        while (ptr != NULL) {
            if (ptr == fl) {
                prev->fl_Link = fl->fl_Link;
                found = TRUE;
                break;
            }
            prev = ptr;
            ptr = BADDR(ptr->fl_Link);
        }
    }

    if (found) {
        kprintf("\tFreeing lock.\n");

        fl->fl_Task = NULL;

        if (fl->dircache_active)
            FreeDirCache(sb, fl->dircache);

        FS_FreeMem(fl);

        return 0;
    }

    return ERROR_OBJECT_NOT_FOUND;
}

void FreeLock(struct ExtFileLock *fl) {
    struct FSSuper *ptr = glob->sblist, *prev=NULL;

    if (FreeLockSB(fl, glob->sb) == 0)
        return;

    while (ptr != NULL) {
        if (FreeLockSB(fl, ptr) == 0)
            break;

        prev = ptr;
        ptr = ptr->next;
    }

    if (ptr) {
        if (ptr->doslist->dol_misc.dol_volume.dol_LockList == NULL) { /* check if the device can be removed */
            kprintf("\tRemoving disk completely\n");

            SendVolumePacket(ptr->doslist, ACTION_VOLUME_REMOVE);

            ptr->doslist = NULL;
            FreeFATSuper(ptr);

            if (prev)
                prev->next = ptr->next;
            else
                glob->sblist = ptr->next;

            FS_FreeMem(ptr);
        }
    }
}

