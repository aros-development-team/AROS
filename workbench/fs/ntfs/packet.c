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

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>  

#include "ntfs_fs.h"
#include "ntfs_protos.h"
#include "support.h"

#include "debug.h"

void ProcessPackets(void) {
    struct Message *msg;
    struct DosPacket *pkt;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    while ((msg = GetMsg(glob->ourport)) != NULL) {
        IPTR res = DOSFALSE;
        LONG err = 0;

        pkt = (struct DosPacket *) msg->mn_Node.ln_Name;

        switch(pkt->dp_Type) {
            case ACTION_LOCATE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock = NULL;
                UBYTE *path = BADDR(pkt->dp_Arg2);
                LONG access = pkt->dp_Arg3;

		D(bug("[NTFS] %s: ** LOCATE_OBJECT: lock 0x%08x (dir %ld/%d) name '", __PRETTY_FUNCTION__, pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		RawPutChars(AROS_BSTR_ADDR(path), AROS_BSTR_strlen(path)); bug("' type %s\n",
                      pkt->dp_Arg3 == EXCLUSIVE_LOCK ? "EXCLUSIVE" : "SHARED"));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpLockFile(fl, AROS_BSTR_ADDR(path), AROS_BSTR_strlen(path), access, &lock)) == 0)
                    res = (IPTR)MKBADDR(lock);

                break;
            }

            case ACTION_FREE_LOCK: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[NTFS] %s: ** FREE_LOCK: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

                OpUnlockFile(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_COPY_DIR:
            case ACTION_COPY_DIR_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock = NULL;

                D(bug("[NTFS] %s: ** COPY_DIR: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCopyLock(fl, &lock)) == 0)
                    res = (IPTR)MKBADDR(lock);

                break;
            }

            case ACTION_PARENT:
            case ACTION_PARENT_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock = NULL;

                D(bug("[NTFS] %s: ** PARENT: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));
                
                if ((err = TestLock(fl)))
                    break;
 
                if ((err = OpLockParent(fl, &lock)) == 0)
                    res = (IPTR)MKBADDR(lock);

                break;
            }

            case ACTION_SAME_LOCK: {
                struct ExtFileLock *fl1 = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl2 = BADDR(pkt->dp_Arg2);

                D(bug("[NTFS] %s: ** SAME_LOCK: lock #1 0x%08x (dir %ld/%d) lock #2 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      fl1 != NULL ? fl1->dir->ioh.mft.mftrec_no : 0, fl1 != NULL ? fl1->dir->cur_no : -1,
                      pkt->dp_Arg2,
                      fl2 != NULL ? fl2->dir->ioh.mft.mftrec_no : 0, fl2 != NULL ? fl2->dir->cur_no : -1));

                err = 0;

                if (fl1 == fl2 || fl1->gl == fl2->gl)
                    res = DOSTRUE;

                break;
            }

            case ACTION_EXAMINE_OBJECT:
            case ACTION_EXAMINE_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);

                D(bug("[NTFS] %s: ** EXAMINE_OBJECT: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

                if ((err = TestLock(fl)))
                    break;

                if ((err = FillFIB(fl, fib)) == 0)
		{
		    if (fl->gl->attr & ATTR_DIRECTORY)
		    {
			if (fl->entry)
			{
			    if (fl->entry->key)
			    {
				if ((fl->entry->key->indx) && (fl->entry->key->indx !=fl->dir->ioh.mft.buf))
				{
				    D(bug("[NTFS] %s: ** EXAMINE_OBJECT: freeing old key indx buffer @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key->indx));
				    FreeMem(fl->entry->key->indx, glob->data->idx_size << SECTORSIZE_SHIFT);
				    fl->entry->key->indx = NULL;
				}
				D(bug("[NTFS] %s: ** EXAMINE_OBJECT: freeing old key @ 0x%p\n", __PRETTY_FUNCTION__, fl->entry->key));
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
                    res = DOSTRUE;
		}

                break;
            }

            case ACTION_EXAMINE_NEXT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock = NULL;
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);
                struct DirEntry de;

                D(bug("[NTFS] %s: ** EXAMINE_NEXT: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

                if ((err = TestLock(fl)))
                    break;

		memset(&de, 0, sizeof(struct DirEntry));
		if ((fl->entry != NULL) && (fl->entry->key))
		{
		    struct Index_Key searchkey;
		    
		    CopyMem(fl->entry->key, &searchkey, sizeof(struct Index_Key));
		    de.key = &searchkey;
		}

                if ((err = GetNextDirEntry(fl->dir, &de, TRUE)) != 0) {
                    if (err == ERROR_OBJECT_NOT_FOUND)
                        err = ERROR_NO_MORE_ENTRIES;
		    D(bug("[NTFS] %s: ** EXAMINE_NEXT: no more entries..\n", __PRETTY_FUNCTION__));
                    break;
                }

                if ((err = LockFile(&de, SHARED_LOCK, &lock)) != 0) {
                    break;
                }

                if (!(err = FillFIB(lock, fib))) {
                    res = DOSTRUE;
                }

                FreeLock(lock);

                break;
            }

            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE: {
                struct FileHandle *fh = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *path = BADDR(pkt->dp_Arg3);
                struct ExtFileLock *lock;

	        D(bug("[NTFS] %s: ** %s: lock 0x%08x (dir %ld/%d) path '", __PRETTY_FUNCTION__,
		      pkt->dp_Type == ACTION_FINDINPUT  ? "FINDINPUT"  :
		      pkt->dp_Type == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
							  "FINDUPDATE",
		      pkt->dp_Arg2,
		      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		  RawPutChars(AROS_BSTR_ADDR(path), AROS_BSTR_strlen(path)); bug("'\n"));

                if ((err = TestLock(fl)))
                    break;

		if ((err = OpOpenFile(fl, AROS_BSTR_ADDR(path), AROS_BSTR_strlen(path), pkt->dp_Type, &lock)) != 0)
                    break;

                fh->fh_Arg1 = (IPTR)MKBADDR(lock);
                fh->fh_Port = DOSFALSE;

                res = DOSTRUE;

                break;
            }

            case ACTION_READ: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR)pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3;
		UQUAD read;

                D(
		    bug("[NTFS] %s: ** READ: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		    bug("[NTFS] %s: ** READ: want %u @ pos %u\n", __PRETTY_FUNCTION__, want, fl->pos);
		);

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpRead(fl, buffer, (UQUAD)want, &read)) != 0)
                    res = -1;
                else
		{
		    if (read > 0x7FFFFFFF)
			res = 0x7FFFFFFF;
		    else
			res = (ULONG)read;
		}

                break;
            }

            case ACTION_WRITE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
#if !defined(NTFS_READONLY)		
                APTR buffer = (APTR)pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3;
		UQUAD written;
#endif
                D(bug("[NTFS] %s: ** WRITE: lock 0x%08x (dir %ld/%ld pos %ld) want %ld\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1,
                      fl->pos,
                      want));
#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
#else
                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpWrite(fl, buffer, want, &written)) != 0)
                    res = -1;
                else
		{
		    if (written > 0x7FFFFFFF)
			res = 0x7FFFFFFF;
		    else
			res = written;
		}
#endif
                break;
            }

            case ACTION_SEEK: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                ULONG offsetfrom = pkt->dp_Arg3;

                D(
		    bug("[NTFS] %s: ** SEEK: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__, pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		    bug("[NTFS] %s: ** SEEK: offset %d, current pos %u \n", __PRETTY_FUNCTION__, offset, fl->pos);
		)

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                res = fl->pos;
                err = 0;

                if (offsetfrom == OFFSET_BEGINNING &&
                    offset >= 0 &&
                    offset <= fl->gl->size)
		{
		    D(bug("[NTFS] %s: ** SEEK: from BEGINNING\n", __PRETTY_FUNCTION__));
                    fl->pos = offset;
		}
                else if (offsetfrom == OFFSET_CURRENT &&
                         offset + fl->pos >= 0 &&
                         offset + fl->pos <= fl->gl->size)
		{
		    D(bug("[NTFS] %s: ** SEEK: from CURRENT\n", __PRETTY_FUNCTION__));
                    fl->pos += offset;
		}
                else if (offsetfrom == OFFSET_END
                         && offset <= 0
                         && fl->gl->size + offset >= 0)
		{
		    D(bug("[NTFS] %s: ** SEEK: from END\n", __PRETTY_FUNCTION__));
                    fl->pos = fl->gl->size + offset;
		}
                else {
                    res = -1;
                    err = ERROR_SEEK_ERROR;
                }

                break;
            }

#if defined(ACTION_CHANGE_FILE_POSITION64)
            case ACTION_CHANGE_FILE_POSITION64: {
                D(bug("[NTFS] %s: ** CHANGE_FILE_POSITION64\n", __PRETTY_FUNCTION__));
		res = DOSFALSE;
                break;
            }
#endif

#if defined(ACTION_GET_FILE_POSITION64)
            case ACTION_GET_FILE_POSITION64: {
                D(bug("[NTFS] %s: ** GET_FILE_POSITION64\n", __PRETTY_FUNCTION__));
		res = DOSFALSE;
                break;
            }
#endif
	    
            case ACTION_SET_FILE_SIZE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
#if !defined(NTFS_READONLY)
                LONG offset = pkt->dp_Arg2;
                LONG offsetfrom = pkt->dp_Arg3;
                LONG newsize;
#endif
                D(bug("[NTFS] %s: ** SET_FILE_SIZE: lock 0x%08x (dir %ld/%ld pos %ld) offset %ld\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1,
                      fl->pos,
                      offset));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
#else
                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpSetFileSize(fl, offset, offsetfrom, &newsize)) != 0)
                    res = -1;
                else
                    res = newsize;
#endif

                break;
            }

#if defined(ACTION_CHANGE_FILE_SIZE64)
	    case ACTION_CHANGE_FILE_SIZE64: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
#if !defined(NTFS_READONLY)
                UQUAD offset = pkt->dp_Arg2;
                LONG offsetfrom = pkt->dp_Arg3;
                UQUAD newsize;
#endif
                D(bug("[NTFS] %s: ** CHANGE_FILE_SIZE64: lock 0x%08x (dir %ld/%ld pos %ld) offset %lld offsetfrom %s\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1,
                      fl->pos,
                      offset,
                      offsetfrom == OFFSET_BEGINNING ? "BEGINNING" :
                      offsetfrom == OFFSET_END       ? "END"       :
                      offsetfrom == OFFSET_CURRENT   ? "CURRENT"   :
                                                   "(unknown)"));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
#else
                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpSetFileSize(fl, offset, offsetfrom, &newsize)) != 0)
                    res = -1;
                else
                    res = newsize;
