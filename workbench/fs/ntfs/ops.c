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

#define AROS_ALMOST_COMPATIBLE

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/exec.h>

#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include "ntfs_fs.h"
#include "ntfs_protos.h"
#include "support.h"

#include "debug.h"

LONG OpLockFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, LONG access, struct ExtFileLock **filelock)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    // if they passed in a name, go searching for it
    if (namelen != 0)
        return LockFileByName(dirlock, name, namelen, access, filelock);

    // otherwise the empty filename, just make a copy
    else if (dirlock != NULL)
        return CopyLock(dirlock, filelock);

    // null dir lock means they want the root
    else
        return LockRoot(access, filelock);
}

void OpUnlockFile(struct ExtFileLock *lock)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (lock != NULL)
        FreeLock(lock);
}

LONG OpCopyLock(struct ExtFileLock *lock, struct ExtFileLock **copy)
{
    D(bug("[NTFS]: %s(lock @ 0x%p)\n", __PRETTY_FUNCTION__, lock));

    if (lock != NULL)
        return CopyLock(lock, copy);
    else
        return LockRoot(SHARED_LOCK, copy);
}

LONG OpLockParent(struct ExtFileLock *lock, struct ExtFileLock **parent)
{
    LONG err;
    struct DirEntry de;
    struct DirHandle dh;
    struct NTFSMFTAttr dirattr;
    struct MFTAttr *attrentry;

    D(bug("[NTFS]: %s(lock @ 0x%p)\n", __PRETTY_FUNCTION__, lock));


    // the root has no parent, but as a special case we have to return success
    // with the zero lock
    if (lock == NULL || lock->gl == &glob->data->info->root_lock) {
        *parent = NULL;
        return 0;
    }

    // if we're in the root directory, then the root is our parent
    if (lock->gl->dir_cluster == glob->data->info->root_lock.dir_cluster)
        return LockRoot(SHARED_LOCK, parent);

    memset(&de, 0, sizeof(struct DirEntry));
    memset(&dh, 0, sizeof(struct DirHandle));

    // get the parent dir
    if (lock->gl->attr & ATTR_DIRECTORY)
    {
	dh.ioh.mft.mftrec_no = lock->dir->ioh.mft.mftrec_no;
	InitDirHandle(glob->data, &dh, FALSE);

	if ((err = GetDirEntryByPath(&dh, "/", 1, &de)) != 0) {
	    return err;
	}
    }
    else
    {
	dh.ioh.mft.mftrec_no = lock->gl->dir_cluster / glob->data->mft_size;
	InitDirHandle(glob->data, &dh, FALSE);

	INIT_MFTATTRIB(&dirattr, &dh.ioh.mft);
	attrentry = FindMFTAttrib(&dirattr, AT_FILENAME);
	attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(*((UWORD *)(attrentry + 0x14))));

	// take us up
	dh.ioh.mft.mftrec_no = AROS_LE2QUAD(*((UQUAD *)attrentry)) & MFTREF_MASK;
	if (dh.ioh.mft.mftrec_no == 0x2)
		dh.ioh.mft.mftrec_no = FILE_ROOT;
	dh.ioh.first_cluster = dh.ioh.mft.mftrec_no * glob->data->mft_size;
	D(bug("[NTFS] %s: parent_mft = %d [%d]\n", __PRETTY_FUNCTION__, (dh.ioh.first_cluster / glob->data->mft_size), dh.ioh.mft.mftrec_no));
	ReleaseDirHandle(&dh);
	InitDirHandle(dh.ioh.data, &dh, TRUE);
	
	if ((err = GetDirEntryByCluster(&dh, lock->gl->dir_cluster, &de)) != 0) {
	    return err;
	}
    }

    D(bug("[NTFS] %s: found parent!\n", __PRETTY_FUNCTION__));

    err = LockFile(&de, SHARED_LOCK, parent);

    return err;
}

/*
 * obtains a lock on the named file under the given dir. this is the service
 * routine for DOS Open() (ie FINDINPUT/FINDOUTPUT/FINDUPDATE) and as such may
 * only return a lock on a file, never on a dir.
 */
