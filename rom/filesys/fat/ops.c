/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2011 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define AROS_ALMOST_COMPATIBLE

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/exec.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_OPS
#include "debug.h"

#define FREE_CLUSTER_CHAIN(sb,cl)                               \
    do {                                                        \
        ULONG cluster = cl;                                     \
        while (cluster >= 0 && cluster < sb->eoc_mark - 7) {    \
            ULONG next_cluster = GET_NEXT_CLUSTER(sb, cluster); \
            FreeCluster(sb, cluster);                           \
            cluster = next_cluster;                             \
        }                                                       \
    } while(0)

/*
 * this takes a full path and moves to the directory that would contain the
 * last file in the path. e.g. calling with (dh, "foo/bar/baz", 11) will move to
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

    D(bug("[fat] base is '"); RawPutChars(base, baselen);
      bug("', name is '"); RawPutChars(name, namelen); bug("'\n"));

    if (baselen > 0) {
        if ((err = GetDirEntryByPath(dh, base, baselen, &de)) != 0) {
            D(bug("[fat] base not found\n"));
            return err;
        }

	if ((err = InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(&de), dh, TRUE)) != 0)
            return err;
    }

    *pname = name;
    *pnamelen = namelen;

    return 0;
}

LONG OpLockFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, LONG access, struct ExtFileLock **filelock) {
    /* if they passed in a name, go searching for it */
    if (namelen != 0)
        return LockFileByName(dirlock, name, namelen, access, filelock);

    /* otherwise the empty filename, just make a copy */
    else if (dirlock != NULL)
        return CopyLock(dirlock, filelock);

    /* null dir lock means they want the root */
    else
        return LockRoot(access, filelock);
}

void OpUnlockFile(struct ExtFileLock *lock) {
    if (lock != NULL)
        FreeLock(lock);
}

LONG OpCopyLock(struct ExtFileLock *lock, struct ExtFileLock **copy) {
    if (lock != NULL)
        return CopyLock(lock, copy);
    else
        return LockRoot(SHARED_LOCK, copy);
}

LONG OpLockParent(struct ExtFileLock *lock, struct ExtFileLock **parent) {
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG parent_cluster;

    /* the root has no parent, but as a special case we have to return success
     * with the zero lock */
    if (lock == NULL || lock->gl == &glob->sb->info->root_lock) {
        *parent = NULL;
        return 0;
    }

    /* if we're in the root directory, then the root is our parent */
    if (lock->gl->dir_cluster == glob->sb->rootdir_cluster)
        return LockRoot(SHARED_LOCK, parent);

    /* get the parent dir */
    InitDirHandle(glob->sb, lock->gl->dir_cluster, &dh, FALSE);
    if ((err = GetDirEntryByPath(&dh, "/", 1, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* and its cluster */
    parent_cluster = FIRST_FILE_CLUSTER(&de);

    /* then we go through the parent dir, looking for a link back to us. we do
     * this so that we have an entry with the proper name for copying by
     * LockFile() */
    InitDirHandle(glob->sb, parent_cluster, &dh, TRUE);
    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de)) == 0) {
        /* don't go past the end */
        if (de.e.entry.name[0] == 0x00) {
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }

        /* we found it if it's not empty, and it's not the volume id or a long
         * name, and it is a directory, and it does point to us */
        if (de.e.entry.name[0] != 0xe5 &&
            !(de.e.entry.attr & ATTR_VOLUME_ID) &&
            de.e.entry.attr & ATTR_DIRECTORY &&
            FIRST_FILE_CLUSTER(&de) == lock->gl->dir_cluster) {
            
            err = LockFile(parent_cluster, dh.cur_index, SHARED_LOCK, parent);
            break;
        }
    }

    ReleaseDirHandle(&dh);
    return err;
}

/*
 * obtains a lock on the named file under the given dir. this is the service
 * routine for DOS Open() (ie FINDINPUT/FINDOUTPUT/FINDUPDATE) and as such may
 * only return a lock on a file, never on a dir.
 */