#endif
		break;
	    }
#endif

#if defined(ACTION_GET_FILE_SIZE64)
	    case ACTION_GET_FILE_SIZE64: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[NTFS] %s: ** GET_FILE_SIZE64: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

		if ((fl->entry) && (fl->gl))
		    res = (IPTR)&fl->gl->size;

		break;
	    }
#endif

            case ACTION_END: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[NTFS] %s: ** END: lock 0x%08x (dir %ld/%ld)\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1,
                      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1));

                if ((err = TestLock(fl)))
                    break;

                FreeLock(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_IS_FILESYSTEM:
                D(bug("[NTFS] %s: ** IS_FILESYSTEM\n", __PRETTY_FUNCTION__));

                res = DOSTRUE;
                break;

            case ACTION_CURRENT_VOLUME: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[NTFS] %s: ** CURRENT_VOLUME: lock 0x%08x\n", __PRETTY_FUNCTION__,
                      pkt->dp_Arg1));

                res = (IPTR)((fl) ? fl->fl_Volume : ((glob->data != NULL) ? MKBADDR(glob->data->doslist) : BNULL));
                break;
            }

            case ACTION_INFO:
            case ACTION_DISK_INFO: {
                struct InfoData *id;

                if (pkt->dp_Type == ACTION_INFO) {
                    struct FileLock *fl = BADDR(pkt->dp_Arg1);

                    D(bug("[NTFS] %s: ** INFO: lock 0x%08x\n", __PRETTY_FUNCTION__,
                          pkt->dp_Arg1));

                    if (fl && (glob->data == NULL || fl->fl_Volume != MKBADDR(glob->data->doslist))) {
                        err = ERROR_DEVICE_NOT_MOUNTED;
                        break;
                    }

                    id = BADDR(pkt->dp_Arg2);
                }
                else {
                    D(bug("[NTFS] %s: ** DISK_INFO\n", __PRETTY_FUNCTION__));

                    id = BADDR(pkt->dp_Arg1);
                }

                FillDiskInfo(id);

                res = DOSTRUE;
                break;
            }

            case ACTION_INHIBIT: {
                LONG inhibit = pkt->dp_Arg1;

                D(bug("[NTFS] %s: ** INHIBIT: %sinhibit\n", __PRETTY_FUNCTION__,
                    inhibit == DOSTRUE ? "" : "un"));

                if (inhibit == DOSTRUE) {
                    glob->disk_inhibited++;
                    if (glob->disk_inhibited == 1)
                        DoDiskRemove();
                }
                else if (glob->disk_inhibited) {
                    glob->disk_inhibited--;
                    if (glob->disk_inhibited == 0)
                       ProcessDiskChange();
                }

                res = DOSTRUE;
                break;
            }

            case ACTION_DIE: {
                struct FSData *fs_data;
                struct NotifyNode *nn;

                D(bug("[NTFS] %s: ** DIE\n", __PRETTY_FUNCTION__));

                /* clear our message port from notification requests so DOS won't send
                 * notification-end packets to us after we're gone */
                ForeachNode(&glob->sblist, fs_data) {
                    ForeachNode(&fs_data->info->notifies, nn) {
                        nn->nr->nr_Handler = NULL;
                    }
                }

                if ((glob->data != NULL
                    && !(IsListEmpty(&glob->data->info->locks)
                    && IsListEmpty(&glob->data->info->notifies)))) {

                    D(bug("[NTFS] %s:\tThere are remaining locks or notification "
                        "requests. Shutting down is not possible\n", __PRETTY_FUNCTION__));

                    err = ERROR_OBJECT_IN_USE;
                    break;
                }

                D(bug("[NTFS] %s:\tNothing pending. Shutting down the handler\n", __PRETTY_FUNCTION__));

                DoDiskRemove(); /* risky, because of async. volume remove, but works */

                glob->quit = TRUE;
                glob->death_packet = pkt;
                glob->devnode->dol_Task = NULL;

                res = DOSTRUE;
                break;
            }

