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
#include <dos/dos.h>
#include <proto/exec.h>

#include "fat_fs.h"
#include "fat_protos.h"


#define FREE_CLUSTER_CHAIN(sb,cl)                               \
    do {                                                        \
        ULONG cluster = cl;                                     \
        while (cluster >= 0 && cluster < sb->eoc_mark) {        \
            ULONG next_cluster = GET_NEXT_CLUSTER(sb, cluster); \
            SET_NEXT_CLUSTER(sb, cluster, 0);                   \
            cluster = next_cluster;                             \
        }                                                       \
    } while(0)

/* find the named file in the directory referenced by dirlock, and delete it.
 * if the file is a directory, it will only be deleted if its empty */
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen) {
    LONG err;
    BPTR b;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;
    UBYTE checksum;
    ULONG order;

    D(bug("[fat] deleting file '%.*s' in directory at cluster %ld\n", namelen, name, dirlock->ioh.first_cluster));

    /* obtain a lock on the file. we need an exclusive lock as we don't want
     * to delete the file if its in use */
    if ((err = TryLockObj(dirlock, name, namelen, EXCLUSIVE_LOCK, &b)) != 0) {
        D(bug("[fat] couldn't obtain exclusive lock on named file\n"));
        return err;
    }
    lock = BADDR(b);

    /* if its a directory, we have to make sure its empty */
    if (lock->attr & ATTR_DIRECTORY) {
        D(bug("[fat] file is a directory, making sure its empty\n"));

        if ((err = InitDirHandle(lock->ioh.sb, lock->ioh.first_cluster, &dh)) != 0) {
            FreeLock(lock);
            return err;
        }

        /* loop over the entries, starting from entry 2 (the first real
         * entry). skipping unused ones, we look for the end-of-directory
         * marker. if we find it, the directory is empty. if we find a real
         * name, its in use */
        de.index = 1;
        while ((err = GetDirEntry(&dh, de.index+1, &de)) == 0) {
            /* skip unused entries */
            if (de.e.entry.name[0] == 0xe5)
                continue;

            /* end of directory, its empty */
            if (de.e.entry.name[0] == 0x00)
                break;

            /* otherwise the directory is still in use */
            D(bug("[fat] directory still has files in it, won't delete it\n"));

            ReleaseDirHandle(&dh);
            FreeLock(lock);
            return ERROR_DIRECTORY_NOT_EMPTY;
        }

        ReleaseDirHandle(&dh);
    }

    /* open the containing directory */
    if ((err = InitDirHandle(lock->ioh.sb, lock->dir_cluster, &dh)) != 0) {
        FreeLock(lock);
        return err;
    }

    /* get the entry for the file */
    GetDirEntry(&dh, lock->dir_entry, &de);

    /* calculate the short name checksum before we trample on the name */
    CALC_SHORT_NAME_CHECKSUM(de.e.entry.name, checksum);

    D(bug("[fat] short name checksum is 0x%02x\n", checksum));

    /* mark the short entry free */
    de.e.entry.name[0] = 0xe5;
    UpdateDirEntry(&de);

    D(bug("[fat] deleted short name entry\n"));

    /* now we loop over the previous entries, looking for matching long name
     * entries and killing them */
    order = 1;
    while ((err = GetDirEntry(&dh, de.index-1, &de)) == 0) {

        /* see if this is a matching long name entry. if its not, we're done */
        if (!((de.e.entry.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) ||
            (de.e.long_entry.order & ~0x40) != order ||
            de.e.long_entry.checksum != checksum)

            break;

        /* kill it */
        de.e.entry.name[0] = 0xe5;
        UpdateDirEntry(&de);

        order++;
    }

    D(bug("[fat] deleted %ld long name entries\n", order-1));

    /* directory entries are free */
    ReleaseDirHandle(&dh);

    /* now free the clusters the file was using */
    FREE_CLUSTER_CHAIN(glob->sb, lock->ioh.first_cluster);

    /* this lock is now completely meaningless */
    FreeLock(lock);

    D(bug("[fat] deleted '%.*s'\n", namelen, name));

    return 0;
}

LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct ExtFileLock **newdirlock) {
    LONG err;
    BPTR b;
    UBYTE *base;
    ULONG baselen;
    ULONG cluster;
    struct DirHandle dh, sdh;
    struct DirEntry de, sde;

    D(bug("[fat] creating directory '%.*s' in directory at cluster %ld\n", namelen, name, dirlock->ioh.first_cluster));

    /* we break the given name into two pieces - the name of the containing
     * dir, and the name of the new dir to go within it. if the base ends up
     * empty, then we just use the dirlock */
    baselen = namelen;
    base = name;
    while (baselen > 0) {
        if (base[baselen-1] != '/')
            break;
        baselen--;
    }
    while (baselen > 0) {
        if (base[baselen-1] == '/')
            break;
        baselen--;
    }
    namelen -= baselen;
    name = &base[baselen];

    D(bug("[fat] base is '%.*s', name is '%.*s'\n", baselen, base, namelen, name));

    if ((err = InitDirHandle(glob->sb, dirlock->ioh.first_cluster, &dh)) != 0)
        return err;

    if (baselen > 0) {
        if ((err = GetDirEntryByPath(&dh, base, baselen, &de)) != 0) {
            D(bug("[fat] base not found\n"));
            return err;
        }

        if ((err = InitDirHandle(glob->sb, FIRST_FILE_CLUSTER(&de), &dh)) != 0)
            return err;
    }

    /* now see if the wanted name is in this dir. if it exists, then we do
     * nothing */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de)) == 0) {
        D(bug("[fat] name exists, can't do anything\n"));
        ReleaseDirHandle(&dh);
        return ERROR_OBJECT_EXISTS;
    }

    /* find a free cluster to store the dir in */
    if ((err = FindFreeCluster(glob->sb, &cluster)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* allocate it */
    SET_NEXT_CLUSTER(glob->sb, cluster, glob->sb->eoc_mark);

    D(bug("[fat] allocated cluster %ld for directory\n", cluster));

    /* create the entry, pointing to the new cluster */
    if ((err = CreateDirEntry(&dh, name, namelen, ATTR_DIRECTORY, cluster, &de)) != 0) {
        /* deallocate the cluster */
        SET_NEXT_CLUSTER(glob->sb, cluster, 0);

        ReleaseDirHandle(&dh);
        return err;
    }

    /* now get a handle on the new directory */
    InitDirHandle(glob->sb, cluster, &sdh);

    /* create the dot entry. its a direct copy of the just-created entry, but
     * with a different name */
    GetDirEntry(&sdh, 0, &sde);
    CopyMem(&de.e.entry, &sde.e.entry, sizeof(struct FATDirEntry));
    CopyMem(".          ", &sde.e.entry.name, 11);
    UpdateDirEntry(&sde);

    /* create the dot-dot entry. again, a copy, with the cluster pointer setup
     * to point to the parent */
    GetDirEntry(&sdh, 1, &sde);
    CopyMem(&de.e.entry, &sde.e.entry, sizeof(struct FATDirEntry));
    CopyMem("..         ", &sde.e.entry.name, 11);
    sde.e.entry.first_cluster_lo = dh.ioh.first_cluster & 0xffff;
    sde.e.entry.first_cluster_hi = dh.ioh.first_cluster >> 16;
    UpdateDirEntry(&sde);

    /* put an empty entry at the end to mark end of directory */
    GetDirEntry(&sdh, 2, &sde);
    memset(&sde.e.entry, 0, sizeof(struct FATDirEntry));
    UpdateDirEntry(&sde);

    /* new dir created */
    ReleaseDirHandle(&sdh);

    /* now obtain a lock on the new dir */
    err = LockFile(de.index, de.cluster, SHARED_LOCK, &b);
    *newdirlock = BADDR(b);

    /* done */
    ReleaseDirHandle(&dh);

    return err;
}