LONG OpOpenFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, LONG action, struct ExtFileLock **filelock) {
    LONG err;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;

    D(bug("[fat] opening file '"); RawPutChars(name, namelen);
      bug("' in dir at cluster %ld, action %s\n",
	  dirlock != NULL ? dirlock->ioh.first_cluster : 0,
          action == ACTION_FINDINPUT  ? "FINDINPUT"  :
          action == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
          action == ACTION_FINDUPDATE ? "FINDUPDATE" : "[unknown]"));

    dh.ioh.sb = NULL; /* Explicitly mark the dirhandle as uninitialised */

    /* no filename means they're trying to open whatever dirlock is (which
     * despite the name may not actually be a dir). since there's already an
     * extant lock, it's never going to be possible to get an exclusive lock,
     * so this will only work for FINDINPUT (read-only) */
    if (namelen == 0) {
        D(bug("[fat] trying to copy passed dir lock\n"));

        if (action != ACTION_FINDINPUT) {
            D(bug("[fat] can't copy lock for write (exclusive)\n"));
            return ERROR_OBJECT_IN_USE;
        }

        /* dirs can't be opened */
        if (dirlock == NULL || dirlock->gl->attr & ATTR_DIRECTORY) {
            D(bug("[fat] dir lock is a directory, which can't be opened\n"));
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* it's a file, just copy the lock */
        return CopyLock(dirlock, filelock);
    }

    /* lock the file */
    err = LockFileByName(dirlock, name, namelen, action == ACTION_FINDINPUT ? SHARED_LOCK : EXCLUSIVE_LOCK, &lock);

    /* found it */
    if (err == 0) {
        D(bug("[fat] found existing file\n"));

        /* can't open directories */
        if (lock->gl->attr & ATTR_DIRECTORY) {
            D(bug("[fat] it's a directory, can't open it\n"));
            FreeLock(lock);
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* INPUT/UPDATE use the file as/is */
        if (action != ACTION_FINDOUTPUT) {
            D(bug("[fat] returning the lock\n"));
            *filelock = lock;
            return 0;
        }

        /* whereas OUTPUT truncates it */
        D(bug("[fat] handling FINDOUTPUT, so truncating the file\n"));

        if (lock->gl->attr & ATTR_READ_ONLY) {
            D(bug("[fat] file is write protected, doing nothing\n"));
            FreeLock(lock);
            return ERROR_WRITE_PROTECTED;
        }

        /* update the dir entry to make the file empty */
	InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh, FALSE);
        GetDirEntry(&dh, lock->gl->dir_entry, &de);
        de.e.entry.first_cluster_lo = de.e.entry.first_cluster_hi = 0;
        de.e.entry.file_size = 0;
        de.e.entry.attr |= ATTR_ARCHIVE;
        UpdateDirEntry(&de);

        D(bug("[fat] set first cluster and size to 0 in directory entry\n"));

        /* free the clusters */
        FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);
        lock->gl->first_cluster = lock->ioh.first_cluster = 0xffffffff;
        RESET_HANDLE(&lock->ioh);
        lock->gl->size = 0;

        D(bug("[fat] file truncated, returning the lock\n"));

        /* file is empty, go */
        *filelock = lock;

        return 0;
    }

    /* any error other than "not found" should be taken as-is */
    if (err != ERROR_OBJECT_NOT_FOUND)
        return err;

    /* not found. for INPUT we bail out */
    if (action == ACTION_FINDINPUT) {
        D(bug("[fat] file not found, and not creating it\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    D(bug("[fat] trying to create '"); RawPutChars(name, namelen); bug("'\n"));

    /* otherwise it's time to create the file. get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, TRUE)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* if the dir is write protected, can't do anything. root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster) {
        GetDirEntry(&dh, 0, &de);
        if (de.e.entry.attr & ATTR_READ_ONLY) {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh);
            return ERROR_WRITE_PROTECTED;
        }
    }

    /* create the entry */
    if ((err = CreateDirEntry(&dh, name, namelen, ATTR_ARCHIVE, 0, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* lock the new file */
    err = LockFile(de.cluster, de.index, EXCLUSIVE_LOCK, filelock);

    /* done */
    ReleaseDirHandle(&dh);

    if (err == 0) {
        (*filelock)->do_notify = TRUE;
        D(bug("[fat] returning lock on new file\n"));
    }

    return err;
}

/* find the named file in the directory referenced by dirlock, and delete it.
 * if the file is a directory, it will only be deleted if it's empty */
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen) {
    LONG err;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;

    D(bug("[fat] deleting file '"); RawPutChars(name, namelen);
      bug("' in directory at cluster % ld\n", dirlock != NULL ? dirlock->ioh.first_cluster : 0));

    dh.ioh.sb = NULL;

    /* obtain a lock on the file. we need an exclusive lock as we don't want
     * to delete the file if it's in use */
    if ((err = LockFileByName(dirlock, name, namelen, EXCLUSIVE_LOCK, &lock)) != 0) {
        D(bug("[fat] couldn't obtain exclusive lock on named file\n"));
        return err;
    }

    if (lock->gl->attr & ATTR_READ_ONLY) {
        D(bug("[fat] file is write protected, doing nothing\n"));
        FreeLock(lock);
        return ERROR_DELETE_PROTECTED;
    }

    /* if it's a directory, we have to make sure it's empty */
    if (lock->gl->attr & ATTR_DIRECTORY) {
        D(bug("[fat] file is a directory, making sure it's empty\n"));

	if ((err = InitDirHandle(lock->ioh.sb, lock->ioh.first_cluster, &dh, FALSE)) != 0) {
            FreeLock(lock);
            return err;
        }

        /* loop over the entries, starting from entry 2 (the first real
         * entry). skipping unused ones, we look for the end-of-directory
         * marker. if we find it, the directory is empty. if we find a real
         * name, it's in use */
        de.index = 1;
        while ((err = GetDirEntry(&dh, de.index+1, &de)) == 0) {
            /* skip unused entries */
            if (de.e.entry.name[0] == 0xe5)
                continue;

            /* end of directory, it's empty */
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
    if ((err = InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh, TRUE)) != 0) {
        FreeLock(lock);
        return err;
    }

    /* if the dir is write protected, can't do anything. root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster) {
        GetDirEntry(&dh, 0, &de);
        if (de.e.entry.attr & ATTR_READ_ONLY) {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh);
            FreeLock(lock);
            return ERROR_WRITE_PROTECTED;
        }
    }

    /* get the entry for the file */
    GetDirEntry(&dh, lock->gl->dir_entry, &de);

    /* kill it */
    DeleteDirEntry(&de);

    /* it's all good */
    ReleaseDirHandle(&dh);

    /* now free the clusters the file was using */
    FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);

    /* notify */
    SendNotifyByLock(lock->ioh.sb, lock->gl);

    /* this lock is now completely meaningless */
    FreeLock(lock);

    D(bug("[fat] deleted '"); RawPutChars(name, namelen); bug("'\n"));

    return 0;
}

LONG OpRenameFile(struct ExtFileLock *sdirlock, UBYTE *sname, ULONG snamelen, struct ExtFileLock *ddirlock, UBYTE *dname, ULONG dnamelen) {
    struct DirHandle sdh, ddh;
    struct DirEntry sde, dde;
    struct GlobalLock *gl;
    LONG err;
    ULONG len;

    /* get the source dir handle */
    if ((err = InitDirHandle(glob->sb, sdirlock != NULL ? sdirlock->ioh.first_cluster : 0, &sdh, FALSE)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&sdh, &sname, &snamelen)) != 0) {
        ReleaseDirHandle(&sdh);
        return err;
    }

    /* get the entry */
    if ((err = GetDirEntryByName(&sdh, sname, snamelen, &sde)) != 0) {
        ReleaseDirHandle(&sdh);
        return err;
    }

    /* now get a handle on the passed dest dir */
    if ((err = InitDirHandle(glob->sb, ddirlock != NULL ? ddirlock->ioh.first_cluster : 0, &ddh, FALSE)) != 0) {
        ReleaseDirHandle(&sdh);
        return err;
    }

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&ddh, &dname, &dnamelen)) != 0) {
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
        return err;
    }

    /* check the source and dest dirs. if either is read-only, do nothing */
    GetDirEntry(&sdh, 0, &dde);
    if (dde.e.entry.attr & ATTR_READ_ONLY) {
        D(bug("[fat] source dir is read only, doing nothing\n"));
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
        return ERROR_WRITE_PROTECTED;
    }
    GetDirEntry(&ddh, 0, &dde);
    if (dde.e.entry.attr & ATTR_READ_ONLY) {
        D(bug("[fat] dest dir is read only, doing nothing\n"));
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
        return ERROR_WRITE_PROTECTED;
    }

    /* now see if the wanted name is in this dir. if it exists, do nothing */
    if ((err = GetDirEntryByName(&ddh, dname, dnamelen, &dde)) == 0) {
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
        return ERROR_OBJECT_EXISTS;
    }
    else if (err != ERROR_OBJECT_NOT_FOUND) {
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
        return err;
    }

    /* at this point we have the source entry in sde, and we know the dest
     * doesn't exist */

    /* XXX if sdh and ddh are the same dir and there's room in the existing
     * entries for the new name, just overwrite the name */

    /* make a new entry in the target dir */
    if ((err = CreateDirEntry(&ddh, dname, dnamelen, sde.e.entry.attr | ATTR_ARCHIVE, (sde.e.entry.first_cluster_hi << 16) | sde.e.entry.first_cluster_lo, &dde)) != 0) {
        ReleaseDirHandle(&ddh);
        ReleaseDirHandle(&sdh);
    }

    /* copy in the leftover attributes */
    dde.e.entry.create_date = sde.e.entry.create_date;
    dde.e.entry.create_time = sde.e.entry.create_time;
    dde.e.entry.write_date = sde.e.entry.write_date;
    dde.e.entry.write_time = sde.e.entry.write_time;
    dde.e.entry.last_access_date = sde.e.entry.last_access_date;
    dde.e.entry.create_time_tenth = sde.e.entry.create_time_tenth;
    dde.e.entry.file_size = sde.e.entry.file_size;

    UpdateDirEntry(&dde);

    /* update the global lock (if present) with the new dir cluster/entry */
    ForeachNode(&sdh.ioh.sb->info->locks, gl)
        if (gl->dir_cluster == sde.cluster && gl->dir_entry == sde.index) {
            D(bug("[fat] found lock with old dir entry (%ld/%ld), changing to (%ld/%ld)\n", sde.cluster, sde.index, dde.cluster, dde.index));

            gl->dir_cluster = dde.cluster;
            gl->dir_entry = dde.index;

            /* update the filename too */
            GetDirEntryShortName(&dde, &(gl->name[1]), &len); gl->name[0] = (UBYTE) len;
            GetDirEntryLongName(&dde, &(gl->name[1]), &len); gl->name[0] = (UBYTE) len;
        }

    /* delete the original */
    DeleteDirEntry(&sde);

    /* notify */
    SendNotifyByDirEntry(sdh.ioh.sb, &dde);

    ReleaseDirHandle(&ddh);
    ReleaseDirHandle(&sdh);

    return 0;
}

LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct ExtFileLock **newdirlock) {
    LONG err, i;
    ULONG cluster;
    struct DirHandle dh, sdh;
    struct DirEntry de, sde;

    D(bug("[fat] creating directory '"); RawPutChars(name, namelen);
      bug("' in directory at cluster %ld\n", dirlock != NULL ? dirlock->ioh.first_cluster : 0));

    /* get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* Make sure 'name' is just the FilePart() */
    for (i = namelen-1; i > 0; i--) {
        if (name[i] == '/' ||
            name[i] == ':') {
            namelen -= (i + 1);
            name    += (i + 1);
            break;
        }
    }

    /* if the dir is write protected, can't do anything. root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster) {
        GetDirEntry(&dh, 0, &de);
        if (de.e.entry.attr & ATTR_READ_ONLY) {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh);
            return ERROR_WRITE_PROTECTED;
        }
    }

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
    AllocCluster(dh.ioh.sb, cluster);

    D(bug("[fat] allocated cluster %ld for directory\n", cluster));

    /* create the entry, pointing to the new cluster */
    if ((err = CreateDirEntry(&dh, name, namelen, ATTR_DIRECTORY | ATTR_ARCHIVE, cluster, &de)) != 0) {
        /* deallocate the cluster */
        FreeCluster(dh.ioh.sb, cluster);

        ReleaseDirHandle(&dh);
        return err;
    }

    /* now get a handle on the new directory */
    InitDirHandle(dh.ioh.sb, cluster, &sdh, FALSE);

    /* create the dot entry. it's a direct copy of the just-created entry, but
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
    cluster = dh.ioh.first_cluster;
    if (cluster == dh.ioh.sb->rootdir_cluster)
        cluster = 0;
    sde.e.entry.first_cluster_lo = cluster & 0xffff;
    sde.e.entry.first_cluster_hi = cluster >> 16;
    UpdateDirEntry(&sde);

    /* clear all remaining entries (the first of which marks the end of the
     * directory) */
    for (i = 2; GetDirEntry(&sdh, i, &sde) == 0; i++) {
        memset(&sde.e.entry, 0, sizeof(struct FATDirEntry));
        UpdateDirEntry(&sde);
    }

    /* new dir created */
    ReleaseDirHandle(&sdh);

    /* now obtain a lock on the new dir */
    err = LockFile(de.cluster, de.index, SHARED_LOCK, newdirlock);

    /* done */
    ReleaseDirHandle(&dh);

    /* notify */
    SendNotifyByLock((*newdirlock)->ioh.sb, (*newdirlock)->gl);

    return err;
}