#if 0
            /* XXX AROS needs these ACTION_ headers defined in dos/dosextens.h */

            case ACTION_GET_DISK_FSSM: {
                D(bug("[NTFS] %s: ** ACTION_GET_DISK_FSSM\n", __PRETTY_FUNCTION__));

                res = (ULONG) glob->fssm;
                break;
            }

            case ACTION_FREE_DISK_FSSM: {
                D(bug("[NTFS] %s: ** ACTION_FREE_DISK_FSSM\n", __PRETTY_FUNCTION__));

                res = DOSTRUE;
                break;

            }
#endif

            case ACTION_DISK_CHANGE: { /* internal */
                struct DosList *vol = (struct DosList *)pkt->dp_Arg2;
                struct VolumeInfo *vol_info = BADDR(vol->dol_misc.dol_volume.dol_LockList);
                ULONG type = pkt->dp_Arg3;

                D(bug("[NTFS] %s: ** DISK_CHANGE [INTERNAL]\n", __PRETTY_FUNCTION__));

                if (pkt->dp_Arg1 == ID_NTFS_DISK) { /* security check */

                    if (AttemptLockDosList(LDF_VOLUMES|LDF_WRITE)) {

                        if (type == ACTION_VOLUME_ADD) {
                            AddDosEntry(vol);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKINSERTED);

                            D(bug("[NTFS] %s: \tVolume added successfuly\n", __PRETTY_FUNCTION__));
                        }
                        else if (type == ACTION_VOLUME_REMOVE) {
                            RemDosEntry(vol);
                            DeletePool(vol_info->mem_pool);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKREMOVED);

                            D(bug("[NTFS] %s: \tVolume removed successfuly.\n", __PRETTY_FUNCTION__));
                        }

                        FreeDosObject(DOS_STDPKT, pkt); /* cleanup */

                        pkt = NULL;
                        D(bug("[NTFS] %s: Packet destroyed\n", __PRETTY_FUNCTION__));
                    }

                    else {
                        D(bug("[NTFS] %s:\tDosList is locked\n", __PRETTY_FUNCTION__));
                        Delay(5);
                        PutMsg(glob->ourport, pkt->dp_Link);
                        pkt = NULL;
                        D(bug("[NTFS] %s: Message moved to the end of the queue\n", __PRETTY_FUNCTION__));
                    }
                }
                else
                    err = ERROR_OBJECT_WRONG_TYPE;

                break;
            }

            case ACTION_RENAME_DISK: {
                UBYTE *name = BADDR(pkt->dp_Arg1);
                
		D(bug("[NTFS] %s: ** RENAME_DISK: name '", __PRETTY_FUNCTION__); RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)name; // unused
#else
                if (glob->data->doslist == NULL) {
                    err = glob->disk_inserted ? ERROR_NOT_A_DOS_DISK : ERROR_NO_DISK;
                    break;
                }

                while (! AttemptLockDosList(LDF_VOLUMES | LDF_WRITE))
                    ProcessPackets();

//                err = SetVolumeName(glob->data, name);
                UnLockDosList(LDF_VOLUMES | LDF_WRITE);
                if (err != 0)
                    break;

#ifdef AROS_FAST_BPTR
                /* ReadFATSuper() sets a null byte after the
                 * string, so this should be fine */
                CopyMem(glob->data->volume.name + 1, glob->data->doslist->dol_Name,
                    glob->data->volume.name[0] + 1);
#else
                CopyMem(glob->data->volume.name, BADDR(glob->data->doslist->dol_Name),
                    glob->data->volume.name[0] + 2);
#endif

                SendEvent(IECLASS_DISKINSERTED);

                res = DOSTRUE;
#endif
                break;
            }

            case ACTION_DELETE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                UBYTE *name = BADDR(pkt->dp_Arg2);

	        D(bug("[NTFS] %s: ** DELETE_OBJECT: lock 0x%08x (dir %ld/%d) path '", __PRETTY_FUNCTION__,
		      pkt->dp_Arg1,
		      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
		(void)name; // unused
#else
                if ((err = TestLock(fl)))
                    break;

                err = OpDeleteFile(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name));
#endif
                break;
            }

            case ACTION_RENAME_OBJECT: {
                struct ExtFileLock *sfl = BADDR(pkt->dp_Arg1), *dfl = BADDR(pkt->dp_Arg3);
                UBYTE *sname = BADDR(pkt->dp_Arg2), *dname = BADDR(pkt->dp_Arg4);

	        D(bug("[NTFS] %s: ** RENAME_OBJECT: srclock 0x%08x (dir %ld/%d) name '", __PRETTY_FUNCTION__,
		      pkt->dp_Arg1,
		      sfl != NULL ? sfl->dir->ioh.mft.mftrec_no : 0, sfl != NULL ? sfl->dir->cur_no : -1);
		  RawPutChars(AROS_BSTR_ADDR(sname), AROS_BSTR_strlen(sname)); bug("' destlock 0x%08x (dir %ld/%d) name '",
		      pkt->dp_Arg3,
		      dfl != NULL ? dfl->dir->ioh.mft.mftrec_no : 0, dfl != NULL ? dfl->dir->cur_no : -1);
		  RawPutChars(AROS_BSTR_ADDR(dname), AROS_BSTR_strlen(dname)); bug("'\n"));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)sfl; // unused
		(void)dfl; // unused
		(void)sname; // unused
		(void)dname; // unused
