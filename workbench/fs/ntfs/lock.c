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
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "ntfs_fs.h"
#include "ntfs_protos.h"
#include "support.h"

#include "debug.h"

#if DEBUG == 0
#define DumpLocks(fs_data)
#else
void DumpLocks(struct FSData *fs_data)
{
    struct GlobalLock *gl;
    ULONG count;

    bug("[NTFS] %s: global locks-:\n", __PRETTY_FUNCTION__);

    ListLength(&fs_data->info->root_lock.locks, count);
    bug("[NTFS] %s:\troot: %ld references\n", __PRETTY_FUNCTION__, count);
    
    ForeachNode(&fs_data->info->locks, gl) {
        ListLength(&gl->locks, count);
	bug("[NTFS] %s:\t    (%ld/%ld) ", __PRETTY_FUNCTION__, gl->dir_cluster / glob->data->mft_size, gl->dir_entry); RawPutChars(&(gl->name[1]), gl->name[0]);
	bug(": %ld references\n",  count);
    }
}
#endif

LONG TestLock(struct ExtFileLock *fl)
{
    if (fl == 0 && glob->data == NULL) {
        if (glob->disk_inserted == FALSE)
            return ERROR_NO_DISK;
        else
            return ERROR_NOT_A_DOS_DISK;
    }
 
    if (glob->data == NULL || glob->disk_inhibited || (fl && fl->fl_Volume != MKBADDR(glob->data->doslist)))
        return ERROR_DEVICE_NOT_MOUNTED;

    if (fl && fl->magic != ID_NTFS_DISK)
        return ERROR_OBJECT_WRONG_TYPE;

    return 0;
}

LONG LockFileByName(struct ExtFileLock *fl, UBYTE *name, LONG namelen, LONG access, struct ExtFileLock **lock)
{
    LONG err = ERROR_OBJECT_NOT_FOUND;
    struct DirHandle dirh;
    struct DirHandle *dh = &dirh;
    struct DirEntry de;

    D(bug("[NTFS] %s('", __PRETTY_FUNCTION__); RawPutChars(name, namelen); bug("')\n"));

    D(bug("[NTFS] %s: fs_data @ 0x%p\n", __PRETTY_FUNCTION__, glob->data));

    /* if the name is empty, just duplicate the base lock */
    if (namelen == 0)
        return CopyLock(fl, lock);

    /* if the base lock is a file, the name must either be empty (handled
     * above) or start with '/' (handled here) */
    if (fl != NULL && !(fl->gl->attr & ATTR_DIRECTORY)) {
        if (name[0] == '/') {
            if (namelen == 1)
                return OpLockParent(fl, lock);
            else {
                name++;
                namelen--;
            }
        }
        else
            return ERROR_OBJECT_WRONG_TYPE;
    }

    dh->ioh.mft.buf = NULL;

    /* open the dir */
    if (fl == NULL)
    {
        dh->ioh.mft.mftrec_no = FILE_ROOT;
    }
    else
    {
	dh->ioh.mft.mftrec_no = fl->dir->ioh.mft.mftrec_no;
    }
    InitDirHandle(glob->data, dh, FALSE);

    D(bug("[NTFS] %s: looking in directory MFT #%u\n", __PRETTY_FUNCTION__, (IPTR)dh->ioh.mft.mftrec_no));

    memset(&de, 0, sizeof(struct DirEntry));

    /* look for the entry */
    if ((err = GetDirEntryByPath(dh, name, namelen, &de)) != 0) {
        D(bug("[NTFS] %s: couldn't get lock\n", __PRETTY_FUNCTION__));
        return err;
    }

    ReleaseDirHandle(dh);

    /* found it, do the locking proper */
    if (de.entrytype & ATTR_DIRECTORY && !de.entry)
        err = LockRoot(access, lock);
    else
        err = LockFile(&de, access, lock);

    return err;
}