LONG OpRead(struct ExtFileLock *lock, UBYTE *data, ULONG want, ULONG *read) {
    LONG err;

    D(bug("[fat] request to read %ld bytes from file pos %ld\n", want, lock->pos));

    if (want == 0)
        return 0;

    if (want + lock->pos > lock->gl->size) {
        want = lock->gl->size - lock->pos;
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

    /* need an exclusive lock */
    if (lock->gl->access != EXCLUSIVE_LOCK) {
        D(bug("[fat] can't modify global attributes via a shared lock\n"));
        return ERROR_OBJECT_IN_USE;
    }

    /* don't modify the file if it's protected */
    if (lock->gl->attr & ATTR_READ_ONLY) {
        D(bug("[fat] file is write protected\n"));
        return ERROR_WRITE_PROTECTED;
    }

    if (want == 0) {
        *written = 0;
        return 0;
    }

    /* if this is the first write, make a note as we'll have to store the
     * first cluster in the directory entry later */
    if (lock->ioh.first_cluster == 0xffffffff)
        update_entry = TRUE;

    if ((err = WriteFileChunk(&(lock->ioh), lock->pos, want, data, written)) == 0) {
        /* if nothing was written but success was returned (can that even
         * happen?) then we don't want to mess with the dir entry */
        if (*written == 0) {
            D(bug("[fat] nothing successfully written (!), nothing else to do\n"));
            return 0;
        }

        /* something changed, we need to tell people about it */
        lock->do_notify = TRUE;

        /* move to the end of the area written */
        lock->pos += *written;

        /* update the dir entry if the size changed */
        if (lock->pos > lock->gl->size) {
            lock->gl->size = lock->pos;
            update_entry = TRUE;
        }

        /* force an update if the file hasn't already got an archive bit. this
         * will happen if this was the first write to an existing file that
         * didn't cause it to grow */
        else if (!(lock->gl->attr & ATTR_ARCHIVE))
            update_entry = TRUE;

        D(bug("[fat] wrote %ld bytes, new file pos is %ld, size is %ld\n", *written, lock->pos, lock->gl->size));

        if (update_entry) {
            D(bug("[fat] updating dir entry, first cluster is %ld, size is %ld\n", lock->ioh.first_cluster, lock->gl->size));

            lock->gl->first_cluster = lock->ioh.first_cluster;

            InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh, FALSE);
            GetDirEntry(&dh, lock->gl->dir_entry, &de);

            de.e.entry.file_size = lock->gl->size;
            de.e.entry.first_cluster_lo = lock->gl->first_cluster & 0xffff;
            de.e.entry.first_cluster_hi = lock->gl->first_cluster >> 16;
    
            de.e.entry.attr |= ATTR_ARCHIVE;
            UpdateDirEntry(&de);

            ReleaseDirHandle(&dh);
        }
    }

    return err;
}

