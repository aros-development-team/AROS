/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2007-2026 The AROS Development Team
 * Copyright (C) 2006 Marek Szyprowski
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <proto/exec.h>
#include <proto/utility.h>

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/notify.h>

#include <string.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_OPS
#include "debug.h"

/*
 * Clusters 0 and 1 are reserved - entry 0 carries the media descriptor -
 * so an empty file, whose chain starts at 0, has nothing to free. The
 * old lower bound was 'cluster >= 0' on an unsigned, always true, and
 * freeing cluster 0 wrote over the media descriptor.
 */
#define FREE_CLUSTER_CHAIN(sb,cl)                               \
    do {                                                        \
        ULONG cluster = cl;                                     \
        while (cluster >= 2 && cluster < sb->eoc_mark - 7) {    \
            ULONG next_cluster = GET_NEXT_CLUSTER(sb, cluster); \
            FreeCluster(sb, cluster);                           \
            cluster = next_cluster;                             \
        }                                                       \
    } while(0)

/* Release a chain without continuing after an entry update fails. */
static LONG FreeClusterChain(struct FSSuper *sb, ULONG cluster)
{
    ULONG count = 0;
    while (cluster >= 2 && cluster < sb->eoc_mark - 7)
    {
        ULONG next;
        if (cluster >= sb->clusters_count + 2 || ++count > sb->clusters_count)
            return ERROR_NOT_A_DOS_DISK;
        sb->fat_io_error = FALSE;
        next = GET_NEXT_CLUSTER(sb, cluster);
        if (sb->fat_io_error)
            return ERROR_UNKNOWN;
        if (next < 2)
            return ERROR_NOT_A_DOS_DISK;
        if (!FreeCluster(sb, cluster))
            return ERROR_UNKNOWN;
        cluster = next;
    }
    return 0;
}

/*
 * This takes a full path and moves to the directory that would contain the
 * last file in the path. E.g. calling with (dh, "foo/bar/baz", 11) will move
 * to directory "foo/bar" under the dir specified by dh. dh will become a
 * handle to the new dir. After the return, name will be "baz" and namelen
 * will be 3
 */
static LONG MoveToSubdir(struct DirHandle *dh, UBYTE **pname,
    ULONG *pnamelen, struct Globals *glob)
{
    LONG err;
    UBYTE *name = *pname, *base, ch, *p;
    ULONG namelen = *pnamelen, baselen;
    struct DirEntry de;

    /* Skip device name (if any) */
    for (ch = *(p = name); ch != ':' && ch != '\0'; ch = *(++p));
    if (ch == ':')
    {
        namelen -= (p - name) + 1;
        name = p + 1;
    }

    /* We break the given name into two pieces - the name of the containing
     * dir, and the name of the new dir to go within it. If the base ends up
     * empty, then we just use the dirlock */
    baselen = namelen;
    base = name;
    while (baselen > 0)
    {
        if (base[baselen - 1] != '/')
            break;
        baselen--;
    }
    while (baselen > 0)
    {
        if (base[baselen - 1] == '/')
            break;
        baselen--;
    }
    namelen -= baselen;
    name = &base[baselen];

    D(
        bug("[fat] base is '");
        RawPutChars(base, baselen); bug("', name is '");
        RawPutChars(name, namelen);
        bug("'\n");
    )

    if (baselen > 0)
    {
        if ((err = GetDirEntryByPath(dh, base, baselen, &de, glob)) != 0)
        {
            D(bug("[fat] base not found\n"));
            return err;
        }

        if ((err = InitDirHandle(dh->ioh.sb, FIRST_FILE_CLUSTER(&de), dh,
            TRUE, glob)) != 0)
            return err;
    }

    *pname = name;
    *pnamelen = namelen;

    return 0;
}

LONG OpLockFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    LONG access, struct ExtFileLock **filelock, struct Globals *glob)
{
    /* If they passed in a name, go searching for it */
    if (namelen != 0)
        return LockFileByName(dirlock, name, namelen, access, filelock,
            glob);

    /* Otherwise the empty filename, just make a copy */
    else if (dirlock != NULL)
        return CopyLock(dirlock, filelock, glob);

    /* Null dir lock means they want the root */
    else
        return LockRoot(access, filelock, glob);
}

void OpUnlockFile(struct ExtFileLock *lock, struct Globals *glob)
{
    if (lock != NULL)
        FreeLock(lock, glob);
}

LONG OpCopyLock(struct ExtFileLock *lock, struct ExtFileLock **copy,
    struct Globals *glob)
{
    if (lock != NULL)
        return CopyLock(lock, copy, glob);
    else
        return LockRoot(SHARED_LOCK, copy, glob);
}

LONG OpLockParent(struct ExtFileLock *lock, struct ExtFileLock **parent,
    struct Globals *glob)
{
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    ULONG parent_cluster;

    /* The root has no parent, but as a special case we have to return success
     * with the zero lock */
    if (lock == NULL || lock->gl == &glob->sb->info->root_lock)
    {
        *parent = NULL;
        return 0;
    }

    /* If we're in the root directory, then the root is our parent */
    if (lock->gl->dir_cluster == glob->sb->rootdir_cluster)
        return LockRoot(SHARED_LOCK, parent, glob);

