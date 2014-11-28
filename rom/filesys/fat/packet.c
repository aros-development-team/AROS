/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2014 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
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

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_PACKETS
#include "debug.h"

void ProcessPackets(void) {
    struct Message *msg;
    struct DosPacket *pkt;

    while ((msg = GetMsg(glob->ourport)) != NULL) {
        IPTR res = DOSFALSE;
        LONG err = 0;

        pkt = (struct DosPacket *) msg->mn_Node.ln_Name;

        switch(pkt->dp_Type) {
            case ACTION_LOCATE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;
                UBYTE *path = BADDR(pkt->dp_Arg2);
                LONG access = pkt->dp_Arg3;

		D(bug("[FAT] LOCATE_OBJECT: lock 0x%08x (dir %ld/%ld) name '", pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
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

                D(bug("[FAT] FREE_LOCK: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                OpUnlockFile(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_COPY_DIR:
            case ACTION_COPY_DIR_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[FAT] COPY_DIR: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCopyLock(fl, &lock)) == 0)
                    res = (IPTR)MKBADDR(lock);

                break;
            }

            case ACTION_PARENT:
            case ACTION_PARENT_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[FAT] ACTION_PARENT: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));
                
                if ((err = TestLock(fl)))
                    break;
 
                if ((err = OpLockParent(fl, &lock)) == 0)
                    res = (IPTR)MKBADDR(lock);

                break;
            }

            case ACTION_SAME_LOCK: {
                struct ExtFileLock *fl1 = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl2 = BADDR(pkt->dp_Arg2);

                D(bug("[FAT] ACTION_SAME_LOCK: lock #1 0x%08x (dir %ld/%ld) lock #2 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl1 != NULL ? fl1->gl->dir_cluster : 0, fl1 != NULL ? fl1->gl->dir_entry : 0,
                      pkt->dp_Arg2,
                      fl2 != NULL ? fl2->gl->dir_cluster : 0, fl2 != NULL ? fl2->gl->dir_entry : 0));

                err = 0;

                if (fl1 == fl2 || fl1->gl == fl2->gl)
                    res = DOSTRUE;

                break;
            }

            case ACTION_EXAMINE_OBJECT:
            case ACTION_EXAMINE_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);

                D(bug("[FAT] EXAMINE_OBJECT: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                if ((err = FillFIB(fl, fib)) == 0)
                    res = DOSTRUE;

                break;
            }

            case ACTION_EXAMINE_NEXT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);
                struct DirHandle dh;
                struct DirEntry de;

                D(bug("[FAT] EXAMINE_NEXT: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

		if ((err = InitDirHandle(glob->sb, fl->ioh.first_cluster, &dh, FALSE)) != 0)
                    break;

                dh.cur_index = fib->fib_DiskKey;

                if ((err = GetNextDirEntry(&dh, &de)) != 0) {
                    if (err == ERROR_OBJECT_NOT_FOUND)
                        err = ERROR_NO_MORE_ENTRIES;
                    ReleaseDirHandle(&dh);
                    break;
                }

                if ((err = LockFile(fl->ioh.first_cluster, dh.cur_index, SHARED_LOCK, &lock)) != 0) {
                    ReleaseDirHandle(&dh);
                    break;
                }

                if (!(err = FillFIB(lock, fib))) {
                    fib->fib_DiskKey = dh.cur_index;
                    res = DOSTRUE;
                }

                FreeLock(lock);
                ReleaseDirHandle(&dh);

                break;
            }

            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE: {
                struct FileHandle *fh = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *path = BADDR(pkt->dp_Arg3);
                struct ExtFileLock *lock;

	        D(bug("[FAT] %s: lock 0x%08x (dir %ld/%ld) path '",
		      pkt->dp_Type == ACTION_FINDINPUT  ? "FINDINPUT"  :
		      pkt->dp_Type == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
							  "FINDUPDATE",
		      pkt->dp_Arg2,
		      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
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
                ULONG want = pkt->dp_Arg3, read;

                D(bug("[FAT] READ: lock 0x%08x (dir %ld/%ld pos %ld) want %ld\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      want));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpRead(fl, buffer, want, &read)) != 0)
                    res = -1;
                else
                    res = read;

                break;
            }

            case ACTION_WRITE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR)pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3, written;

                D(bug("[FAT] WRITE: lock 0x%08x (dir %ld/%ld pos %ld) want %ld\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      want));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpWrite(fl, buffer, want, &written)) != 0)
                    res = -1;
                else
                    res = written;

                break;
            }

            case ACTION_SEEK: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                ULONG whence = pkt->dp_Arg3;

                D(bug("[FAT] SEEK: lock 0x%08x (dir %ld/%ld pos %ld) offset %ld whence %s\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      offset,
                      whence == OFFSET_BEGINNING ? "BEGINNING" :
                      whence == OFFSET_END       ? "END"       :
                      whence == OFFSET_CURRENT   ? "CURRENT"   :
                                                   "(unknown)"));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                res = fl->pos;
                err = 0;

                if (whence == OFFSET_BEGINNING &&
                    offset >= 0 &&
                    offset <= fl->gl->size)
                    fl->pos = offset;
                else if (whence == OFFSET_CURRENT &&
                         offset + fl->pos >= 0 &&
                         offset + fl->pos <= fl->gl->size)
                    fl->pos += offset;
                else if (whence == OFFSET_END
                         && offset <= 0
                         && fl->gl->size + offset >= 0)
                    fl->pos = fl->gl->size + offset;
                else {
                    res = -1;
                    err = ERROR_SEEK_ERROR;
                }

                break;
            }

            case ACTION_SET_FILE_SIZE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                LONG whence = pkt->dp_Arg3;
                LONG newsize;

                D(bug("[FAT] SET_FILE_SIZE: lock 0x%08x (dir %ld/%ld pos %ld) offset %ld whence %s\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      offset,
                      whence == OFFSET_BEGINNING ? "BEGINNING" :
                      whence == OFFSET_END       ? "END"       :
                      whence == OFFSET_CURRENT   ? "CURRENT"   :
                                                   "(unknown)"));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpSetFileSize(fl, offset, whence, &newsize)) != 0)
                    res = -1;
                else
                    res = newsize;

                break;
            }

            case ACTION_END: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[FAT] END: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                FreeLock(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_IS_FILESYSTEM:
                D(bug("[FAT] IS_FILESYSTEM\n"));

                res = DOSTRUE;
                break;

            case ACTION_CURRENT_VOLUME: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[FAT] CURRENT_VOLUME: lock 0x%08x\n",
                      pkt->dp_Arg1));

                res = (IPTR)((fl) ? fl->fl_Volume : ((glob->sb != NULL) ? MKBADDR(glob->sb->doslist) : BNULL));
                break;
            }

            case ACTION_INFO:
            case ACTION_DISK_INFO: {
                struct InfoData *id;

                if (pkt->dp_Type == ACTION_INFO) {
                    struct FileLock *fl = BADDR(pkt->dp_Arg1);

                    D(bug("[FAT] INFO: lock 0x%08x\n",
                          pkt->dp_Arg1));

                    if (fl && (glob->sb == NULL || fl->fl_Volume != MKBADDR(glob->sb->doslist))) {
                        err = ERROR_DEVICE_NOT_MOUNTED;
                        break;
                    }

                    id = BADDR(pkt->dp_Arg2);
                }
                else {
                    D(bug("[FAT] DISK_INFO\n"));

                    id = BADDR(pkt->dp_Arg1);
                }

                FillDiskInfo(id);

                res = DOSTRUE;
                break;
            }

            case ACTION_INHIBIT: {
                LONG inhibit = pkt->dp_Arg1;

                D(bug("[FAT] INHIBIT: %sinhibit\n",
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
                struct FSSuper *sb;
                struct NotifyNode *nn;

                D(bug("[FAT] DIE\n"));

                /* clear our message port from notification requests so DOS won't send
                 * notification-end packets to us after we're gone */
                ForeachNode(&glob->sblist, sb) {
                    ForeachNode(&sb->info->notifies, nn) {
                        nn->nr->nr_Handler = NULL;
                    }
                }

                if ((glob->sb != NULL
                    && !(IsListEmpty(&glob->sb->info->locks)
                    && IsListEmpty(&glob->sb->info->notifies)))) {

                    D(bug("\tThere are remaining locks or notification "
                        "requests. Shutting down is not possible\n"));

                    err = ERROR_OBJECT_IN_USE;
                    break;
                }

                D(bug("\tNothing pending. Shutting down the handler\n"));

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
                D(bug("\nGot ACTION_GET_DISK_FSSM\n"));

                res = (ULONG) glob->fssm;
                break;
            }

            case ACTION_FREE_DISK_FSSM: {
                D(bug("\nGot ACTION_FREE_DISK_FSSM\n"));

                res = DOSTRUE;
                break;

            }
#endif

            case ACTION_DISK_CHANGE: { /* internal */
                struct DosList *vol = (struct DosList *)pkt->dp_Arg2;
                struct VolumeInfo *vol_info =
                    BADDR(vol->dol_misc.dol_volume.dol_LockList);
                ULONG type = pkt->dp_Arg3;

                D(bug("[FAT] DISK_CHANGE [INTERNAL]\n"));

                if (pkt->dp_Arg1 == ID_FAT_DISK) { /* security check */

                    if (AttemptLockDosList(LDF_VOLUMES|LDF_WRITE)) {

                        if (type == ACTION_VOLUME_ADD) {
                            AddDosEntry(vol);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKINSERTED);

                            D(bug("\tVolume added successfuly\n"));
                        }
                        else if (type == ACTION_VOLUME_REMOVE) {
                            RemDosEntry(vol);
                            DeletePool(vol_info->mem_pool);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKREMOVED);

                            D(bug("\tVolume removed successfuly.\n"));
                        }

                        FreeDosObject(DOS_STDPKT, pkt); /* cleanup */

                        pkt = NULL;
                        D(bug("Packet destroyed\n"));
                    }

                    else {
                        D(bug("\tDosList is locked\n"));
                        Delay(5);
                        PutMsg(glob->ourport, pkt->dp_Link);
                        pkt = NULL;
                        D(bug("Message moved to the end of the queue\n"));
                    }
                }
                else
                    err = ERROR_OBJECT_WRONG_TYPE;

                break;
            }

            case ACTION_RENAME_DISK: {
                UBYTE *name = BADDR(pkt->dp_Arg1);
                
		D(bug("[FAT] RENAME_DISK: name '"); RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

                if (glob->sb->doslist == NULL) {
                    err = glob->disk_inserted ? ERROR_NOT_A_DOS_DISK : ERROR_NO_DISK;
                    break;
                }

                while (! AttemptLockDosList(LDF_VOLUMES | LDF_WRITE))
                    ProcessPackets();

                err = SetVolumeName(glob->sb, name);
                UnLockDosList(LDF_VOLUMES | LDF_WRITE);
                if (err != 0)
                    break;

#ifdef AROS_FAST_BPTR
                /* ReadFATSuper() sets a null byte after the
                 * string, so this should be fine */
                CopyMem(glob->sb->volume.name + 1, glob->sb->doslist->dol_Name,
                    glob->sb->volume.name[0] + 1);
#else
                CopyMem(glob->sb->volume.name, BADDR(glob->sb->doslist->dol_Name),
                    glob->sb->volume.name[0] + 2);
#endif

                SendEvent(IECLASS_DISKINSERTED);

                res = DOSTRUE;

                break;
            }

            case ACTION_DELETE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                UBYTE *name = BADDR(pkt->dp_Arg2);

	        D(bug("[FAT] DELETE_OBJECT: lock 0x%08x (dir %ld/%ld) path '",
		      pkt->dp_Arg1,
		      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

                if ((err = TestLock(fl)))
                    break;

                err = OpDeleteFile(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name));
                if (err == 0)
                    res = DOSTRUE;

                break;
            }

            case ACTION_RENAME_OBJECT: {
                struct ExtFileLock *sfl = BADDR(pkt->dp_Arg1), *dfl = BADDR(pkt->dp_Arg3);
                UBYTE *sname = BADDR(pkt->dp_Arg2), *dname = BADDR(pkt->dp_Arg4);

	        D(bug("[FAT] RENAME_OBJECT: srclock 0x%08x (dir %ld/%ld) name '",
		      pkt->dp_Arg1,
		      sfl != NULL ? sfl->gl->dir_cluster : 0, sfl != NULL ? sfl->gl->dir_entry : 0);
		  RawPutChars(AROS_BSTR_ADDR(sname), AROS_BSTR_strlen(sname)); bug("' destlock 0x%08x (dir %ld/%ld) name '",
		      pkt->dp_Arg3,
		      dfl != NULL ? dfl->gl->dir_cluster : 0, dfl != NULL ? dfl->gl->dir_entry : 0);
		  RawPutChars(AROS_BSTR_ADDR(dname), AROS_BSTR_strlen(dname)); bug("'\n"));

                if ((err = TestLock(sfl)) != 0 || (err = TestLock(dfl)) != 0)
                    break;

                err = OpRenameFile(sfl, AROS_BSTR_ADDR(sname), AROS_BSTR_strlen(sname), dfl, AROS_BSTR_ADDR(dname), AROS_BSTR_strlen(dname));
                if (err == 0)
                    res = DOSTRUE;

                break;
            }

            case ACTION_CREATE_DIR: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *new;
                UBYTE *name = BADDR(pkt->dp_Arg2);

	        D(bug("[FAT] CREATE_DIR: lock 0x%08x (dir %ld/%ld) name '",
		      pkt->dp_Arg1,
		      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("'\n"));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCreateDir(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), &new)) == 0)
                    res = (IPTR)MKBADDR(new);

                break;
            }

            case ACTION_SET_PROTECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *name = BADDR(pkt->dp_Arg3);
                ULONG prot = pkt->dp_Arg4;

	        D(bug("[FAT] SET_PROTECT: lock 0x%08x (dir %ld/%ld) name '", pkt->dp_Arg2,
		      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
		  RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("' prot 0x%08x\n", prot));
                if ((err = TestLock(fl)))
                    break;

                err = OpSetProtect(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), prot);

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

		    D(bug("[FAT] SET_DATE: lock 0x%08x (dir %ld/%ld) name '",
			  pkt->dp_Arg2,
			  fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0);
		      RawPutChars(AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name)); bug("' ds '%s'\n", datestr));
                }
