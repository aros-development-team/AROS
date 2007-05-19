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
#include <aros/debug.h>

void ProcessPackets(void) {
    struct Message *msg;
    struct DosPacket *pkt;
    struct MsgPort *rp;

    while ((msg = GetMsg(glob->ourport)) != NULL) {
        LONG res = DOSFALSE;
        LONG err = 0;

        pkt = (struct DosPacket *) msg->mn_Node.ln_Name;

        switch(pkt->dp_Type) {
            case ACTION_LOCATE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;
                UBYTE *path = BADDR(pkt->dp_Arg2);
                LONG access = pkt->dp_Arg3;

                D(bug("[fat] LOCATE_OBJECT: lock 0x%08x (dir %ld/%ld) name '%.*s' type %s\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      path[0], &path[1],
                      pkt->dp_Arg3 == EXCLUSIVE_LOCK ? "EXCLUSIVE" : "SHARED"));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpLockFile(fl, &path[1], path[0], access, &lock)) == 0)
                    res = MKBADDR(lock);

                break;
            }

            case ACTION_FREE_LOCK: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] FREE_LOCK: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                OpUnlockFile(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_COPY_DIR:
            case ACTION_COPY_DIR_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[fat] COPY_DIR: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCopyLock(fl, &lock)) == 0)
                    res = MKBADDR(lock);

                break;
            }

            case ACTION_PARENT:
            case ACTION_PARENT_FH: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[fat] ACTION_PARENT: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));
                
                if ((err = TestLock(fl)))
                    break;
 
                if ((err = OpLockParent(fl, &lock)) == 0)
                    res = MKBADDR(lock);

                break;
            }

            case ACTION_SAME_LOCK: {
                struct ExtFileLock *fl1 = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl2 = BADDR(pkt->dp_Arg2);

                D(bug("[fat] ACTION_SAME_LOCK: lock #1 0x%08x (dir %ld/%ld) lock #2 0x%08x (dir %ld/%ld)\n",
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

                D(bug("[fat] EXAMINE_OBJECT: lock 0x%08x (dir %ld/%ld)\n",
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

                D(bug("[fat] EXAMINE_NEXT: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                if ((err = InitDirHandle(glob->sb, fl->ioh.first_cluster, &dh)) != 0)
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

                if ((err = FillFIB(lock, fib))) {
                    FreeLock(lock);
                    ReleaseDirHandle(&dh);
                    break;
                }

                fib->fib_DiskKey = dh.cur_index;

                FreeLock(lock);

                ReleaseDirHandle(&dh);

                res = DOSTRUE;

                break;
            }

            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE: {
                struct FileHandle *fh = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *path = BADDR(pkt->dp_Arg3);
                struct ExtFileLock *lock;

                D(bug("[fat] %s: lock 0x%08x (dir %ld/%ld) path '%.*s'\n",
                      pkt->dp_Type == ACTION_FINDINPUT  ? "FINDINPUT"  :
                      pkt->dp_Type == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
                                                          "FINDUPDATE",
                      pkt->dp_Arg2,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      path[0], &path[1]));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpOpenFile(fl, &path[1], path[0], pkt->dp_Type, &lock)) != 0)
                    break;

                fh->fh_Arg1 = MKBADDR(lock);
                fh->fh_Port = DOSFALSE;

                res = DOSTRUE;

                break;
            }

            case ACTION_READ: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR)pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3;

                D(bug("[fat] READ: lock 0x%08x (dir %ld/%ld pos %ld) want %ld\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      want));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpRead(fl, buffer, want, &res)) != 0)
                    res = -1;

                break;
            }

            case ACTION_WRITE: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR)pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3;

                D(bug("[fat] WRITE: lock 0x%08x (dir %ld/%ld pos %ld) want %ld\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      fl->pos,
                      want));

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = OpWrite(fl, buffer, want, &res)) != 0)
                    res = -1;

                break;
            }

            case ACTION_SEEK: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                ULONG whence = pkt->dp_Arg3;

                D(bug("[fat] SEEK: lock 0x%08x (dir %ld/%ld pos %ld) offset %ld whence %s\n",
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

                D(bug("[fat] SET_FILE_SIZE: lock 0x%08x (dir %ld/%ld pos %ld) offset %ld whence %s\n",
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

                if ((err = OpSetFileSize(fl, offset, whence, &res)) != 0)
                    res = -1;

                break;
            }

            case ACTION_END: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] END: lock 0x%08x (dir %ld/%ld)\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl)))
                    break;

                FreeLock(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_IS_FILESYSTEM:
                D(bug("[fat] IS_FILESYSTEM\n"));

                res = DOSTRUE;
                break;

            case ACTION_CURRENT_VOLUME: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] CURRENT_VOLUME: lock 0x%08x\n",
                      pkt->dp_Arg1));

                res = (fl) ? fl->fl_Volume : ((glob->sb != NULL) ? MKBADDR(glob->sb->doslist) : NULL);
                break;
            }

            case ACTION_INFO:
            case ACTION_DISK_INFO: {
                struct InfoData *id;

                if (pkt->dp_Type == ACTION_INFO) {
                    struct FileLock *fl = BADDR(pkt->dp_Arg1);

                    D(bug("[fat] INFO: lock 0x%08x\n",
                          pkt->dp_Arg1));

                    if (fl && (glob->sb == NULL || fl->fl_Volume != MKBADDR(glob->sb->doslist))) {
                        err = ERROR_DEVICE_NOT_MOUNTED;
                        break;
                    }

                    id = BADDR(pkt->dp_Arg2);
                }
                else {
                    D(bug("[fat] DISK_INFO\n"));

                    id = BADDR(pkt->dp_Arg1);
                }

                FillDiskInfo(id);

                res = DOSTRUE;
                break;
            }

            case ACTION_INHIBIT: {
                LONG inhibit = pkt->dp_Arg1;

                D(bug("[fat] INHIBIT: %sinhibit\n",
                    inhibit == DOSTRUE ? "" : "un"));

                if (inhibit == DOSTRUE) {
                    glob->disk_inhibited++;
                    if (glob->disk_inhibited == 1)
                        DoDiskRemove();
                }
                else {
                    glob->disk_inhibited--;
                    if (glob->disk_inhibited == 0)
                       ProcessDiskChange();
                }

                res = DOSTRUE;
                break;
            }

            case ACTION_DIE: {
                D(bug("[fat] DIE\n"));

                if (glob->sblist != NULL || (glob->sb != NULL && glob->sb->doslist->dol_misc.dol_volume.dol_LockList != NULL)) {
                    D(bug("\tThere are some locks/volumes left. Shutting down is not possible\n"));
                    err = ERROR_OBJECT_IN_USE;
                    break;
                }

                D(bug("\tNo locks pending. Shutting down the handler\n"));

                DoDiskRemove(); /* risky, because of async. volume remove, but works */

                glob->quit = TRUE;
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
                ULONG type = pkt->dp_Arg3;

                D(bug("[fat] DISK_CHANGE [INTERNAL]\n"));

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
                            FreeVecPooled(glob->mempool, vol);
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
                
                D(bug("[fat] RENAME_DISK: name '%.*s'\n",
                      name[0], &name[1]));

                if (glob->sb->doslist == NULL) {
                    err = glob->disk_inserted ? ERROR_NOT_A_DOS_DISK : ERROR_NO_DISK;
                    break;
                }

                while (! AttemptLockDosList(LDF_VOLUMES | LDF_WRITE))
                    ProcessPackets();

                if ((err = SetVolumeName(glob->sb, name)) != 0)
                    break;

                UnLockDosList(LDF_VOLUMES | LDF_WRITE);

                SendEvent(IECLASS_DISKINSERTED);

                res = DOSTRUE;

                break;
            }

            case ACTION_DELETE_OBJECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                UBYTE *name = BADDR(pkt->dp_Arg2);

                D(bug("[fat] DELETE_OBJECT: lock 0x%08x (dir %ld/%ld) path '%.*s'\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      name[0], &name[1]));

                if ((err = TestLock(fl)))
                    break;

                err = OpDeleteFile(fl, &name[1], name[0]);

                break;
            }

            case ACTION_RENAME_OBJECT: {
                struct ExtFileLock *sfl = BADDR(pkt->dp_Arg1), *dfl = BADDR(pkt->dp_Arg3);
                UBYTE *sname = BADDR(pkt->dp_Arg2), *dname = BADDR(pkt->dp_Arg4);

                D(bug("[fat] RENAME_OBJECT: srclock 0x%08x (dir %ld/%ld) name '%.*s' destlock 0x%08x (dir %ld/%ld) name '%.*s'\n",
                      pkt->dp_Arg1,
                      sfl != NULL ? sfl->gl->dir_cluster : 0, sfl != NULL ? sfl->gl->dir_entry : 0,
                      sname[0], &sname[1],
                      pkt->dp_Arg3,
                      dfl != NULL ? dfl->gl->dir_cluster : 0, dfl != NULL ? dfl->gl->dir_entry : 0,
                      dname[0], &dname[1]));

                if ((err = TestLock(sfl)) != 0 || (err = TestLock(dfl)) != 0)
                    break;

                err = OpRenameFile(sfl, &sname[1], sname[0], dfl, &dname[1], dname[0]);

                break;
            }

            case ACTION_CREATE_DIR: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *new;
                UBYTE *name = BADDR(pkt->dp_Arg2);

                D(bug("[fat] CREATE_DIR: lock 0x%08x (dir %ld/%ld) name '%.*s'\n",
                      pkt->dp_Arg1,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      name[0], &name[1]));

                if ((err = TestLock(fl)))
                    break;

                if ((err = OpCreateDir(fl, &name[1], name[0], &new)) == 0)
                    res = MKBADDR(new);

                break;
            }

            case ACTION_SET_PROTECT: {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                UBYTE *name = BADDR(pkt->dp_Arg3);
                ULONG prot = pkt->dp_Arg4;

                D(bug("[fat] SET_PROTECT: lock 0x%08x (dir %ld/%ld) name '%.*s' prot 0x%08x\n",
                      pkt->dp_Arg2,
                      fl != NULL ? fl->gl->dir_cluster : 0, fl != NULL ? fl->gl->dir_entry : 0,
                      name[0], &name[1],
                      prot));

                if ((err = TestLock(fl)))
                    break;

                err = OpSetProtect(fl, &name[1], name[0], prot);

                break;
            }

            case ACTION_ADD_NOTIFY: {
                struct NotifyRequest *nr = pkt->dp_Arg1;

                D(bug("[fat] ADD_NOTIFY: nr 0x%08x name '%s'\n", nr, nr->nr_FullName));

                err = OpAddNotify(nr);

                break;
            }

            case ACTION_REMOVE_NOTIFY: {
                struct NotifyRequest *nr = pkt->dp_Arg1;

                D(bug("[fat] REMOVE_NOTIFY: nr 0x%08x name '%s'\n", nr, nr->nr_FullName));

                err = OpRemoveNotify(nr);
                
                break;
            }

            default:
                D(bug("[fat] got unknown packet type %ld\n", pkt->dp_Type));

                err = ERROR_ACTION_NOT_KNOWN;
        }

        if (pkt != NULL) {
            D(bug("[fat] replying to packet: result 0x%x, error 0x%x\n", res, err));

            rp = pkt->dp_Port;

            pkt->dp_Port = glob->ourport;
            pkt->dp_Res1 = res;
            pkt->dp_Res2 = err;

            PutMsg(rp, pkt->dp_Link);
        }

#if defined(DEBUG_CACHESTATS) && DEBUG_CACHESTATS != 0
        cache_stats(glob->sb->cache);
#endif
    }
}
