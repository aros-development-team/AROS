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
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>  

#include "fat_fs.h"
#include "fat_protos.h"

void ProcessPackets(void) {
    struct DosPacket *packet;

    while((packet = GetPacket(glob->ourport))) {
        LONG res = DOSFALSE;
        LONG err = 0;

        switch(packet->dp_Type) {
            case ACTION_LOCATE_OBJECT: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);
                UBYTE *path = BADDR(packet->dp_Arg2);
                LONG access = packet->dp_Arg3;

                kprintf("\nGot ACTION_LOCATE_OBJECT\n");
                kprintf("\tfl = %lx key %lx ", packet->dp_Arg1, (fl ? fl->fl_Key : 0));
                kprintf("type: %s\n", (packet->dp_Arg3 == EXCLUSIVE_LOCK) ? (LONG)"exclusive" : (LONG)"shared");

                if ((err = TestLock(fl)))
                    break;

                if (path[0] != 0) {
                    err = TryLockObj(fl, &path[1], path[0], access, &res);
                }

                else if (fl != NULL) {
                    kprintf("\tCopying lock\n");
                    err = CopyLock(fl, &res);
                }

                else {
                    kprintf("\tLocking root directory.\n");
                    err = LockRoot(access, &res);
                }

                break;
            }

            case ACTION_FREE_LOCK: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);

                kprintf("\nGot FREE_LOCK\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg1, fl->fl_Key);

                if(fl)
                    FreeLock(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_COPY_DIR:
            case ACTION_COPY_DIR_FH: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);

                kprintf("\nGot ACTION_COPY_DIR\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg1, fl->fl_Key);
                
                if ((err = TestLock(fl)))
                    break;
 
                if (fl)
                    err = CopyLock(fl, &res);
                else
                    err = LockRoot(SHARED_LOCK, &res);

                break;
            }

            case ACTION_PARENT:
            case ACTION_PARENT_FH: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);

                kprintf("\nGot ACTION_Parent\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg1, fl->fl_Key);
                
                if ((err = TestLock(fl)))
                    break;
 
                if (fl && fl->fl_Key != FAT_ROOTDIR_MARK)
                    err = LockParent(fl, SHARED_LOCK, &res);
                else
                    err = ERROR_OBJECT_NOT_FOUND;

                break;
            }

            case ACTION_SAME_LOCK: {
                struct ExtFileLock *fl1 = BADDR(packet->dp_Arg1);
                struct ExtFileLock *fl2 = BADDR(packet->dp_Arg2);

                kprintf("\nGot ACTION_SAME_LOCK\n");
                err = 0;

                if (fl1 == fl2 || ((fl1->fl_Volume == fl2->fl_Volume) && fl1->fl_Key == fl2->fl_Key))
                    res = DOSTRUE;

                break;
            }

            case ACTION_EXAMINE_OBJECT:
            case ACTION_EXAMINE_FH: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);
                struct FileInfoBlock *fib = BADDR(packet->dp_Arg2);                     

                kprintf("\nGot ACTION_EXAMINE_OBJECT\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg1, fl->fl_Key);

                if ((err = TestLock(fl)))
                    break;

                if ((err = FillFIB(fl, fib)) == 0)
                    res = DOSTRUE;

                break;
            }

            case ACTION_EXAMINE_NEXT: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);
                struct FileInfoBlock *fib = BADDR(packet->dp_Arg2);
                struct DirHandle dh;
                struct DirEntry de;
                BPTR b; struct ExtFileLock *temp_lock;

                kprintf("\nGot ACTION_EXAMINE_NEXT\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg1, fl->fl_Key);

                if ((err = TestLock(fl)))
                    break;

                if ((err = InitDirHandle(glob->sb, fl->first_cluster, &dh)))
                    break;

                dh.cur_index = fib->fib_DiskKey;

                if ((err = GetNextDirEntry(&dh, &de))) {
                    if (err == ERROR_OBJECT_NOT_FOUND)
                        err = ERROR_NO_MORE_ENTRIES;
                    ReleaseDirHandle(&dh);
                    break;
                }

                if ((err = LockFile(dh.cur_index, fl->first_cluster, SHARED_LOCK, &b))) {
                    ReleaseDirHandle(&dh);
                    break;
                }
                temp_lock = BADDR(b);

                if ((err = FillFIB(temp_lock, fib))) {
                    FreeLock(temp_lock);
                    ReleaseDirHandle(&dh);
                    break;
                }

                fib->fib_DiskKey = dh.cur_index;

                FreeLock(temp_lock);

                ReleaseDirHandle(&dh);

                res = DOSTRUE;

                break;
            }

            case ACTION_FINDINPUT: {
                struct FileHandle *fh = BADDR(packet->dp_Arg1);
                struct ExtFileLock *fl = BADDR(packet->dp_Arg2);

                BPTR lock;
                UBYTE *path = BADDR(packet->dp_Arg3);

                kprintf("\nGot ACTION_FINDINPUT\n");
                kprintf("\tfl = %lx ino %lx\n", packet->dp_Arg2, (fl ? fl->fl_Key : 0));

                if ((err = TestLock(fl)))
                    break;

                /* handle empty filename */
                if (path[0] == 0) {
                    if (fl != NULL) {
                        kprintf("\tCopying lock\n");
                        err = CopyLock(fl, &lock);
                    }
                    else {
                        kprintf("\tLocking root directory.\n");
                        err = LockRoot(SHARED_LOCK, &lock);
                    }
                }

                else
                    err = TryLockObj(fl, &path[1], path[0], SHARED_LOCK, &lock);

                if (err == 0) {
                    struct ExtFileLock *fl_new = BADDR(lock);

                    if ((fl_new->attr & ATTR_DIRECTORY) == 0) {
                        fh->fh_Arg1 = (LONG)lock;
                        fh->fh_Port = DOSFALSE;

                        fl_new->pos = 0;
                        if (fl_new->first_cluster)
                            InitExtent(glob->sb, fl_new->data_ext, fl_new->first_cluster);

                        res = DOSTRUE;
                        break;
                    }
                    else
                        err = ERROR_OBJECT_WRONG_TYPE;

                    FreeLock(fl_new);
                    break;
                }

                err = ERROR_OBJECT_NOT_FOUND;
                break;
            }

            case ACTION_READ: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);
                APTR buffer = (APTR)packet->dp_Arg2;
                ULONG togo = packet->dp_Arg3;

                kprintf("\nGot ACTION_READ\n");
                kprintf("\tfl = %lx pos %ld toread %ld\n", packet->dp_Arg1, fl->pos, togo);

                if (togo + fl->pos > fl->size)
                    togo = fl->size - fl->pos;
 
                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                if ((err = File_Read(fl, togo, buffer, &res)) == 0)
                    fl->pos += res;
                else
                    res = -1;

                break;
            }

            case ACTION_SEEK: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);
                LONG newpos = packet->dp_Arg2;

                kprintf("\nGot ACTION_SEEK\n");
                kprintf("\tfl = %lx move %ld type %lx\n", packet->dp_Arg1, newpos, packet->dp_Arg3);

                if ((err = TestLock(fl))) {
                    res = -1;
                    break;
                }

                res = fl->pos;
                err = 0;

                if (packet->dp_Arg3 == OFFSET_BEGINNING && newpos >= 0 && newpos <= fl->size)
                    fl->pos = newpos;
                else if (packet->dp_Arg3 == OFFSET_CURRENT && newpos + fl->pos >= 0 && newpos + fl->pos <= fl->size)
                    fl->pos += newpos;
                else if (packet->dp_Arg3 == OFFSET_END && newpos <= 0 && fl->size + newpos >= 0)
                    fl->pos = fl->size + newpos;
                else {
                    res = -1;
                    err = ERROR_SEEK_ERROR;
                }

                break;
            }

            case ACTION_END: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);

                kprintf("\nGot ACTION_END\n");
                kprintf("\tfl = %lx\n", packet->dp_Arg1);

                if ((err = TestLock(fl)))
                    break;

                FreeLock(fl);

                res = DOSTRUE;
                break;
            }

            case ACTION_IS_FILESYSTEM:
                kprintf("\nGot ACTION_IS_FS\n");

                res = DOSTRUE;
                break;

            case ACTION_CURRENT_VOLUME: {
                struct ExtFileLock *fl = BADDR(packet->dp_Arg1);

                kprintf("\nGot ACTION_CURRENT_VOLUME\n");

                res = (fl) ? fl->fl_Volume : ((glob->sb != NULL) ? MKBADDR(glob->sb->doslist) : NULL);
                break;
            }

            case ACTION_INFO:
            case ACTION_DISK_INFO: {
                struct InfoData *id;

                if (packet->dp_Type == ACTION_INFO) {
                    struct FileLock *fl = BADDR(packet->dp_Arg1);

                    kprintf("\nGot ACTION_INFO\n");

                    if (fl && (glob->sb == NULL || fl->fl_Volume != MKBADDR(glob->sb->doslist))) {
                        err = ERROR_DEVICE_NOT_MOUNTED;
                        break;
                    }

                    id = BADDR(packet->dp_Arg2);
                }
                else {
                    kprintf("\nGot ACTION_DISK_INFO\n");
                    id = BADDR(packet->dp_Arg1);
                }

                FillDiskInfo(id);

                res = DOSTRUE;
                break;
            }

            case ACTION_INHIBIT: {
                LONG inhibit = packet->dp_Arg1;
                kprintf("\nGot ACTION_INHIBIT\n");

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
                kprintf("\nGot ACTION_DIE\n");
                if (glob->sblist != NULL || (glob->sb != NULL && glob->sb->doslist->dol_misc.dol_volume.dol_LockList != NULL)) {
                    kprintf("\tThere are some locks/volumes left. Shutting down is not possible\n");
                    err = ERROR_OBJECT_IN_USE;
                    break;
                }

                kprintf("\tNo locks pending. Shutting down the handler\n");

                DoDiskRemove(); /* risky, because of async. volume remove, but works */

                glob->quit = TRUE;
                glob->devnode->dol_Task = NULL;

                res = DOSTRUE;
                break;
            }