#else
                if ((err = TestLock(sfl)) != 0 || (err = TestLock(dfl)) != 0)
                    break;

                err = OpRenameFile(sfl, AROS_BSTR_ADDR(sname), AROS_BSTR_strlen(sname), dfl, AROS_BSTR_ADDR(dname), AROS_BSTR_strlen(dname));
#endif
                break;
            }

            case ACTION_CREATE_DIR: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *new;
                UBYTE *name = BADDR(pkt->dp_Arg2);

	        D(bug("[NTFS] %s: ** CREATE_DIR: lock 0x%08x (dir %ld/%d) name '", __PRETTY_FUNCTION__,
		      pkt->dp_Arg1,
		      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
		new=NULL;(void)new; // unused
		(void)name; // unused
#else
                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCreateDir(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), &new)) == 0)
                    res = (IPTR)MKBADDR(new);
#endif
                break;
            }

            case ACTION_SET_PROTECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *name = BADDR(pkt->dp_Arg3);
                ULONG prot = pkt->dp_Arg4;

	        D(bug("[NTFS] %s: ** SET_PROTECT: lock 0x%08x (dir %ld/%d) name '", __PRETTY_FUNCTION__, pkt->dp_Arg2,
		      (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("' prot 0x%08x\n", prot));

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
		(void)name; // unused
		(void)prot; // unused
#else
                if ((err = TestLock(fl)))
                    break;

                err = OpSetProtect(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), prot);
#endif
                break;
            }

            case ACTION_SET_DATE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *name = BADDR(pkt->dp_Arg3);
		struct DateStamp *ds = (struct DateStamp *)pkt->dp_Arg4;