LONG LockFile(struct DirEntry *de, LONG access, struct ExtFileLock **lock)
{
    struct GlobalLock *node, *gl;
    struct ExtFileLock *fl;

    D(bug("[NTFS]: %s(entry @ 0x%p) (%s)\n", __PRETTY_FUNCTION__, de, access == SHARED_LOCK ? "shared" : "exclusive"));

    /* first see if we already have a global lock for this file */
    gl = NULL;
    ForeachNode(&glob->data->info->locks, node)
        if (node->dir_cluster == de->cluster && node->dir_entry == de->no) {
	    D(bug("[NTFS] %s: using GlobalLock @ 0x%p\n", __PRETTY_FUNCTION__, node));
            gl = node;
            break;
        }

    /* if we do and we're trying for an exclusive lock, then bail out */
    if (gl != NULL && access == EXCLUSIVE_LOCK) {
        D(bug("[NTFS] %s: can't obtain exclusive lock on already-locked file\n", __PRETTY_FUNCTION__));
        return ERROR_OBJECT_IN_USE;
    }

    /* allocate space for the lock. we do this first so that we don't go to
     * all the effort of setting up the global lock only to have to discard it
     * if the filelock allocation fails */
    if ((fl = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct ExtFileLock))) == NULL)
        return ERROR_NO_FREE_STORE;

    if ((fl->dir = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct DirHandle))) == NULL)
        return ERROR_NO_FREE_STORE;

    memset(fl->dir, 0, sizeof(struct DirHandle));

    if ((fl->entry = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct DirEntry))) == NULL)
        return ERROR_NO_FREE_STORE;

    memset(fl->entry, 0, sizeof(struct DirEntry));

    fl->data = glob->data;

    fl->dir->ioh.mft.mftrec_no = de->cluster / glob->data->mft_size;
    InitDirHandle(glob->data, fl->dir, FALSE);

    fl->entry->data = de->data;		/* filesystem data */
    fl->entry->entryname = AllocVec(strlen(de->entryname) + 1, MEMF_ANY|MEMF_CLEAR);
    CopyMem(de->entryname, fl->entry->entryname, strlen(de->entryname));
    fl->entry->entrytype = de->entrytype;

    fl->entry->no = de->no;

    if ((fl->entry->entry = de->entry) == NULL)
    {
	GetDirEntry(fl->dir, de->no, fl->entry);
    }
    de->entry = NULL;

    fl->entry->cluster = de->cluster;

    /* if we don't have a global lock we need to build one */
    if (gl == NULL) {
        if ((gl = _AllocVecPooled(glob->data->info->mem_pool,
            sizeof(struct GlobalLock))) == NULL) {
            _FreeVecPooled(glob->data->info->mem_pool, fl);
            return ERROR_NO_FREE_STORE;
        }

        gl->dir_cluster = fl->entry->cluster;
        gl->dir_entry = fl->entry->no;
        gl->access = access;

        gl->first_cluster = fl->entry->entry->mftrec_no * glob->data->mft_size;
        
        gl->attr = fl->entry->entrytype;
	if (fl->entry->entry)
	    gl->size = fl->entry->entry->size;

	gl->name[0] = strlen(fl->entry->entryname) + 1;
	CopyMem(fl->entry->entryname, &gl->name[1], gl->name[0]);
	
        NEWLIST(&gl->locks);

        ADDTAIL(&glob->data->info->locks, gl);

        D(bug("[NTFS] %s: created new global lock for '%s'\n", __PRETTY_FUNCTION__, &gl->name[1]));

        /* TODO : look through the notify list. if there's any in there that aren't
         * currently attached to a global lock, expand them and if they are
         * for this file, fill them in */
    }

    /* now setup the file lock */
    fl->fl_Link = BNULL;
    fl->fl_Access = access;
    fl->fl_Task = glob->ourport;
    fl->fl_Volume = MKBADDR(glob->data->doslist);

    fl->magic = ID_NTFS_DISK;

    fl->pos = 0;

    fl->do_notify = FALSE;

    fl->gl = gl;
    ADDTAIL(&gl->locks, &fl->node);

    D(bug("[NTFS] %s: created file lock @ 0x%08x\n", __PRETTY_FUNCTION__, fl));

    DumpLocks(glob->data);

    *lock = fl;
    return 0;
}