LONG OpSetFileSize(struct ExtFileLock *lock, LONG offset, LONG whence, LONG *newsize) {
    LONG err;
    LONG size;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG want, count;
    ULONG cl, next, first, last;

    /* need an exclusive lock to do what is effectively a write */
    if (lock->gl->access != EXCLUSIVE_LOCK) {
        D(bug("[fat] can't modify global attributes via a shared lock\n"));
        return ERROR_OBJECT_IN_USE;
    }

    /* don't modify the file if it's protected */
    if (lock->gl->attr & ATTR_READ_ONLY) {
        D(bug("[fat] file is write protected\n"));
        return ERROR_WRITE_PROTECTED;
    }

    /* calculate the new length based on the current position */
    if (whence == OFFSET_BEGINNING && offset >= 0)
        size = offset;
    else if (whence == OFFSET_CURRENT && lock->pos + offset>= 0)
        size = lock->pos + offset;
    else if (whence == OFFSET_END && offset <= 0 && lock->gl->size + offset >= 0)
        size = lock->gl->size + offset;
    else
        return ERROR_SEEK_ERROR;

    if (lock->gl->size == size) {
        D(bug("[fat] new size matches old size, nothing to do\n"));
        return 0;
    }

    D(bug("[fat] old size was %ld bytes, new size is %ld bytes\n", lock->gl->size, size));

    /* get the dir that this file is in */
    if ((err = InitDirHandle(glob->sb, lock->gl->dir_cluster, &dh, FALSE)) != 0)
        return err;

    /* and the entry */
    if ((err = GetDirEntry(&dh, lock->gl->dir_entry, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* calculate how many clusters we need */
    want = (size >> glob->sb->clustersize_bits) + ((size & (glob->sb->clustersize-1)) ? 1 : 0);

    D(bug("[fat] want %ld clusters for file\n", want));

    /* we're getting three things here - the first cluster of the existing
     * file, the last cluster of the existing file (which might be the same),
     * and the number of clusters currently allocated to it (it's not safe to
     * infer it from the current size as a broken fat implementation may have
     * allocated it more than it needs). we handle file shrinking/truncation
     * here as it falls out naturally from following the current cluster chain
     */

    cl = FIRST_FILE_CLUSTER(&de);
    if (cl == 0) {
        D(bug("[fat] file is empty\n"));

        first = 0;
        count = 0;
    }

    else if (want == 0) {
        /* if we're fully truncating the file, then the below loop will
         * actually not truncate the file at all (count will get incremented
         * past want first time around the loop). it's a pain to incorporate a
         * full truncate into the loop, not counting the change to the first
         * cluster, so it's easier to just take care of it all here */
        D(bug("[fat] want nothing, so truncating the entire file\n"));

        FREE_CLUSTER_CHAIN(glob->sb, cl);

        /* now it has nothing */
        first = 0;
        count = 0;
    }

    else {
        first = cl;
        count = 0;

        /* do the actual count */
        while ((last = GET_NEXT_CLUSTER(glob->sb, cl))
            < glob->sb->eoc_mark - 7) {
            count++;
            cl = last;

            /* if we get as many clusters as we want, kill everything after it */
            if (count == want) {
                FREE_CLUSTER_CHAIN(glob->sb, GET_NEXT_CLUSTER(glob->sb, cl));
                SET_NEXT_CLUSTER(glob->sb, cl, glob->sb->eoc_mark);

                D(bug("[fat] truncated file\n"));

                break;
            }
        }

        D(bug("[fat] file has %ld clusters\n", count));
    }

    /* now we know how big the current file is. if we don't have enough,
     * allocate more until we do */
    if (count < want) {
        D(bug("[fat] growing file\n"));

        while (count < want) {
            if ((err = FindFreeCluster(glob->sb, &next)) != 0) {
                /* XXX probably no free clusters left. we should clean up the
                    * extras we allocated before returning. it won't hurt
                    * anything to leave them but it is dead space */
                ReleaseDirHandle(&dh);
                return err;
            }

            /* mark the cluster used */
            AllocCluster(glob->sb, next);

            /* if the file had no clusters, then this is the first and we
                * need to note it for later storage in the direntry */
            if (cl == 0)
                first = next;

            /* otherwise, hook it up to the current one */
            else
                SET_NEXT_CLUSTER(glob->sb, cl, next);

            /* one more */
            count++;
            cl = next;
        }
    }

    /* clusters are fixed, now update the directory entry */
    de.e.entry.first_cluster_lo = first & 0xffff;
    de.e.entry.first_cluster_hi = first >> 16;
    de.e.entry.file_size = size;
    de.e.entry.attr |= ATTR_ARCHIVE;
    UpdateDirEntry(&de);

    D(bug("[fat] set file size to %ld, first cluster is %ld\n", size, first));

    /* done! */
    *newsize = size;

    return 0;
}

LONG OpSetProtect(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, ULONG prot) {
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;

    /* get the dir handle */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* can't change permissions on the root */
    if (dh.ioh.first_cluster == dh.ioh.sb->rootdir_cluster && namelen == 0) {
        D(bug("[fat] can't set protection on root dir\n"));
        ReleaseDirHandle(&dh);
        return ERROR_INVALID_LOCK;
    }

    /* get the entry */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* set the attributes */
    de.e.entry.attr &= ~(ATTR_ARCHIVE | ATTR_READ_ONLY);
    de.e.entry.attr |= (prot & FIBF_ARCHIVE ? ATTR_ARCHIVE : 0);

    /* only set read-only if neither writable nor deletable */
    if ((prot & (FIBF_WRITE | FIBF_DELETE)) == (FIBF_WRITE | FIBF_DELETE))
        de.e.entry.attr |= ATTR_READ_ONLY;
    UpdateDirEntry(&de);

    D(bug("[fat] new protection is 0x%08x\n", de.e.entry.attr));

    SendNotifyByDirEntry(glob->sb, &de);

    /* if it's a directory, we also need to update the protections for the
     * directory's . entry */
    if (de.e.entry.attr & ATTR_DIRECTORY) {
        ULONG attr = de.e.entry.attr;

        D(bug("[fat] setting protections for directory '.' entry\n"));

	InitDirHandle(glob->sb, FIRST_FILE_CLUSTER(&de), &dh, TRUE);
        GetDirEntry(&dh, 0, &de);
        de.e.entry.attr = attr;
        UpdateDirEntry(&de);
    }

    ReleaseDirHandle(&dh);

    return 0;
}

LONG OpSetDate(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct DateStamp *ds) {
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;

    /* get the dir handle */
    if ((err = InitDirHandle(glob->sb, dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE)) != 0)
        return err;

    /* get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* can't set date on the root */
    if (dh.ioh.first_cluster == dh.ioh.sb->rootdir_cluster && namelen == 0) {
        D(bug("[fat] can't set date on root dir\n"));
        ReleaseDirHandle(&dh);
        return ERROR_INVALID_LOCK;
    }

    /* get the entry */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de)) != 0) {
        ReleaseDirHandle(&dh);
        return err;
    }

    /* set and update the date */
    ConvertAROSDate(ds, &de.e.entry.write_date, &de.e.entry.write_time);
    de.e.entry.last_access_date = de.e.entry.write_date;
    UpdateDirEntry(&de);

    SendNotifyByDirEntry(glob->sb, &de);

    ReleaseDirHandle(&dh);

    return 0;
}

