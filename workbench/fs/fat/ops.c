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

/*
 * this takes a full path and moves to the directory that would contain the
 * last file in the path. ie calling with (dh, "foo/bar/baz", 11) will move to
 * directory "foo/bar" under the dir specified by dh. dh will become a handle
 * to the new dir. after the return name will be "baz" and namelen will be 3
 */
static LONG MoveToSubdir(struct DirHandle *dh, UBYTE **pname, ULONG *pnamelen) {
    LONG err;
    UBYTE *name = *pname, *base;
    ULONG namelen = *pnamelen, baselen;
    struct DirEntry de;

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

    if (baselen > 0) {
        if ((err = GetDirEntryByPath(dh, base, baselen, &de)) != 0) {
            D(bug("[fat] base not found\n"));
            return err;
        }

        if ((err = InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(&de), dh)) != 0)
            return err;
    }

    *pname = name;
    *pnamelen = namelen;

    return 0;
}

/*
 * obtains a lock on the named file under the given dir. this is the service
 * routine for DOS Open() (ie FINDINPUT/FINDOUTPUT/FINDUPDATE) and as such may
 * only return a lock on a file, never on a dir.
 */
LONG OpOpenFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, LONG action, struct ExtFileLock **filelock) {
    LONG err;
    BPTR b;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;

    D(bug("[fat] opening file '%.*s' in dir at cluster %ld, action %s\n",
          namelen, name, dirlock != NULL ? dirlock->ioh.first_cluster : 0,
          action == ACTION_FINDINPUT  ? "FINDINPUT"  :
          action == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
          action == ACTION_FINDUPDATE ? "FINDUPDATE" : "[unknown]"));

    /* no filename means they're trying to open whatever dirlock is (which
     * despite the name may not actually be a dir). since there's already an
     * extant lock, its never going be possible to get an exclusive lock, so
     * this will only work for FINDINPUT (read-only) */
    if (namelen == 0) {
        D(bug("[fat] trying to copy passed dir lock\n"));

        if (action != ACTION_FINDINPUT) {
            D(bug("[fat] can't copy lock for write (exclusive)\n"));
            return ERROR_OBJECT_IN_USE;
        }

        /* dirs can't be opened */
        if (dirlock == NULL || dirlock->attr & ATTR_DIRECTORY) {
            D(bug("[fat] dir lock is a directory, which can't be opened\n"));
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* its a file, just copy the lock */
        if ((err = CopyLock(dirlock, &b)) != 0)
            return err;

        *filelock = BADDR(b);
        return 0;
    }

    /* lock the file */
    err = TryLockObj(dirlock, name, namelen, action == ACTION_FINDINPUT ? SHARED_LOCK : EXCLUSIVE_LOCK, &b);

    /* found it */
    if (err == 0) {
        lock = BADDR(b);

        /* can't open directories */
        if (lock->attr & ATTR_DIRECTORY) {
            FreeLock(lock);
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* INPUT/UPDATE use the file as/is */
        if (action != ACTION_FINDOUTPUT) {
            *filelock = lock;
            return 0;
        }

        /* whereas OUTPUT truncates it */

        /* update the dir entry to make the file empty */
        InitDirHandle(lock->ioh.sb, lock->dir_cluster, &dh);
        GetDirEntry(&dh, lock->dir_entry, &de);
        de.e.entry.first_cluster_lo = de.e.entry.first_cluster_hi = 0;
        de.e.entry.file_size = 0;
        UpdateDirEntry(&de);

        /* free the clusters */
        FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);
        lock->ioh.first_cluster = 0xffffffff;
        RESET_HANDLE(&lock->ioh);
        lock->size = 0;

        /* file is empty, go */
        *filelock = lock;

        return 0;
    }

    /* any error other than "not found" should be taken as-is */
    if (err != ERROR_OBJECT_NOT_FOUND)
        return err;

    /* not found. for INPUT or UPDATE, we bail out */
    if (action != ACTION_FINDOUTPUT)
        return ERROR_OBJECT_NOT_FOUND;

    /* otherwise its time to create the file. get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0)
        return err;

    /* now see if the wanted name is in this dir. if it exists, then we do
     * nothing */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de)) == 0) {
        D(bug("[fat] name exists, can't do anything\n"));
        ReleaseDirHandle(&dh);
        return ERROR_OBJECT_EXISTS;
    }

    /* create the entry */
    if ((err = CreateDirEntry(&dh, name, namelen, 0, 0, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* lock the new file */
    err = LockFile(de.index, de.cluster, EXCLUSIVE_LOCK, &b);
    *filelock = BADDR(b);

    /* done */
    ReleaseDirHandle(&dh);

    return err;
}

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
    FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);

    /* this lock is now completely meaningless */
    FreeLock(lock);

    D(bug("[fat] deleted '%.*s'\n", namelen, name));

    return 0;
}

LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct ExtFileLock **newdirlock) {
    LONG err;
    BPTR b;
    ULONG cluster;
    struct DirHandle dh, sdh;
    struct DirEntry de, sde;

    D(bug("[fat] creating directory '%.*s' in directory at cluster %ld\n", namelen, name, dirlock != NULL ? dirlock->ioh.first_cluster : 0));

    /* get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0)
        return err;

    /* now see if the wanted name is in this dir. if it exists, then we do
     * nothing */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de)) == 0) {
        D(bug("[fat] name exists, can't do anything\n"));
        ReleaseDirHandle(&dh);
        return ERROR_OBJECT_EXISTS;
    }

    /* find a free cluster to store the dir in */
    if ((err = FindFreeCluster(dh.ioh.sb, &cluster)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* allocate it */
    SET_NEXT_CLUSTER(dh.ioh.sb, cluster, dh.ioh.sb->eoc_mark);

    D(bug("[fat] allocated cluster %ld for directory\n", cluster));

    /* create the entry, pointing to the new cluster */
    if ((err = CreateDirEntry(&dh, name, namelen, ATTR_DIRECTORY, cluster, &de)) != 0) {
        /* deallocate the cluster */
        SET_NEXT_CLUSTER(dh.ioh.sb, cluster, 0);

        ReleaseDirHandle(&dh);
        return err;
    }

    /* now get a handle on the new directory */
    InitDirHandle(dh.ioh.sb, cluster, &sdh);

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

LONG OpRead(struct ExtFileLock *lock, UBYTE *data, ULONG want, ULONG *read) {
    LONG err;

    D(bug("[fat] request to read %ld bytes from file pos %ld\n", want, lock->pos));

    if (want + lock->pos > lock->size) {
        want = lock->size - lock->pos;
        D(bug("[fat] full read would take us past end-of-file, adjusted want to %ld bytes\n", want));
    }

    if ((err = ReadFileChunk(&(lock->ioh), lock->pos, want, data, read)) == 0) {
        lock->pos += *read;
        D(bug("[fat] read %ld bytes, new file pos is %ld\n", *read, lock->pos));
    }

    return err;
}

LONG OpWrite(struct ExtFileLock *lock, UBYTE *data, ULONG want, ULONG *written) {
    LONG err;
    BOOL update_entry = FALSE;
    struct DirHandle dh;
    struct DirEntry de;

    D(bug("[fat] request to write %ld bytes to file pos %ld\n", want, lock->pos));

    /* if this is the first write, make a note as we'll have to store the
     * first cluster in the directory entry later */
    if (lock->ioh.first_cluster == 0xffffffff)
        update_entry = TRUE;

    if ((err = WriteFileChunk(&(lock->ioh), lock->pos, want, data, written)) == 0) {
        lock->pos += *written;
        if (lock->pos > lock->size) {
            lock->size = lock->pos;
            update_entry = TRUE;
        }

        D(bug("[fat] wrote %ld bytes, new file pos is %ld, size is %ld\n", *written, lock->pos, lock->size));

        if (update_entry) {
            D(bug("[fat] updating dir entry, first cluster is %ld, size is %ld\n", lock->ioh.first_cluster, lock->size));

            InitDirHandle(lock->ioh.sb, lock->dir_cluster, &dh);
            GetDirEntry(&dh, lock->dir_entry, &de);

            de.e.entry.file_size = lock->size;
            de.e.entry.first_cluster_lo = lock->ioh.first_cluster & 0xffff;
            de.e.entry.first_cluster_hi = lock->ioh.first_cluster >> 16;

            UpdateDirEntry(&de);
        }
    }

    return err;
}