LONG LockRoot(LONG access, struct ExtFileLock **lock)
{
    struct ExtFileLock *fl;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (access == EXCLUSIVE_LOCK) {
        D(bug("[NTFS] %s: EXCLUSIVE_LOCK\n", __PRETTY_FUNCTION__));
        return ERROR_OBJECT_IN_USE;
    }

    if ((fl = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct ExtFileLock))) == NULL)
        return ERROR_NO_FREE_STORE;

    if ((fl->dir = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct DirHandle))) == NULL)
        return ERROR_NO_FREE_STORE;

    memset(fl->dir, 0, sizeof(struct DirHandle));

    if ((fl->entry = _AllocVecPooled(glob->data->info->mem_pool,
        sizeof(struct DirEntry))) == NULL)
        return ERROR_NO_FREE_STORE;

    memset(fl->entry, 0, sizeof(struct DirEntry));

    fl->fl_Link = BNULL;
    fl->fl_Key = 0;
    fl->fl_Access = SHARED_LOCK;
    fl->fl_Task = glob->ourport;
    fl->fl_Volume = MKBADDR(glob->data->doslist);

    fl->magic = ID_NTFS_DISK;

    fl->dir->ioh.mft.data = glob->data; 

    fl->dir->ioh.mft.mftrec_no = FILE_ROOT;
    InitDirHandle(glob->data, fl->dir, FALSE);

    fl->pos = 0;

    fl->do_notify = FALSE;

    if (IsListEmpty(&glob->data->info->root_lock.locks))
        ADDTAIL(&glob->data->info->locks, &glob->data->info->root_lock);
    fl->gl = &glob->data->info->root_lock;
    fl->data = glob->data;
    ADDTAIL(&glob->data->info->root_lock.locks, &fl->node);

    D(bug("[NTFS] %s: created root lock 0x%08x\n", __PRETTY_FUNCTION__, fl));

    DumpLocks(glob->data);

    *lock = fl;
    return 0;
}