#if defined(DEBUG) && DEBUG != 0
                {
                    struct DateTime dt;
                    char datestr[LEN_DATSTRING];

                    dt.dat_Stamp = *ds;
                    dt.dat_Format = FORMAT_DOS;
                    dt.dat_Flags = 0;
                    dt.dat_StrDay = NULL;
                    dt.dat_StrDate = datestr;
                    dt.dat_StrTime = NULL;
                    DateToStr(&dt);

		    D(bug("[NTFS] %s: ** SET_DATE: lock 0x%08x (dir %ld/%d) name '", __PRETTY_FUNCTION__,
			  pkt->dp_Arg2,
			  (fl != NULL && fl->dir != NULL) ? fl->dir->ioh.mft.mftrec_no : FILE_ROOT, (fl != NULL && fl->entry != NULL) ? fl->entry->no : -1);
		      RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("' ds '%s'\n", datestr));
                }
#endif

#if defined(NTFS_READONLY)
		res = ERROR_DISK_WRITE_PROTECTED;
		(void)fl; // unused
		(void)name; // unused
		(void)ds; // unused
#else
                if ((err = TestLock(fl)))
                    break;

                err = OpSetDate(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), ds);
#endif
                break;
            }

            case ACTION_ADD_NOTIFY: {
		struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[NTFS] %s: ** ADD_NOTIFY: nr 0x%08x name '%s'\n", __PRETTY_FUNCTION__, nr, nr->nr_FullName));

                err = OpAddNotify(nr);

                break;
            }

            case ACTION_REMOVE_NOTIFY: {
		struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[NTFS] %s: ** REMOVE_NOTIFY: nr 0x%08x name '%s'\n", __PRETTY_FUNCTION__, nr, nr->nr_FullName));

                err = OpRemoveNotify(nr);
                
                break;
            }

            default:
                D(bug("[NTFS] %s: got unknown packet type %ld\n", __PRETTY_FUNCTION__, pkt->dp_Type));

                err = ERROR_ACTION_NOT_KNOWN;
        }

        if (pkt != NULL) {
            pkt->dp_Res1 = res;
            pkt->dp_Res2 = err;
            if (!glob->quit) {
                D(bug("[NTFS] %s: replying to packet [result 0x%x, error 0x%x]\n", __PRETTY_FUNCTION__,
                    res, err));
                ReplyPacket(pkt);
            }
        }

        RestartTimer();
    }
}

void ReplyPacket(struct DosPacket *pkt)
{
    struct MsgPort *rp;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    rp = pkt->dp_Port;

    pkt->dp_Port = glob->ourport;

    PutMsg(rp, pkt->dp_Link);
}