LONG OpOpenFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, LONG action, struct ExtFileLock **filelock)
{
    LONG err;
    struct ExtFileLock *lock;

    D(bug("[NTFS]: %s('", __PRETTY_FUNCTION__); RawPutChars(name, namelen); bug("')\n"));
    D(bug("[NTFS] %s: action = %s\n", __PRETTY_FUNCTION__,
          action == ACTION_FINDINPUT  ? "FINDINPUT"  :
          action == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
          action == ACTION_FINDUPDATE ? "FINDUPDATE" : "[unknown]"));

    // no filename means they're trying to open whatever dirlock is (which
    // despite the name may not actually be a dir). since there's already an
    // extant lock, it's never going to be possible to get an exclusive lock,
    // so this will only work for FINDINPUT (read-only)
    if (namelen == 0) {
        D(bug("[NTFS] %s: trying to copy passed dir lock\n", __PRETTY_FUNCTION__));

        if (action != ACTION_FINDINPUT) {
            D(bug("[NTFS] %s: can't copy lock for write (exclusive)\n", __PRETTY_FUNCTION__));
            return ERROR_OBJECT_IN_USE;
        }

        // dirs can't be opened
        if (dirlock == NULL || dirlock->gl->attr & ATTR_DIRECTORY) {
            D(bug("[NTFS] %s: dir lock is a directory, which can't be opened\n", __PRETTY_FUNCTION__));
            return ERROR_OBJECT_WRONG_TYPE;
        }

        // it's a file, just copy the lock
        return CopyLock(dirlock, filelock);
    }

    // lock the file
    err = LockFileByName(dirlock, name, namelen, action == ACTION_FINDINPUT ? SHARED_LOCK : EXCLUSIVE_LOCK, &lock);

    // found it
    if (err == 0) {
        D(bug("[NTFS] %s: found existing file\n", __PRETTY_FUNCTION__));

        // can't open directories
        if (lock->gl->attr & ATTR_DIRECTORY) {
            D(bug("[NTFS] %s: it's a directory, can't open it\n", __PRETTY_FUNCTION__));
            FreeLock(lock);
            return ERROR_OBJECT_WRONG_TYPE;
        }

        // INPUT/UPDATE use the file as/is
        if (action != ACTION_FINDOUTPUT) {
            D(bug("[NTFS] %s: returning the lock\n", __PRETTY_FUNCTION__));
            *filelock = lock;
            return 0;
        }

        // whereas OUTPUT truncates it
        D(bug("[NTFS] %s: handling FINDOUTPUT, so truncating the file\n", __PRETTY_FUNCTION__));

        if (lock->gl->attr & ATTR_READ_ONLY) {
            D(bug("[NTFS] %s: file is write protected, doing nothing\n", __PRETTY_FUNCTION__));
            FreeLock(lock);
            return ERROR_WRITE_PROTECTED;
        }

        // update the dir entry to make the file empty
        UpdateDirEntry(lock->entry);

        D(bug("[NTFS] %s: set first cluster and size to 0 in directory entry\n", __PRETTY_FUNCTION__));

        // free the clusters
        lock->gl->first_cluster = lock->dir->ioh.first_cluster = 0xffffffff;
        RESET_HANDLE(&lock->dir->ioh);
        lock->gl->size = 0;

        D(bug("[NTFS] %s: file truncated, returning the lock\n", __PRETTY_FUNCTION__));

        // file is empty, go
        *filelock = lock;

        return 0;
    }

    // any error other than "not found" should be taken as-is
    if (err != ERROR_OBJECT_NOT_FOUND)
        return err;

    // not found. for INPUT we bail out
    if (action == ACTION_FINDINPUT) {
        D(bug("[NTFS] %s: file not found, and not creating it\n", __PRETTY_FUNCTION__));
        return ERROR_OBJECT_NOT_FOUND;
    }

    D(bug("[NTFS] %s: trying to create '", __PRETTY_FUNCTION__); RawPutChars(name, namelen); bug("'\n"));

    if (err == 0) {
        (*filelock)->do_notify = TRUE;
        D(bug("[NTFS] %s: returning lock on new file\n", __PRETTY_FUNCTION__));
    }

    return err;
}

/* find the named file in the directory referenced by dirlock, and delete it.
 * if the file is a directory, it will only be deleted if it's empty */
LONG OpDeleteFile(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen)
{
    LONG err = 0;

    D(bug("[NTFS]: %s('", __PRETTY_FUNCTION__); RawPutChars(name, namelen); bug("')\n"));
    
#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif

    return err;
}

LONG OpRenameFile(struct ExtFileLock *sdirlock, UBYTE *sname, ULONG snamelen, struct ExtFileLock *ddirlock, UBYTE *dname, ULONG dnamelen)
{
    LONG err = 0;
    
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif

    return err;
}

LONG OpCreateDir(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct ExtFileLock **newdirlock)
{
    LONG err = 0;

    D(bug("[NTFS]: %s('", __PRETTY_FUNCTION__); RawPutChars(name, namelen); bug("')\n"));

#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif

    return err;
}

LONG OpRead(struct ExtFileLock *lock, UBYTE *data, UQUAD want, UQUAD *read)
{
    LONG err = 0;
    struct NTFSMFTAttr dataatrr;
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));
    D(bug("[NTFS] %s: %u bytes, pos %u\n", __PRETTY_FUNCTION__, (unsigned int)want, (unsigned int)lock->pos));

    if (want == 0)
        return 0;

    if (want + lock->pos > lock->gl->size) {
        want = lock->gl->size - lock->pos;
        D(bug("[NTFS] %s: full read would take us past end-of-file, adjusted want to %u bytes\n", __PRETTY_FUNCTION__, (unsigned int)want));
    }

    INIT_MFTATTRIB(&dataatrr, lock->entry->entry);
    if (MapMFTAttrib (&dataatrr, lock->entry->entry, AT_DATA))
    {
	if (ReadMFTAttrib(&dataatrr, data, lock->pos, want, 0) == 0)
	{
	    *read = want;
	    lock->pos = lock->pos + want;
	    D(bug("[NTFS] %s: read %u bytes, new file pos is %u\n", __PRETTY_FUNCTION__, (unsigned int)want, (unsigned int)lock->pos));
	}
    }
    return err;
}

