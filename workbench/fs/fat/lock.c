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

LONG TryLockObj(struct ExtFileLock *fl, UBYTE *name, LONG namelen, LONG access, BPTR *result) {
    LONG err = ERROR_OBJECT_NOT_FOUND;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG dir_cluster;

    if (fl && (fl->attr & ATTR_DIRECTORY) == 0)
        return ERROR_OBJECT_WRONG_TYPE;

    dir_cluster = (fl) ? fl->ioh.first_cluster : 0;
    
    kprintf("\tSearching for: "); knprints(name, namelen);
    SkipColon(name, namelen);

    InitDirHandle(glob->sb, dir_cluster, &dh);
    err = GetDirEntryByPath(&dh, name, namelen, &de);

    if (err == 0) {
        if (FIRST_FILE_CLUSTER(&de) == 0)
            err = LockRoot(access, result);
        else
            err = LockFile(de.index, dh.ioh.first_cluster, access, result);
    }

    ReleaseDirHandle(&dh);

    return err;
}

LONG LockFile(ULONG entry, ULONG cluster, LONG axs, BPTR *res) {
    struct ExtFileLock *fl;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG len;

    kprintf("\tLockFile entry %ld cluster %ld\n", entry, cluster);

    if ((fl = AllocVecPooled(glob->mempool, sizeof(struct ExtFileLock))) == NULL)
        return ERROR_NO_FREE_STORE;

    InitDirHandle(glob->sb, cluster, &dh);
    GetDirEntry(&dh, entry, &de);

    fl->fl_Access = axs;
    fl->fl_Task = glob->ourport;
    fl->fl_Volume = MKBADDR(glob->sb->doslist);
    fl->fl_Link = glob->sb->doslist->dol_misc.dol_volume.dol_LockList;
    fl->magic = ID_FAT_DISK;

    glob->sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

    fl->dir_entry = entry;
    fl->dir_cluster = cluster;
    fl->attr = de.e.entry.attr;
    fl->size = AROS_LE2LONG(de.e.entry.file_size);

    fl->ioh.sb = glob->sb;
    fl->ioh.first_cluster = FIRST_FILE_CLUSTER(&de);
    fl->ioh.block = NULL;
    RESET_HANDLE(&(fl->ioh));

    GetDirShortName(&de, &(fl->name[1]), &len); fl->name[0] = (UBYTE) len;
    GetDirLongName(&de, &(fl->name[1]), &len); fl->name[0] = (UBYTE) len;

    ReleaseDirHandle(&dh);

    *res = MKBADDR(fl);
    return 0;
}

LONG LockRoot(LONG axs, BPTR *res) {
    struct ExtFileLock *fl;

    kprintf("\tLockRoot()\n");

    if ((fl = AllocVecPooled(glob->mempool, sizeof(struct ExtFileLock))) == NULL)
        return ERROR_NO_FREE_STORE;

    fl->fl_Access = axs;
    fl->fl_Task = glob->ourport;
    fl->fl_Volume = MKBADDR(glob->sb->doslist);
    fl->fl_Link = glob->sb->doslist->dol_misc.dol_volume.dol_LockList;
    fl->magic = ID_FAT_DISK;

    glob->sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

    fl->dir_entry = FAT_ROOTDIR_MARK;
    fl->dir_cluster = FAT_ROOTDIR_MARK;
    fl->attr = ATTR_DIRECTORY;
    fl->size = 0;

    fl->ioh.sb = glob->sb;
    fl->ioh.first_cluster = 0;
    fl->ioh.block = NULL;
    RESET_HANDLE(&(fl->ioh));

    CopyMem(glob->sb->volume.name, fl->name, 32);

    *res = MKBADDR(fl);
    return 0;
}

LONG CopyLock(struct ExtFileLock *src_fl, BPTR *res) {
    struct ExtFileLock *fl;

    if (src_fl->fl_Access == EXCLUSIVE_LOCK)
        return ERROR_OBJECT_IN_USE;

    if ((fl = AllocVecPooled(glob->mempool, sizeof(struct ExtFileLock))) == NULL)
        return ERROR_NO_FREE_STORE;

    fl->fl_Access = src_fl->fl_Access;
    fl->fl_Task = glob->ourport;
    fl->fl_Volume = MKBADDR(glob->sb->doslist);
    fl->fl_Link = glob->sb->doslist->dol_misc.dol_volume.dol_LockList;
    fl->magic = ID_FAT_DISK;

    glob->sb->doslist->dol_misc.dol_volume.dol_LockList = MKBADDR(fl);

    fl->dir_entry = src_fl->dir_entry;
    fl->dir_cluster = src_fl->dir_cluster;
    fl->attr = src_fl->attr;
    fl->size = src_fl->size;

    fl->ioh.sb = glob->sb;
    fl->ioh.first_cluster = src_fl->ioh.first_cluster;
    fl->ioh.block = NULL;
    RESET_HANDLE(&(fl->ioh));

    memcpy(fl->name, src_fl->name, 108);

    *res = MKBADDR(fl);
    return 0;
}

LONG LockParent(struct ExtFileLock *fl, LONG axs, BPTR *res) {
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG parent_cluster;

    /* if we're in the root directory, then the root is our parent */
    if (fl->dir_cluster == 0)
        return LockRoot(axs, res);

    /* get the parent dir */
    InitDirHandle(glob->sb, fl->dir_cluster, &dh);
    if ((err = GetDirEntryByPath(&dh, "/", 1, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* and its cluster */
    parent_cluster = FIRST_FILE_CLUSTER(&de);

    /* then we go through the parent dir, looking for a link back to us. we do
     * this so that we have an entry with the proper name for copying by
     * LockFile() */
    InitDirHandle(glob->sb, parent_cluster, &dh);
    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de)) == 0) {
        /* don't go past the end */
        if (de.e.entry.name[0] == 0x00) {
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }

        /* we found it if its not empty, and its not the volume id or a long
         * name, and it is a directory, and it does point to us */
        if (de.e.entry.name[0] != 0xe5 &&
            !(de.e.entry.attr & ATTR_VOLUME_ID) &&
            de.e.entry.attr & ATTR_DIRECTORY &&
            FIRST_FILE_CLUSTER(&de) == fl->dir_cluster) {
            
            err = LockFile(dh.cur_index, parent_cluster, axs, res);
            break;
        }
    }

    ReleaseDirHandle(&dh);
    return err;
}

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

        if (fl->ioh.block != NULL)
            cache_put_block(glob->cache, fl->ioh.block, 0);

        FreeVecPooled(glob->mempool, fl);

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

            FreeVecPooled(glob->mempool, ptr);
        }
    }
}