#if 0
            /* XXX AROS needs these ACTION_ headers defined in dos/dosextens.h */

            case ACTION_GET_DISK_FSSM: {
                kprintf("\nGot ACTION_GET_DISK_FSSM\n");

                res = (ULONG) glob->fssm;
                break;
            }

            case ACTION_FREE_DISK_FSSM: {
                kprintf("\nGot ACTION_FREE_DISK_FSSM\n");

                res = DOSTRUE;
                break;

            }
#endif

            case ACTION_DISK_CHANGE: { /* internal */
                struct DosList *vol = (struct DosList *)packet->dp_Arg2;
                ULONG type = packet->dp_Arg3;

                kprintf("\nGot ACTION_DISK_CHANGE\n");

                if (packet->dp_Arg1 == ID_FAT_DISK) { /* security check */

                    if (AttemptLockDosList(LDF_VOLUMES|LDF_WRITE)) {

                        if (type == ACTION_VOLUME_ADD) {
                            AddDosEntry(vol);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKINSERTED);

                            kprintf("\tVolume added successfuly\n");
                        }
                        else if (type == ACTION_VOLUME_REMOVE) {
                            RemDosEntry(vol);
                            FS_FreeMem(vol);
                            UnLockDosList(LDF_VOLUMES|LDF_WRITE);

                            SendEvent(IECLASS_DISKREMOVED);

                            kprintf("\tVolume removed successfuly.\n");
                        }

                        FreeDosObject(DOS_STDPKT, packet); /* cleanup */

                        packet = NULL;
                        kprintf("Packet destroyed\n");
                    }

                    else {
                        kprintf("\tDosList is locked\n");
                        Delay(5);
                        PutMsg(glob->ourport, packet->dp_Link);
                        packet = NULL;
                        kprintf("Message moved to the end of the queue\n");
                    }
                }
                else
                    err = ERROR_OBJECT_WRONG_TYPE;

                break;
            }

            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE:
            case ACTION_RENAME_DISK:
            case ACTION_WRITE:
            case ACTION_DELETE_OBJECT:
            case ACTION_RENAME_OBJECT:
            case ACTION_CREATE_DIR:
            case ACTION_SET_FILE_SIZE: {
                kprintf("\nGot unsupported write packet\n");

                err = ERROR_DISK_WRITE_PROTECTED;
                break;
            }

            default:
                kprintf("\nGot UNKNOWN %ld\n", packet->dp_Type);

                err = ERROR_ACTION_NOT_KNOWN;
                break;
        }

        if (packet)
            ReturnPacket(packet, res, err);
    }
}