LONG OpAddNotify(struct NotifyRequest *nr) {
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    struct GlobalLock *gl = NULL, *tmp;
    struct NotifyNode *nn;
    BOOL exists = FALSE;

    D(bug("[fat] trying to add notification for '%s'\n", nr->nr_FullName));

    /* if the request is for the volume root, then we just link to the root lock */
    if (nr->nr_FullName[strlen(nr->nr_FullName)-1] == ':') {
        D(bug("[fat] adding notify for root dir\n"));
        gl = &glob->sb->info->root_lock;
    }

    else {
	if ((err = InitDirHandle(glob->sb, 0, &dh, FALSE)) != 0)
        return err;

        /* look for the entry */
        err = GetDirEntryByPath(&dh, nr->nr_FullName, strlen(nr->nr_FullName), &de);
        if (err != 0 && err != ERROR_OBJECT_NOT_FOUND)
            return err;

        /* if it was found, then it might be open. try to find the global lock */
        if (err == 0) {
            exists = TRUE;

            D(bug("[fat] file exists (%ld/%ld), looking for global lock\n", de.cluster, de.index));

            ForeachNode(&glob->sb->info->locks, tmp)
                if (tmp->dir_cluster == de.cluster && tmp->dir_entry == de.index) {
                    gl = tmp;

                    D(bug("[fat] found global lock 0x%0x\n", gl));

                    break;
                }

        }
        else {
            exists = FALSE;

            D(bug("[fat] file doesn't exist\n"));
        }
    }

    if (gl == NULL)
        D(bug("[fat] file not currently locked\n"));

    /* allocate space for the notify node */
    if ((nn = AllocVecPooled(glob->sb->info->mem_pool,
        sizeof(struct NotifyNode))) == NULL)
        return ERROR_NO_FREE_STORE;

    /* plug the bits in */
    nn->gl = gl;
    nn->nr = nr;

    /* add to the list */
    ADDTAIL(&glob->sb->info->notifies, nn);

    /* tell them that the file exists if they wanted to know */
    if (exists && nr->nr_Flags & NRF_NOTIFY_INITIAL)
        SendNotify(nr);

    D(bug("[fat] now reporting activity on '%s'\n", nr->nr_FullName));

    return 0;
}

LONG OpRemoveNotify(struct NotifyRequest *nr) {
    struct FSSuper *sb;
    struct NotifyNode *nn, *nn2;

    D(bug("[fat] trying to remove notification for '%s'\n", nr->nr_FullName));

    /* search inserted volume for the request */
    if (glob->sb != NULL) {
        ForeachNodeSafe(&glob->sb->info->notifies, nn, nn2) {
            if (nn->nr == nr) {
                D(bug("[fat] found notify request in list, removing it\n"));
                REMOVE(nn);
                FreeVecPooled(glob->sb->info->mem_pool, nn);
                return 0;
            }
        }
    }

    /* search offline volumes for the request */
    ForeachNode(&glob->sblist, sb) {
        ForeachNodeSafe(&sb->info->notifies, nn, nn2) {
            if (nn->nr == nr) {
                D(bug("[fat] found notify request in list, removing it\n"));
                REMOVE(nn);
                FreeVecPooled(sb->info->mem_pool, nn);
                AttemptDestroyVolume(sb);
                return 0;
            }
        }
    }

    D(bug("[fat] not found, doing nothing\n"));

    return 0;
}