LONG CopyLock(struct ExtFileLock *fl, struct ExtFileLock **lock)
{
    struct DirEntry de;
    LONG ret;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (fl == NULL ||  (fl->gl == &glob->data->info->root_lock))
        return LockRoot(SHARED_LOCK, lock);

    if (fl->fl_Access == EXCLUSIVE_LOCK) {
        D(bug("[NTFS] %s: EXCLUSIVE_LOCK\n", __PRETTY_FUNCTION__));
        return ERROR_OBJECT_IN_USE;
    }

    D(bug("[NTFS] %s: copying lock (dir %ld; entry %ld)\n", __PRETTY_FUNCTION__, fl->gl->dir_cluster / glob->data->mft_size, fl->gl->dir_entry));

    memset(&de, 0, sizeof(struct DirEntry));

    if (fl->entry)
    {
	de.cluster = fl->dir->ioh.first_cluster;
	de.no = fl->entry->no;
    }
    else
    {
	struct DirHandle dh;
	dh.ioh.mft.mftrec_no = fl->gl->dir_cluster / glob->data->mft_size;
    	dh.ioh.mft.buf = NULL;
	InitDirHandle(glob->data, &dh, FALSE);
	GetDirEntry(&dh, fl->gl->dir_entry, &de);
    }
    if ((ret = LockFile(&de, SHARED_LOCK, lock)) == 0)
    {
	fl = *lock;
	if (fl->gl->attr & ATTR_DIRECTORY)
	{
	    if (fl->entry)
	    {
		if (fl->entry->key)
		{
		    if ((fl->entry->key->indx) && (fl->entry->key->indx !=fl->dir->ioh.mft.buf))
		    {
			D(bug("[NTFS] %s: freeing old key indx buffer @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key->indx));
			FreeMem(fl->entry->key->indx, glob->data->idx_size << SECTORSIZE_SHIFT);
			fl->entry->key->indx = NULL;
		    }
		    D(bug("[NTFS] %s: freeing old key @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key));
		    FreeMem(fl->entry->key, sizeof(struct Index_Key));
		    fl->entry->key = NULL;
		}
		fl->entry = NULL;
	    }
	    fl->dir->parent_mft = fl->dir->ioh.mft.mftrec_no;
	    fl->dir->ioh.mft.mftrec_no = fl->gl->first_cluster / glob->data->mft_size;
	    ReleaseDirHandle(fl->dir);
	    InitDirHandle(glob->data, fl->dir, FALSE);			
	}
    }
    return ret;
}

void FreeLock(struct ExtFileLock *fl)
{
    struct NotifyNode *nn;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (fl == NULL)
        return;

    D(bug("[NTFS] %s: lock @ 0x%08x\n", __PRETTY_FUNCTION__, fl));

    if (fl->do_notify)
        SendNotifyByLock(fl->dir->ioh.data, fl->gl);

    REMOVE(&fl->node);

    if (IsListEmpty((struct List *)&fl->gl->locks))
    {
        REMOVE(fl->gl);

	ForeachNode(&fl->data->info->notifies, nn)
            if(nn->gl == fl->gl)
                nn->gl = NULL;

        if (fl->gl != &fl->data->info->root_lock)
            _FreeVecPooled(glob->data->info->mem_pool, fl->gl);

        D(bug("[NTFS] %s: freed associated global lock\n", __PRETTY_FUNCTION__));
    }

    DumpLocks(fl->data);

    if (fl->entry != NULL)
    {
	FreeVec(fl->entry->entryname);
	fl->entry->entryname = NULL;

	if (fl->entry->entry != NULL)
	{
	    if (fl->entry->entry->cblock != NULL)
	    {
		Cache_FreeBlock(fl->data->cache, fl->entry->entry->cblock);
		fl->entry->entry->cblock = NULL;
		
		if (fl->entry->entry->buf != NULL)
		{
		    FreeMem(fl->entry->entry->buf, glob->data->mft_size << SECTORSIZE_SHIFT);
		    fl->entry->entry->buf = NULL;
		}
	    }
	    FreeMem(fl->entry->entry, sizeof (struct NTFSMFTEntry));
	    fl->entry->entry = NULL;
	}
	if (fl->entry->key != NULL)
	{
	    if ((fl->entry->key->indx != NULL) && (fl->entry->key->indx != fl->dir->ioh.mft.buf))
	    {
		D(bug("[NTFS] %s: freeing old key indx buffer @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key->indx));
		FreeMem(fl->entry->key->indx, glob->data->idx_size << SECTORSIZE_SHIFT);
		fl->entry->key->indx = NULL;
	    }
	    D(bug("[NTFS] %s: freeing old key @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key));
	    FreeMem(fl->entry->key, sizeof(struct Index_Key));
	    fl->entry->key = NULL;
	}
	_FreeVecPooled(glob->data->info->mem_pool, fl->entry);
	fl->entry = NULL;
    }

    if (fl->dir)
    {
	if (fl->dir->ioh.mft.cblock != NULL)
	{
	    Cache_FreeBlock(fl->data->cache, fl->dir->ioh.mft.cblock);
	    fl->dir->ioh.mft.cblock = NULL;
	    
	    if (fl->dir->ioh.mft.buf)
	    {
		FreeMem(fl->dir->ioh.mft.buf, glob->data->mft_size << SECTORSIZE_SHIFT);
		fl->dir->ioh.mft.buf = NULL;
	    }
	}
	if (!(fl->dir->ioh.bitmap))
	{
	    FreeVec(fl->dir->ioh.bitmap);
	    fl->dir->ioh.bitmap = NULL;
	}
	_FreeVecPooled(glob->data->info->mem_pool, fl->dir);
	fl->dir = NULL;
    }
    
    if (fl->data != glob->data)
        AttemptDestroyVolume(fl->data);

    _FreeVecPooled(glob->data->info->mem_pool, fl);
}