LONG OpWrite(struct ExtFileLock *lock, UBYTE *data, UQUAD want, UQUAD *written)
{
    LONG err = 0;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));
#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else
    D(bug("[NTFS] %s: %ld bytes, pos %ld\n", __PRETTY_FUNCTION__, want, lock->pos));
#endif

    return err;
}

LONG OpSetFileSize(struct ExtFileLock *lock, UQUAD offset, LONG offsetfrom, UQUAD *newsize)
{
    LONG err = 0;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));
#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif
    return err;
}

LONG OpSetProtect(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, ULONG prot)
{
    LONG err = 0;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));
#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif
    return err;
}

LONG OpSetDate(struct ExtFileLock *dirlock, UBYTE *name, ULONG namelen, struct DateStamp *ds)
{
    LONG err = 0;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));
#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else

#endif
    return err;
}

LONG OpAddNotify(struct NotifyRequest *nr)
{
    LONG err;
    struct DirHandle dh;
    struct DirEntry de;
    struct GlobalLock *gl = NULL, *tmp;
    struct NotifyNode *nn;
    BOOL exists = FALSE;

    D(bug("[NTFS]: %s('%s')\n", __PRETTY_FUNCTION__, nr->nr_FullName));

    // if the request is for the volume root, then we just link to the root lock
    if (nr->nr_FullName[strlen(nr->nr_FullName)-1] == ':')
    {
        D(bug("[NTFS] %s: adding notify for root dir\n", __PRETTY_FUNCTION__));
        gl = &glob->data->info->root_lock;
    }
    else {
	dh.ioh.mft.mftrec_no = FILE_ROOT;
	dh.ioh.mft.buf = NULL;
	if ((err = InitDirHandle(glob->data, &dh, FALSE)) != 0)
        return err;

	memset(&de, 0, sizeof(struct DirEntry));

        // look for the entry
        err = GetDirEntryByPath(&dh, nr->nr_FullName, strlen(nr->nr_FullName), &de);
        if (err != 0 && err != ERROR_OBJECT_NOT_FOUND)
            return err;

        // if it was found, then it might be open. try to find the global lock
        if (err == 0) {
            exists = TRUE;

            D(bug("[NTFS] %s: file exists (%ld/%ld), looking for global lock\n", __PRETTY_FUNCTION__, de.cluster, de.no));

            ForeachNode(&glob->data->info->locks, tmp)
                if (tmp->dir_cluster == de.cluster && tmp->dir_entry == de.no) {
                    gl = tmp;

                    D(bug("[NTFS] %s: global lock 0x%0x\n", __PRETTY_FUNCTION__, gl));

                    break;
                }

        }
        else {
            exists = FALSE;

            D(bug("[NTFS] %s: file doesn't exist\n", __PRETTY_FUNCTION__));
        }
    }

    if (gl == NULL)
    {
        D(bug("[NTFS] %s: file not currently locked\n", __PRETTY_FUNCTION__));
    }

    // allocate space for the notify node
    if ((nn = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct NotifyNode))) == NULL)
        return ERROR_NO_FREE_STORE;

    // plug the bits in
    nn->gl = gl;
    nn->nr = nr;

    // add to the list
    ADDTAIL(&glob->data->info->notifies, nn);

    // tell them that the file exists if they wanted to know
    if (exists && nr->nr_Flags & NRF_NOTIFY_INITIAL)
        SendNotify(nr);

    D(bug("[NTFS] %s: notifying for '%s'\n", __PRETTY_FUNCTION__, nr->nr_FullName));

    return 0;
}

LONG OpRemoveNotify(struct NotifyRequest *nr)
{
    struct FSData *fs_data;
    struct NotifyNode *nn, *nn2;

    D(bug("[NTFS]: %s('%s')\n", __PRETTY_FUNCTION__, nr->nr_FullName));

    /* search inserted volume for the request */
    if (glob->data != NULL) {
        ForeachNodeSafe(&glob->data->info->notifies, nn, nn2) {
            if (nn->nr == nr) {
                D(bug("[NTFS] %s: found notify request in list, removing it\n", __PRETTY_FUNCTION__));
                REMOVE(nn);
                _FreeVecPooled(glob->data->info->mem_pool, nn);
                return 0;
            }
        }
    }

    /* search offline volumes for the request */
    ForeachNode(&glob->sblist, fs_data) {
        ForeachNodeSafe(&fs_data->info->notifies, nn, nn2) {
            if (nn->nr == nr) {
                D(bug("[NTFS] %s: found notify request in list, removing it\n", __PRETTY_FUNCTION__));
                REMOVE(nn);
                _FreeVecPooled(fs_data->info->mem_pool, nn);
                AttemptDestroyVolume(fs_data);
                return 0;
            }
        }
    }

    D(bug("[NTFS] %s: not found, doing nothing\n", __PRETTY_FUNCTION__));

    return 0;
}