#endif

                if ((err = TestLock(fl)))
                    break;

                err = OpSetDate(fl, AROS_BSTR_ADDR(name), AROS_BSTR_strlen(name), ds);

                break;
            }

            case ACTION_ADD_NOTIFY: {
		struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[FAT] ADD_NOTIFY: nr 0x%08x name '%s'\n", nr, nr->nr_FullName));

                err = OpAddNotify(nr);

                break;
            }

            case ACTION_REMOVE_NOTIFY: {
		struct NotifyRequest *nr = (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[FAT] REMOVE_NOTIFY: nr 0x%08x name '%s'\n", nr, nr->nr_FullName));

                err = OpRemoveNotify(nr);
                
                break;
            }

            default:
                D(bug("[FAT] got unknown packet type %ld\n", pkt->dp_Type));

                err = ERROR_ACTION_NOT_KNOWN;
        }

        if (pkt != NULL) {
            pkt->dp_Res1 = res;
            pkt->dp_Res2 = err;
            if (!glob->quit) {
                D(bug("[FAT] replying to packet: result 0x%x, error 0x%x\n",
                    res, err));
                ReplyPacket(pkt);
            }
        }

        RestartTimer();
    }
}

void ReplyPacket(struct DosPacket *pkt) {
    struct MsgPort *rp;

            rp = pkt->dp_Port;

            pkt->dp_Port = glob->ourport;

            PutMsg(rp, pkt->dp_Link);
}