    /* Get the parent dir */
    InitDirHandle(glob->sb, lock->gl->dir_cluster, &dh, FALSE, glob);
    if ((err = GetDirEntryByPath(&dh, "/", 1, &de, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* And its cluster */
    if ((parent_cluster = FIRST_FILE_CLUSTER(&de)) == 0)
        parent_cluster = glob->sb->rootdir_cluster;

    /* Then we go through the parent dir, looking for a link back to us. We do
     * this so that we have an entry with the proper name for copying by
     * LockFile() */
    InitDirHandle(glob->sb, parent_cluster, &dh, TRUE, glob);
    while ((err = GetDirEntry(&dh, dh.cur_index + 1, &de, glob)) == 0)
    {
        /* Don't go past the end */
        if (de.e.entry.name[0] == 0x00)
        {
            err = ERROR_OBJECT_NOT_FOUND;
            break;
        }

        /* We found it if it's not empty, and it's not the volume id or a long
         * name, and it is a directory, and it does point to us */
        if (de.e.entry.name[0] != 0xe5 &&
            !(de.e.entry.attr & ATTR_VOLUME_ID) &&
            de.e.entry.attr & ATTR_DIRECTORY &&
            FIRST_FILE_CLUSTER(&de) == lock->gl->dir_cluster)
        {
            err =
                LockFile(parent_cluster, dh.cur_index, SHARED_LOCK, parent,
                glob);
            break;
        }
    }

    ReleaseDirHandle(&dh, glob);
    return err;
}

/*
 * Obtains a lock on the named file under the given dir. This is the service
 * routine for DOS Open() (i.e. FINDINPUT/FINDOUTPUT/FINDUPDATE) and as such
 * may only return a lock on a file, never on a dir.
 */
LONG OpOpenFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    LONG action, struct ExtFileLock **filelock, struct Globals *glob)
{
    LONG err;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;

    D(
        bug("[fat] opening file '");
        RawPutChars(name, namelen);
        bug("' in dir at cluster %ld, action %s\n",
            dirlock != NULL ? dirlock->ioh.first_cluster : 0,
            action == ACTION_FINDINPUT ? "FINDINPUT" :
            action == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
            action == ACTION_FINDUPDATE ? "FINDUPDATE" : "[unknown]");
    )

    /* Explicitly mark the dirhandle as uninitialised */
    dh.ioh.sb = NULL;

    /* No filename means they're trying to open whatever dirlock is (which
     * despite the name may not actually be a dir). Since there's already an
     * extant lock, it's never going to be possible to get an exclusive lock,
     * so this will only work for FINDINPUT (read-only) */
    if (namelen == 0)
    {
        D(bug("[fat] trying to copy passed dir lock\n"));

        if (action != ACTION_FINDINPUT)
        {
            D(bug("[fat] can't copy lock for write (exclusive)\n"));
            return ERROR_OBJECT_IN_USE;
        }

        /* Dirs can't be opened */
        if (dirlock == NULL || dirlock->gl->attr & ATTR_DIRECTORY)
        {
            D(bug("[fat] dir lock is a directory, which can't be opened\n"));
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* It's a file, just copy the lock */
        return CopyLock(dirlock, filelock, glob);
    }

    /* Lock the file */
    err = LockFileByName(dirlock, name, namelen,
        action == ACTION_FINDINPUT ? SHARED_LOCK : EXCLUSIVE_LOCK, &lock,
        glob);

    /* Found it */
    if (err == 0)
    {
        D(bug("[fat] found existing file\n"));

        /* Can't open directories */
        if (lock->gl->attr & ATTR_DIRECTORY)
        {
            D(bug("[fat] it's a directory, can't open it\n"));
            FreeLock(lock, glob);
            return ERROR_OBJECT_WRONG_TYPE;
        }

        /* INPUT/UPDATE use the file as/is */
        if (action != ACTION_FINDOUTPUT)
        {
            D(bug("[fat] returning the lock\n"));
            *filelock = lock;
            return 0;
        }

        /* Whereas OUTPUT truncates it */
        D(bug("[fat] handling FINDOUTPUT, so truncating the file\n"));

        if (lock->gl->attr & ATTR_READ_ONLY)
        {
            D(bug("[fat] file is write protected, doing nothing\n"));
            FreeLock(lock, glob);
            return ERROR_WRITE_PROTECTED;
        }

        /* Update the dir entry to make the file empty */
        err = InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh,
            FALSE, glob);
        if (err != 0)
        {
            FreeLock(lock, glob);
            return err;
        }
        err = GetDirEntry(&dh, lock->gl->dir_entry, &de, glob);
        if (err != 0)
        {
            ReleaseDirHandle(&dh, glob);
            FreeLock(lock, glob);
            return err;
        }
        de.e.entry.first_cluster_lo = de.e.entry.first_cluster_hi = 0;
        de.e.entry.file_size = 0;
        de.e.entry.attr |= ATTR_ARCHIVE;
        err = UpdateDirEntry(&de, glob);
        ReleaseDirHandle(&dh, glob);
        if (err != 0)
        {
            FreeLock(lock, glob);
            return err;
        }

        D(bug("[fat] set first cluster and size to 0 in directory entry\n"));

        /* Free the clusters */
        FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);
        lock->gl->first_cluster = lock->ioh.first_cluster = 0xffffffff;
        RESET_HANDLE(&lock->ioh);
        lock->gl->size = 0;

        D(bug("[fat] file truncated, returning the lock\n"));

        /* File is empty, go */
        *filelock = lock;

        return 0;
    }

    /* Any error other than "not found" should be taken as-is */
    if (err != ERROR_OBJECT_NOT_FOUND)
        return err;

    /* Not found. For INPUT we bail out */
    if (action == ACTION_FINDINPUT)
    {
        D(bug("[fat] file not found, and not creating it\n"));
        return ERROR_OBJECT_NOT_FOUND;
    }

    D(
        bug("[fat] trying to create '");
        RawPutChars(name, namelen);
        bug("'\n");
    )

    /* Otherwise it's time to create the file. Get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb,
        dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, TRUE, glob))
        != 0)
        return err;

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* If the dir is write protected, can't do anything. Root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster)
    {
        err = GetDirEntry(&dh, 0, &de, glob);
        if (err != 0)
        {
            ReleaseDirHandle(&dh, glob);
            return err;
        }
        if (de.e.entry.attr & ATTR_READ_ONLY)
        {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh, glob);
            return ERROR_WRITE_PROTECTED;
        }
    }

    /* Create the entry */
    if ((err =
        CreateDirEntry(&dh, name, namelen, ATTR_ARCHIVE, 0, &de, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Lock the new file */
    err = LockFile(de.cluster, de.index, EXCLUSIVE_LOCK, filelock, glob);

    /* Done */
    ReleaseDirHandle(&dh, glob);

    if (err == 0)
    {
        (*filelock)->do_notify = TRUE;
        D(bug("[fat] returning lock on new file\n"));
    }

    return err;
}

/* Find the named file in the directory referenced by dirlock, and delete it.
 * If the file is a directory, it will only be deleted if it's empty */
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct Globals *glob)
{
    LONG err;
    struct ExtFileLock *lock;
    struct DirHandle dh;
    struct DirEntry de;

    D(
        bug("[fat] deleting file '");
        RawPutChars(name, namelen);
        bug("' in directory at cluster % ld\n",
            dirlock != NULL ? dirlock->ioh.first_cluster : 0);
    )

    dh.ioh.sb = NULL;


    /* Obtain a lock on the file. We need an exclusive lock as we don't want
     * to delete the file if it's in use */
    if ((err = LockFileByName(dirlock, name, namelen, EXCLUSIVE_LOCK, &lock,
        glob)) != 0)
    {
        D(bug("[fat] couldn't obtain exclusive lock on named file\n"));
        return err;
    }

    if (lock->gl->attr & ATTR_READ_ONLY)
    {
        D(bug("[fat] file is write protected, doing nothing\n"));
        FreeLock(lock, glob);
        return ERROR_DELETE_PROTECTED;
    }

    /* If it's a directory, we have to make sure it's empty */
    if (lock->gl->attr & ATTR_DIRECTORY)
    {
        D(bug("[fat] file is a directory, making sure it's empty\n"));

        if ((err = InitDirHandle(lock->ioh.sb, lock->ioh.first_cluster, &dh,
            FALSE, glob)) != 0)
        {
            FreeLock(lock, glob);
            return err;
        }

        /* Loop over the entries, starting from entry 2 (the first real
         * entry). Skipping unused ones, we look for the end-of-directory
         * marker. If we find it, the directory is empty. If we find a real
         * name, it's in use */
        de.index = 1;
        while ((err = GetDirEntry(&dh, de.index + 1, &de, glob)) == 0)
        {
            /* Skip unused entries */
            if (de.e.entry.name[0] == 0xe5)
                continue;

            /* End of directory, it's empty */
            if (de.e.entry.name[0] == 0x00)
                break;

            /* Otherwise the directory is still in use */
            D(bug("[fat] directory still has files in it, won't delete it\n"));

            ReleaseDirHandle(&dh, glob);
            FreeLock(lock, glob);
            return ERROR_DIRECTORY_NOT_EMPTY;
        }

        ReleaseDirHandle(&dh, glob);
        if (err != 0 && err != ERROR_OBJECT_NOT_FOUND)
        {
            FreeLock(lock, glob);
            return err;
        }
    }

    /* Open the containing directory */
    if ((err =InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh,
        TRUE, glob)) != 0)
    {
        FreeLock(lock, glob);
        return err;
    }

    /* If the dir is write protected, can't do anything. Root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster)
    {
        if ((err = GetDirEntry(&dh, 0, &de, glob)) != 0)
            goto delete_failed;
        if (de.e.entry.attr & ATTR_READ_ONLY)
        {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh, glob);
            FreeLock(lock, glob);
            return ERROR_WRITE_PROTECTED;
        }
    }

    /* Get the entry for the file */
    if ((err = GetDirEntry(&dh, lock->gl->dir_entry, &de, glob)) != 0)
        goto delete_failed;

    /* Release data only after the directory entry was removed successfully. */
    if ((err = DeleteDirEntry(&de, glob)) != 0)
        goto delete_failed;

    /* It's all good */
    ReleaseDirHandle(&dh, glob);

    /* Now free the clusters the file was using */
    FREE_CLUSTER_CHAIN(lock->ioh.sb, lock->ioh.first_cluster);

    /* Notify */
    SendNotifyByLock(lock->ioh.sb, lock->gl);

    /* This lock is now completely meaningless */
    FreeLock(lock, glob);

    D(
        bug("[fat] deleted '");
        RawPutChars(name, namelen);
        bug("'\n");
    )

    return 0;
delete_failed:
    ReleaseDirHandle(&dh, glob);
    FreeLock(lock, glob);
    return err;
}

LONG OpRenameFile(struct ExtFileLock *sdirlock, UBYTE *sname,
    ULONG snamelen, struct ExtFileLock *ddirlock, UBYTE *dname,
    ULONG dnamelen, struct Globals *glob)
{
    struct DirHandle sdh, ddh;
    struct DirEntry sde, dde;
    struct GlobalLock *gl;
    LONG err;
    ULONG len;

    /* Get the source dir handle */
    if ((err = InitDirHandle(glob->sb,
        sdirlock != NULL ? sdirlock->ioh.first_cluster : 0, &sdh,
        FALSE, glob)) != 0)
        return err;

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&sdh, &sname, &snamelen, glob)) != 0)
    {
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Get the entry */
    if ((err = GetDirEntryByName(&sdh, sname, snamelen, &sde, glob)) != 0)
    {
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Now get a handle on the passed dest dir */
    if ((err = InitDirHandle(glob->sb,
        ddirlock != NULL ? ddirlock->ioh.first_cluster : 0, &ddh,
        FALSE, glob)) != 0)
    {
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&ddh, &dname, &dnamelen, glob)) != 0)
    {
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Check the source and dest dirs. If either is read-only, do nothing */
    err = GetDirEntry(&sdh, 0, &dde, glob);
    if (err != 0)
    {
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }
    if (dde.e.entry.attr & ATTR_READ_ONLY)
    {
        D(bug("[fat] source dir is read only, doing nothing\n"));
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return ERROR_WRITE_PROTECTED;
    }
    err = GetDirEntry(&ddh, 0, &dde, glob);
    if (err != 0)
    {
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }
    if (dde.e.entry.attr & ATTR_READ_ONLY)
    {
        D(bug("[fat] dest dir is read only, doing nothing\n"));
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return ERROR_WRITE_PROTECTED;
    }

    /* Now see if the wanted name is in this dir. If it exists, and is not a capilazation change, do nothing */
    if ((err = GetDirEntryByName(&ddh, dname, dnamelen, &dde, glob)) == 0)
    {
        if ((dnamelen != snamelen) || (strnicmp(sname, dname, snamelen) != 0))
        {
            ReleaseDirHandle(&ddh, glob);
            ReleaseDirHandle(&sdh, glob);
            return ERROR_OBJECT_EXISTS;
        }
    }
    else if (err != ERROR_OBJECT_NOT_FOUND)
    {
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* At this point we have the source entry in sde, and we know the dest
     * doesn't exist or it is a capitalization change */

    /* XXX: if sdh and ddh are the same dir and there's room in the existing
     * entries for the new name, just overwrite the name */

    /* Make a new entry in the target dir */
    if ((err = CreateDirEntry(&ddh, dname, dnamelen,
        sde.e.entry.attr | ATTR_ARCHIVE,
        (sde.e.entry.first_cluster_hi << 16) | sde.e.entry.first_cluster_lo,
        &dde, glob)) != 0)
    {
        /* The new name could not be made (directory or volume full): keep
         * the original entry. Falling through would delete it below. */
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Copy in the leftover attributes */
    dde.e.entry.create_date = sde.e.entry.create_date;
    dde.e.entry.create_time = sde.e.entry.create_time;
    dde.e.entry.write_date = sde.e.entry.write_date;
    dde.e.entry.write_time = sde.e.entry.write_time;
    dde.e.entry.last_access_date = sde.e.entry.last_access_date;
    dde.e.entry.create_time_tenth = sde.e.entry.create_time_tenth;
    dde.e.entry.file_size = sde.e.entry.file_size;

    err = UpdateDirEntry(&dde, glob);
    if (err != 0)
    {
        DeleteDirEntry(&dde, glob);
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Keep the original lock until its directory entry is gone. */
    err = DeleteDirEntry(&sde, glob);
    if (err != 0)
    {
        DeleteDirEntry(&dde, glob);
        ReleaseDirHandle(&ddh, glob);
        ReleaseDirHandle(&sdh, glob);
        return err;
    }

    /* Update the global lock (if present) with the new dir cluster/entry */
    ForeachNode(&sdh.ioh.sb->info->locks, gl)
    {
        if (gl->dir_cluster == sde.cluster && gl->dir_entry == sde.index)
        {
            D(bug("[fat] found lock with old dir entry (%ld/%ld),"
                " changing to (%ld/%ld)\n",
                sde.cluster, sde.index, dde.cluster, dde.index));

            gl->dir_cluster = dde.cluster;
            gl->dir_entry = dde.index;

            /* Update the filename too */
            GetDirEntryShortName(&dde, &(gl->name[1]), &len, glob);
            gl->name[0] = (UBYTE) len;
            GetDirEntryLongName(&dde, &(gl->name[1]), &len);
            gl->name[0] = (UBYTE) len;
        }
    }

    /* Notify */
    SendNotifyByDirEntry(sdh.ioh.sb, &dde);

    ReleaseDirHandle(&ddh, glob);
    ReleaseDirHandle(&sdh, glob);

    return 0;
}

LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct ExtFileLock **newdirlock, struct Globals *glob)
{
    LONG err, i;
    ULONG cluster;
    struct DirHandle dh, sdh;
    struct DirEntry de, sde;

    D(
        bug("[fat] creating directory '");
        RawPutChars(name, namelen);
        bug("' in directory at cluster %ld\n",
            dirlock != NULL ? dirlock->ioh.first_cluster : 0);
    )

    /* Get a handle on the passed dir */
    if ((err = InitDirHandle(glob->sb,
        dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE,
        glob)) != 0)
        return err;

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Make sure 'name' is just the FilePart() */
    for (i = namelen - 1; i > 0; i--)
    {
        if (name[i] == '/' || name[i] == ':')
        {
            namelen -= (i + 1);
            name += (i + 1);
            break;
        }
    }

    /* If the dir is write protected, can't do anything. Root dir is never
     * write protected */
    if (dh.ioh.first_cluster != dh.ioh.sb->rootdir_cluster)
    {
        err = GetDirEntry(&dh, 0, &de, glob);
        if (err != 0)
        {
            ReleaseDirHandle(&dh, glob);
            return err;
        }
        if (de.e.entry.attr & ATTR_READ_ONLY)
        {
            D(bug("[fat] containing dir is write protected, doing nothing\n"));
            ReleaseDirHandle(&dh, glob);
            return ERROR_WRITE_PROTECTED;
        }
    }

    /* Now see if the wanted name is in this dir. If it exists, then we do
     * nothing */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de, glob)) == 0)
    {
        D(bug("[fat] name exists, can't do anything\n"));
        ReleaseDirHandle(&dh, glob);
        return ERROR_OBJECT_EXISTS;
    }

    if (err != ERROR_OBJECT_NOT_FOUND)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Find a free cluster to store the dir in */
    if ((err = FindFreeCluster(dh.ioh.sb, &cluster)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Allocate it */
    if (!AllocCluster(dh.ioh.sb, cluster))
    {
        ReleaseDirHandle(&dh, glob);
        return ERROR_UNKNOWN;
    }

    D(bug("[fat] allocated cluster %ld for directory\n", cluster));

    /* Create the entry, pointing to the new cluster */
    if ((err = CreateDirEntry(&dh, name, namelen,
        ATTR_DIRECTORY | ATTR_ARCHIVE, cluster, &de, glob)) != 0)
    {
        /* Deallocate the cluster */
        FreeCluster(dh.ioh.sb, cluster);

        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Now get a handle on the new directory */
    InitDirHandle(dh.ioh.sb, cluster, &sdh, FALSE, glob);

    /* Create the dot entry. It's a direct copy of the just-created entry, but
     * with a different name */
    if ((err = GetDirEntry(&sdh, 0, &sde, glob)) != 0)
        goto create_failed;
    CopyMem(&de.e.entry, &sde.e.entry, sizeof(struct FATDirEntry));
    CopyMem(".          ", &sde.e.entry.name, FAT_MAX_SHORT_NAME);
    if ((err = UpdateDirEntry(&sde, glob)) != 0)
        goto create_failed;

    /* Create the dot-dot entry. Again, a copy, with the cluster pointer set
     * up to point to the parent */
    if ((err = GetDirEntry(&sdh, 1, &sde, glob)) != 0)
        goto create_failed;
    CopyMem(&de.e.entry, &sde.e.entry, sizeof(struct FATDirEntry));
    CopyMem("..         ", &sde.e.entry.name, FAT_MAX_SHORT_NAME);
    cluster = dh.ioh.first_cluster;
    if (cluster == dh.ioh.sb->rootdir_cluster)
        cluster = 0;
    sde.e.entry.first_cluster_lo = AROS_WORD2LE(cluster & 0xffff);
    sde.e.entry.first_cluster_hi = AROS_WORD2LE(cluster >> 16);
    if ((err = UpdateDirEntry(&sde, glob)) != 0)
        goto create_failed;

    /* Clear all remaining entries (the first of which marks the end of the
     * directory) */
    for (i = 2; (err = GetDirEntry(&sdh, i, &sde, glob)) == 0; i++)
    {
        SetMem(&sde.e.entry, 0, sizeof(struct FATDirEntry));
        if ((err = UpdateDirEntry(&sde, glob)) != 0)
            goto create_failed;
    }

    if (err != ERROR_OBJECT_NOT_FOUND)
        goto create_failed;

    /* New dir created */
    ReleaseDirHandle(&sdh, glob);

    /* Now obtain a lock on the new dir */
    err = LockFile(de.cluster, de.index, SHARED_LOCK, newdirlock, glob);

    /* Done */
    ReleaseDirHandle(&dh, glob);

    /* Notify */
    if (err == 0)
        SendNotifyByLock((*newdirlock)->ioh.sb, (*newdirlock)->gl);

    return err;

create_failed:
    cluster = sdh.ioh.first_cluster;
    ReleaseDirHandle(&sdh, glob);
    /* A failed unlink may have changed part of the entry: retain its data. */
    if (DeleteDirEntry(&de, glob) == 0)
        FreeCluster(dh.ioh.sb, cluster);
    ReleaseDirHandle(&dh, glob);
    return err;
}

LONG OpRead(struct ExtFileLock *lock, UBYTE *data, ULONG want,
    ULONG *read, struct Globals *glob)
{
    LONG err;

    D(bug("[fat] request to read %ld bytes from file pos %ld\n", want,
        lock->pos));

    if (want == 0)
        return 0;

    if (want + lock->pos > lock->gl->size)
    {
        want = lock->gl->size - lock->pos;
        D(bug("[fat] full read would take us past end-of-file,"
            " adjusted want to %ld bytes\n", want));
    }

    if ((err = ReadFileChunk(&(lock->ioh), lock->pos, want, data, read)) == 0)
    {
        lock->pos += *read;
        D(bug("[fat] read %ld bytes, new file pos is %ld\n", *read,
            lock->pos));
    }

    return err;
}

LONG OpWrite(struct ExtFileLock *lock, UBYTE *data, ULONG want,
    ULONG *written, struct Globals *glob)
{
    LONG err;
    ULONG size;
    BOOL update_entry = FALSE;
    struct DirHandle dh;
    struct DirEntry de;

    D(bug("[fat] request to write %ld bytes to file pos %ld\n", want,
        lock->pos));

    /* Need an exclusive lock */
    if (lock->gl->access != EXCLUSIVE_LOCK)
    {
        D(bug("[fat] can't modify global attributes via a shared lock\n"));
        return ERROR_OBJECT_IN_USE;
    }

    /* Don't modify the file if it's protected */
    if (lock->gl->attr & ATTR_READ_ONLY)
    {
        D(bug("[fat] file is write protected\n"));
        return ERROR_WRITE_PROTECTED;
    }

    if (want == 0)
    {
        *written = 0;
        return 0;
    }

    /* If this is the first write, make a note as we'll have to store the
     * first cluster in the directory entry later */
    if (lock->ioh.first_cluster == 0xffffffff)
        update_entry = TRUE;

    if ((err = WriteFileChunk(&(lock->ioh), lock->pos, want, data,
        written)) == 0)
    {
        /* If nothing was written but success was returned (can that even
         * happen?) then we don't want to mess with the dir entry */
        if (*written == 0)
        {
            D(bug("[fat] nothing successfully written (!),"
                " nothing else to do\n"));
            return 0;
        }

        /* Something changed, we need to tell people about it */
        lock->do_notify = TRUE;

        /* Move to the end of the area written */
        lock->pos += *written;

        /* Update the dir entry if the size changed */
        size = lock->gl->size;
        if (lock->pos > size)
        {
            size = lock->pos;
            update_entry = TRUE;
        }

        /* Force an update if the file hasn't already got an archive bit. This
         * will happen if this was the first write to an existing file that
         * didn't cause it to grow */
        else if (!(lock->gl->attr & ATTR_ARCHIVE))
            update_entry = TRUE;

        D(bug("[fat] wrote %ld bytes, new file pos is %ld, size is %ld\n",
            *written, lock->pos, size));

        if (update_entry)
        {
            D(bug("[fat] updating dir entry, first cluster is %ld,"
                " size is %ld\n",
                lock->ioh.first_cluster, size));

            err = InitDirHandle(lock->ioh.sb, lock->gl->dir_cluster, &dh,
                FALSE, glob);
            if (err != 0)
                return err;
            err = GetDirEntry(&dh, lock->gl->dir_entry, &de, glob);
            if (err != 0)
            {
                ReleaseDirHandle(&dh, glob);
                return err;
            }

            de.e.entry.file_size = AROS_LONG2LE(size);
            de.e.entry.first_cluster_lo =
                AROS_WORD2LE(lock->ioh.first_cluster & 0xffff);
            de.e.entry.first_cluster_hi =
                AROS_WORD2LE(lock->ioh.first_cluster >> 16);

            de.e.entry.attr |= ATTR_ARCHIVE;
            err = UpdateDirEntry(&de, glob);

            ReleaseDirHandle(&dh, glob);
            if (err != 0)
                return err;
            lock->gl->first_cluster = lock->ioh.first_cluster;
        }
        lock->gl->size = size;
    }

    return err;
}

LONG OpSetFileSize(struct ExtFileLock *lock, LONG offset, LONG whence,
    LONG *newsize, struct Globals *glob)
{
    struct FSSuper *sb = glob->sb;
    struct DirHandle dh;
    struct DirEntry de;
    QUAD size;
    LONG err;
    ULONG first, cl, next, count = 0, want, keep = 0, tail = 0;
    ULONG keep_next = 0;
    ULONG added = 0, added_last = 0, last = 0, original_next = 0;
    BOOL linked = FALSE;

    if (lock->gl->access != EXCLUSIVE_LOCK)
        return ERROR_OBJECT_IN_USE;
    if (lock->gl->attr & ATTR_READ_ONLY)
        return ERROR_WRITE_PROTECTED;
    if (whence == OFFSET_BEGINNING)
        size = offset;
    else if (whence == OFFSET_CURRENT)
        size = (QUAD)lock->pos + offset;
    else if (whence == OFFSET_END && offset <= 0)
        size = (QUAD)lock->gl->size + offset;
    else
        return ERROR_SEEK_ERROR;
    if (size < 0 || size > 0x7fffffff)
        return ERROR_SEEK_ERROR;
    if (size == lock->gl->size)
    {
        *newsize = size;
        return 0;
    }
    err = InitDirHandle(sb, lock->gl->dir_cluster, &dh, FALSE, glob);
    if (err != 0)
        return err;
    err = GetDirEntry(&dh, lock->gl->dir_entry, &de, glob);
    if (err != 0)
        goto done;
    want = ((ULONG)size >> sb->clustersize_bits) +
        (((ULONG)size & (sb->clustersize - 1)) != 0);
    first = FIRST_FILE_CLUSTER(&de);
    if (first != 0 &&
        (first < 2 || first >= sb->eoc_mark - 7 ||
         first >= sb->clusters_count + 2))
    {
        err = ERROR_NOT_A_DOS_DISK;
        goto done;
    }
    cl = first;
    while (cl >= 2 && cl < sb->eoc_mark - 7)
    {
        if (cl >= sb->clusters_count + 2 || ++count > sb->clusters_count)
        {
            err = ERROR_NOT_A_DOS_DISK;
            goto done;
        }
        if (count == want)
            keep = cl;
        last = cl;
        sb->fat_io_error = FALSE;
        cl = GET_NEXT_CLUSTER(sb, cl);
        if (sb->fat_io_error)
        {
            err = ERROR_UNKNOWN;
            goto done;
        }
        if (count == want)
            keep_next = cl;
        if (cl < 2)
        {
            err = ERROR_NOT_A_DOS_DISK;
            goto done;
        }
    }
    original_next = cl;

    /* Build growth separately so allocation failure leaves the original chain. */
    while (count < want)
    {
        err = FindFreeCluster(sb, &next);
        if (err != 0)
            goto rollback;
        if (!AllocCluster(sb, next))
        {
            err = ERROR_UNKNOWN;
            goto rollback;
        }
        if (added_last != 0 && !SET_NEXT_CLUSTER(sb, added_last, next))
        {
            err = ERROR_UNKNOWN;
            /* A failed mirror write may already have changed the first FAT. */
            if (!SET_NEXT_CLUSTER(sb, added_last, sb->eoc_mark))
                goto done;
            FreeCluster(sb, next);
            goto rollback;
        }
        if (added == 0)
            added = next;
        added_last = next;
        count++;
    }
    if (added != 0 && first >= 2)
    {
        /* Restore this link even if only some FAT copies were updated. */
        linked = TRUE;
        if (!SET_NEXT_CLUSTER(sb, last, added))
        {
            err = ERROR_UNKNOWN;
            goto rollback;
        }
    }
    if (want == 0)
        tail = first;
    else if (keep != 0 && count > want)
        tail = keep_next;
    if (first < 2)
        first = added;
    if (want == 0)
        first = 0;
    de.e.entry.first_cluster_lo = AROS_WORD2LE(first & 0xffff);
    de.e.entry.first_cluster_hi = AROS_WORD2LE(first >> 16);
    de.e.entry.file_size = AROS_LONG2LE((ULONG)size);
    de.e.entry.attr |= ATTR_ARCHIVE;
    err = UpdateDirEntry(&de, glob);
    if (err != 0)
        goto rollback;

    lock->gl->size = size;
    lock->gl->first_cluster = lock->ioh.first_cluster = first ? first : 0xffffffff;
    RESET_HANDLE(&lock->ioh);
    *newsize = size;
    if (tail >= 2 && tail < sb->eoc_mark - 7)
    {
        if (keep != 0 && !SET_NEXT_CLUSTER(sb, keep, sb->eoc_mark))
            err = ERROR_UNKNOWN;
        else
            err = FreeClusterChain(sb, tail);
    }
    goto done;

rollback:
    /* If restoring a link fails, keep its allocated data rather than free it. */
    if (linked && !SET_NEXT_CLUSTER(sb, last, original_next))
        goto done;
    if (added != 0)
        FreeClusterChain(sb, added);
done:
    ReleaseDirHandle(&dh, glob);
    return err;
}

LONG OpSetProtect(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    ULONG prot, struct Globals *glob)
{
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;

    /* Get the dir handle */
    if ((err = InitDirHandle(glob->sb,
        dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE,
        glob)) != 0)
        return err;

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Can't change permissions on the root */
    if (dh.ioh.first_cluster == dh.ioh.sb->rootdir_cluster && namelen == 0)
    {
        D(bug("[fat] can't set protection on root dir\n"));
        ReleaseDirHandle(&dh, glob);
        return ERROR_INVALID_LOCK;
    }

    /* Get the entry */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Set the attributes */
    de.e.entry.attr &= ~(ATTR_ARCHIVE | ATTR_READ_ONLY);
    de.e.entry.attr |= (prot & FIBF_ARCHIVE ? ATTR_ARCHIVE : 0);

    /* Only set read-only if neither writable nor deletable */
    if ((prot & (FIBF_WRITE | FIBF_DELETE)) == (FIBF_WRITE | FIBF_DELETE))
        de.e.entry.attr |= ATTR_READ_ONLY;
    if ((err = UpdateDirEntry(&de, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    D(bug("[fat] new protection is 0x%08x\n", de.e.entry.attr));

    SendNotifyByDirEntry(glob->sb, &de);

    /* If it's a directory, we also need to update the protections for the
     * directory's . entry */
    if (de.e.entry.attr & ATTR_DIRECTORY)
    {
        ULONG attr = de.e.entry.attr;

        D(bug("[fat] setting protections for directory '.' entry\n"));

        err = InitDirHandle(glob->sb, FIRST_FILE_CLUSTER(&de), &dh, TRUE, glob);
        if (err == 0)
            err = GetDirEntry(&dh, 0, &de, glob);
        if (err == 0)
        {
            de.e.entry.attr = attr;
            err = UpdateDirEntry(&de, glob);
        }
    }

    ReleaseDirHandle(&dh, glob);

    return err;
}

LONG OpSetDate(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen,
    struct DateStamp *ds, struct Globals *glob)
{
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    UWORD wdate, wtime;

    /* Get the dir handle */
    if ((err = InitDirHandle(glob->sb,
        dirlock != NULL ? dirlock->ioh.first_cluster : 0, &dh, FALSE,
        glob)) != 0)
        return err;

    /* Get down to the correct subdir */
    if ((err = MoveToSubdir(&dh, &name, &namelen, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Can't set date on the root */
    if (dh.ioh.first_cluster == dh.ioh.sb->rootdir_cluster && namelen == 0)
    {
        D(bug("[fat] can't set date on root dir\n"));
        ReleaseDirHandle(&dh, glob);
        return ERROR_INVALID_LOCK;
    }

    /* Get the entry */
    if ((err = GetDirEntryByName(&dh, name, namelen, &de, glob)) != 0)
    {
        ReleaseDirHandle(&dh, glob);
        return err;
    }

    /* Set and update the date */
    wdate = de.e.entry.write_date;
    wtime = de.e.entry.write_time;
    ConvertDOSDate(ds, &wdate, &wtime,
        glob);
    de.e.entry.write_date = wdate;
    de.e.entry.write_time = wtime;
    de.e.entry.last_access_date = wdate;
    err = UpdateDirEntry(&de, glob);
    if (err == 0)
        SendNotifyByDirEntry(glob->sb, &de);

    ReleaseDirHandle(&dh, glob);

    return err;
}

LONG OpAddNotify(struct NotifyRequest *nr, struct Globals *glob)
{
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    struct GlobalLock *gl = NULL, *tmp;
    struct NotifyNode *nn;
    BOOL exists = FALSE;

    D(bug("[fat] trying to add notification for '%s'\n", nr->nr_FullName));

    /* If the request is for the volume root, then we just link to the root
     * lock */
    if (nr->nr_FullName[strlen(nr->nr_FullName) - 1] == ':')
    {
        D(bug("[fat] adding notify for root dir\n"));
        gl = &glob->sb->info->root_lock;
    }

    else
    {
        if ((err = InitDirHandle(glob->sb, 0, &dh, FALSE, glob)) != 0)
            return err;

        /* Look for the entry */
        err =
            GetDirEntryByPath(&dh, nr->nr_FullName, strlen(nr->nr_FullName),
            &de, glob);
        if (err != 0 && err != ERROR_OBJECT_NOT_FOUND)
            return err;

        /* If it was found, then it might be open. try to find the global
         * lock */
        if (err == 0)
        {
            exists = TRUE;

            D(bug("[fat] file exists (%ld/%ld), looking for global lock\n",
                de.cluster, de.index));

            ForeachNode(&glob->sb->info->locks, tmp)
            {
                if (tmp->dir_cluster == de.cluster
                    && tmp->dir_entry == de.index)
                {
                    gl = tmp;

                    D(bug("[fat] found global lock 0x%0x\n", gl));

                    break;
                }
            }

        }
        else
        {
            exists = FALSE;

            D(bug("[fat] file doesn't exist\n"));
        }
    }

    if (gl == NULL)
        D(bug("[fat] file not currently locked\n"));

    /* Allocate space for the notify node */
    if ((nn = AllocVecPooled(glob->sb->info->mem_pool,
        sizeof(struct NotifyNode))) == NULL)
        return ERROR_NO_FREE_STORE;

    /* Plug the bits in */
    nn->gl = gl;
    nn->nr = nr;

    /* Add to the list */
    ADDTAIL(&glob->sb->info->notifies, nn);

    /* Tell them that the file exists if they wanted to know */
    if (exists && nr->nr_Flags & NRF_NOTIFY_INITIAL)
        SendNotify(nr, glob);

    D(bug("[fat] now reporting activity on '%s'\n", nr->nr_FullName));

    return 0;
}

LONG OpRemoveNotify(struct NotifyRequest *nr, struct Globals *glob)
{
    struct FSSuper *sb;
    struct NotifyNode *nn, *nn2;

    D(bug("[fat] trying to remove notification for '%s'\n",
        nr->nr_FullName));

    /* Search inserted volume for the request */
    if (glob->sb != NULL)
    {
        ForeachNodeSafe(&glob->sb->info->notifies, nn, nn2)
        {
            if (nn->nr == nr)
            {
                D(bug("[fat] found notify request in list, removing it\n"));
                REMOVE(nn);
                FreeVecPooled(glob->sb->info->mem_pool, nn);
                return 0;
            }
        }
    }

    /* Search offline volumes for the request */
    ForeachNode(&glob->sblist, sb)
    {
        ForeachNodeSafe(&sb->info->notifies, nn, nn2)
        {
            if (nn->nr == nr)
            {
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
