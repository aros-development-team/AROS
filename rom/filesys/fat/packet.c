/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright (C) 2006 Marek Szyprowski
 * Copyright (C) 2007-2015 The AROS Development Team
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

void ProcessPackets(struct Globals *glob)
{
    struct Message *msg;
    struct DosPacket *pkt;

    while ((msg = GetMsg(glob->ourport)) != NULL)
    {
        IPTR res = DOSFALSE;
        LONG err = 0;

        pkt = (struct DosPacket *)msg->mn_Node.ln_Name;

        switch (pkt->dp_Type)
        {
        case ACTION_LOCATE_OBJECT:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;
                LONG access = pkt->dp_Arg3;

                D(
                    bug("[fat] LOCATE_OBJECT: lock 0x%08x (dir %ld/%ld) name '",
                        pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                        fl != NULL ? fl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg2),
                        AROS_BSTR_strlen(pkt->dp_Arg2));
                    bug("' type %s\n", pkt->dp_Arg3 == EXCLUSIVE_LOCK ?
                        "EXCLUSIVE" : "SHARED");
                )

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = OpLockFile(fl, AROS_BSTR_ADDR(pkt->dp_Arg2),
                    AROS_BSTR_strlen(pkt->dp_Arg2), access, &lock,
                    glob)) == 0)
                    res = (IPTR) MKBADDR(lock);

                break;
            }

        case ACTION_FREE_LOCK:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] FREE_LOCK: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                OpUnlockFile(fl, glob);

                res = DOSTRUE;
                break;
            }

        case ACTION_COPY_DIR:
        case ACTION_COPY_DIR_FH:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[fat] COPY_DIR: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = OpCopyLock(fl, &lock, glob)) == 0)
                    res = (IPTR) MKBADDR(lock);

                break;
            }

        case ACTION_PARENT:
        case ACTION_PARENT_FH:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;

                D(bug("[fat] ACTION_PARENT: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = OpLockParent(fl, &lock, glob)) == 0)
                    res = (IPTR) MKBADDR(lock);

                break;
            }

        case ACTION_SAME_LOCK:
            {
                struct ExtFileLock *fl1 = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl2 = BADDR(pkt->dp_Arg2);

                D(bug("[fat] ACTION_SAME_LOCK: lock #1 0x%08x (dir %ld/%ld)"
                    " lock #2 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl1 != NULL ? fl1->gl->dir_cluster : 0,
                    fl1 != NULL ? fl1->gl->dir_entry : 0, pkt->dp_Arg2,
                    fl2 != NULL ? fl2->gl->dir_cluster : 0,
                    fl2 != NULL ? fl2->gl->dir_entry : 0));

                err = 0;

                if (fl1 == fl2 || fl1->gl == fl2->gl)
                    res = DOSTRUE;

                break;
            }

        case ACTION_EXAMINE_OBJECT:
        case ACTION_EXAMINE_FH:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);

                D(bug("[fat] EXAMINE_OBJECT: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = FillFIB(fl, fib, glob)) == 0)
                    res = DOSTRUE;

                break;
            }

        case ACTION_EXAMINE_NEXT:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *lock;
                struct FileInfoBlock *fib = BADDR(pkt->dp_Arg2);
                struct DirHandle dh;
                struct DirEntry de;

                D(bug("[fat] EXAMINE_NEXT: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = InitDirHandle(glob->sb, fl->ioh.first_cluster, &dh,
                    FALSE, glob)) != 0)
                    break;

                dh.cur_index = fib->fib_DiskKey;

                if ((err = GetNextDirEntry(&dh, &de, glob)) != 0)
                {
                    if (err == ERROR_OBJECT_NOT_FOUND)
                        err = ERROR_NO_MORE_ENTRIES;
                    ReleaseDirHandle(&dh, glob);
                    break;
                }

                if ((err = LockFile(fl->ioh.first_cluster, dh.cur_index,
                    SHARED_LOCK, &lock, glob)) != 0)
                {
                    ReleaseDirHandle(&dh, glob);
                    break;
                }

                if (!(err = FillFIB(lock, fib, glob)))
                {
                    fib->fib_DiskKey = dh.cur_index;
                    res = DOSTRUE;
                }

                FreeLock(lock, glob);
                ReleaseDirHandle(&dh, glob);

                break;
            }

        case ACTION_FINDINPUT:
        case ACTION_FINDOUTPUT:
        case ACTION_FINDUPDATE:
            {
                struct FileHandle *fh = BADDR(pkt->dp_Arg1);
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                struct ExtFileLock *lock;

                D(
                    bug("[fat] %s: lock 0x%08x (dir %ld/%ld) path '",
                        pkt->dp_Type == ACTION_FINDINPUT ? "FINDINPUT" :
                        pkt->dp_Type == ACTION_FINDOUTPUT ? "FINDOUTPUT" :
                        "FINDUPDATE",
                        pkt->dp_Arg2,
                        fl != NULL ? fl->gl->dir_cluster : 0,
                        fl != NULL ? fl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg3),
                        AROS_BSTR_strlen(pkt->dp_Arg3));
                    bug("'\n");
                )

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = OpOpenFile(fl, AROS_BSTR_ADDR(pkt->dp_Arg3),
                    AROS_BSTR_strlen(pkt->dp_Arg3), pkt->dp_Type,
                    &lock, glob)) != 0)
                    break;

                fh->fh_Arg1 = (IPTR) MKBADDR(lock);
                fh->fh_Port = DOSFALSE;

                res = DOSTRUE;

                break;
            }

        case ACTION_READ:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR) pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3, read;

                D(bug("[fat] READ: lock 0x%08x (dir %ld/%ld pos %ld)"
                    " want %ld\n",
                    pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0, fl->pos, want));

                if ((err = TestLock(fl, glob)))
                {
                    res = -1;
                    break;
                }

                if ((err = OpRead(fl, buffer, want, &read, glob)) != 0)
                    res = -1;
                else
                    res = read;

                break;
            }

        case ACTION_WRITE:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                APTR buffer = (APTR) pkt->dp_Arg2;
                ULONG want = pkt->dp_Arg3, written;

                D(bug("[fat] WRITE: lock 0x%08x (dir %ld/%ld pos %ld)"
                    " want %ld\n",
                    pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0, fl->pos, want));

                if ((err = TestLock(fl, glob)))
                {
                    res = -1;
                    break;
                }

                if ((err = OpWrite(fl, buffer, want, &written, glob)) != 0)
                    res = -1;
                else
                    res = written;

                break;
            }

        case ACTION_SEEK:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                ULONG whence = pkt->dp_Arg3;

                D(bug("[fat] SEEK: lock 0x%08x (dir %ld/%ld pos %ld)"
                    " offset %ld whence %s\n",
                    pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0, fl->pos, offset,
                    whence == OFFSET_BEGINNING ? "BEGINNING" :
                    whence == OFFSET_END ? "END" :
                    whence == OFFSET_CURRENT ? "CURRENT" :
                    "(unknown)"));

                if ((err = TestLock(fl, glob)))
                {
                    res = -1;
                    break;
                }

                res = fl->pos;
                err = 0;

                if (whence == OFFSET_BEGINNING
                    && offset >= 0 && offset <= fl->gl->size)
                    fl->pos = offset;
                else if (whence == OFFSET_CURRENT
                    && offset + fl->pos >= 0
                    && offset + fl->pos <= fl->gl->size)
                    fl->pos += offset;
                else if (whence == OFFSET_END
                    && offset <= 0 && fl->gl->size + offset >= 0)
                    fl->pos = fl->gl->size + offset;
                else
                {
                    res = -1;
                    err = ERROR_SEEK_ERROR;
                }

                break;
            }

        case ACTION_SET_FILE_SIZE:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);
                LONG offset = pkt->dp_Arg2;
                LONG whence = pkt->dp_Arg3;
                LONG newsize;

                D(bug("[fat] SET_FILE_SIZE: lock 0x%08x"
                    " (dir %ld/%ld pos %ld) offset %ld whence %s\n",
                    pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0, fl->pos, offset,
                    whence == OFFSET_BEGINNING ? "BEGINNING" :
                    whence == OFFSET_END ? "END" :
                    whence == OFFSET_CURRENT ? "CURRENT" :
                    "(unknown)"));

                if ((err = TestLock(fl, glob)))
                {
                    res = -1;
                    break;
                }

                if ((err = OpSetFileSize(fl, offset, whence, &newsize,
                    glob)) != 0)
                    res = -1;
                else
                    res = newsize;

                break;
            }

        case ACTION_END:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] END: lock 0x%08x (dir %ld/%ld)\n",
                    pkt->dp_Arg1,
                    fl != NULL ? fl->gl->dir_cluster : 0,
                    fl != NULL ? fl->gl->dir_entry : 0));

                if ((err = TestLock(fl, glob)))
                    break;

                FreeLock(fl, glob);

                res = DOSTRUE;
                break;
            }

        case ACTION_IS_FILESYSTEM:
            D(bug("[fat] IS_FILESYSTEM\n"));

            res = DOSTRUE;
            break;

        case ACTION_CURRENT_VOLUME:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(bug("[fat] CURRENT_VOLUME: lock 0x%08x\n", pkt->dp_Arg1));

                res = (IPTR) ((fl) ? fl->fl_Volume : ((glob->sb != NULL) ?
                    MKBADDR(glob->sb->doslist) : BNULL));
                break;
            }

        case ACTION_INFO:
        case ACTION_DISK_INFO:
            {
                struct InfoData *id;

                if (pkt->dp_Type == ACTION_INFO)
                {
                    struct FileLock *fl = BADDR(pkt->dp_Arg1);

                    D(bug("[fat] INFO: lock 0x%08x\n", pkt->dp_Arg1));

                    if (fl && (glob->sb == NULL
                        || fl->fl_Volume != MKBADDR(glob->sb->doslist)))
                    {
                        err = ERROR_DEVICE_NOT_MOUNTED;
                        break;
                    }

                    id = BADDR(pkt->dp_Arg2);
                }
                else
                {
                    D(bug("[fat] DISK_INFO\n"));

                    id = BADDR(pkt->dp_Arg1);
                }

                FillDiskInfo(id, glob);

                res = DOSTRUE;
                break;
            }

        case ACTION_INHIBIT:
            {
                LONG inhibit = pkt->dp_Arg1;

                D(bug("[fat] INHIBIT: %sinhibit\n",
                    inhibit == DOSTRUE ? "" : "un"));

                if (inhibit == DOSTRUE)
                {
                    glob->disk_inhibited++;
                    if (glob->disk_inhibited == 1)
                        DoDiskRemove(glob);
                }
                else if (glob->disk_inhibited != 0)
                {
                    glob->disk_inhibited--;
                    if (glob->disk_inhibited == 0)
                        ProcessDiskChange(glob);
                }

                res = DOSTRUE;
                break;
            }

        case ACTION_DIE:
            {
                struct FSSuper *sb;
                struct NotifyNode *nn;

                D(bug("[fat] DIE\n"));

                /* Clear our message port from notification requests so DOS
                 * won't send notification-end packets to us after we're gone */
                ForeachNode(&glob->sblist, sb)
                {
                    ForeachNode(&sb->info->notifies, nn)
                    {
                        nn->nr->nr_Handler = NULL;
                    }
                }

                if ((glob->sb != NULL
                    && !(IsListEmpty(&glob->sb->info->locks)
                    && IsListEmpty(&glob->sb->info->notifies))))
                {

                    D(bug("\tThere are remaining locks or notification "
                        "requests. Shutting down is not possible\n"));

                    err = ERROR_OBJECT_IN_USE;
                    break;
                }

                D(bug("\tNothing pending. Shutting down the handler\n"));

                /* Risky, because of async. volume remove, but works */
                DoDiskRemove(glob);

                glob->quit = TRUE;
                glob->death_packet = pkt;
                glob->devnode->dol_Task = NULL;

                res = DOSTRUE;
                break;
            }

#if 0
            /* XXX: AROS needs these ACTION_ headers defined in dos/dosextens.h */

        case ACTION_GET_DISK_FSSM:
            {
                D(bug("\nGot ACTION_GET_DISK_FSSM\n"));

                res = (ULONG) glob->fssm;
                break;
            }

        case ACTION_FREE_DISK_FSSM:
            {
                D(bug("\nGot ACTION_FREE_DISK_FSSM\n"));

                res = DOSTRUE;
                break;

            }
#endif

        case ACTION_DISK_CHANGE:    /* Internal */
            {
                struct DosList *vol = (struct DosList *)pkt->dp_Arg2;
                struct VolumeInfo *vol_info =
                    BADDR(vol->dol_misc.dol_volume.dol_LockList);
                ULONG type = pkt->dp_Arg3;

                D(bug("[fat] DISK_CHANGE [INTERNAL]\n"));

                if (pkt->dp_Arg1 == ID_FAT_DISK)    /* Security check */
                {
                    if (AttemptLockDosList(LDF_VOLUMES | LDF_WRITE))
                    {
                        if (type == ACTION_VOLUME_ADD)
                        {
                            AddDosEntry(vol);
                            UnLockDosList(LDF_VOLUMES | LDF_WRITE);

                            SendEvent(IECLASS_DISKINSERTED, glob);

                            D(bug("\tVolume added\n"));
                        }
                        else if (type == ACTION_VOLUME_REMOVE)
                        {
                            RemDosEntry(vol);
                            DeletePool(vol_info->mem_pool);
                            UnLockDosList(LDF_VOLUMES | LDF_WRITE);

                            SendEvent(IECLASS_DISKREMOVED, glob);

                            D(bug("\tVolume removed\n"));
                        }

                        FreeDosObject(DOS_STDPKT, pkt); /* Cleanup */

                        pkt = NULL;
                        D(bug("Packet destroyed\n"));
                    }

                    else
                    {
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

        case ACTION_RENAME_DISK:
            {

                D(
                    bug("[fat] RENAME_DISK: name '");
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg1),
                        AROS_BSTR_strlen(pkt->dp_Arg1));
                    bug("'\n");
                )

                if (glob->sb->doslist == NULL)
                {
                    err = glob->disk_inserted ?
                        ERROR_NOT_A_DOS_DISK : ERROR_NO_DISK;
                    break;
                }

                while (!AttemptLockDosList(LDF_VOLUMES | LDF_WRITE))
                    ProcessPackets(glob);

                err = SetVolumeName(glob->sb, AROS_BSTR_ADDR(pkt->dp_Arg1),
                    AROS_BSTR_strlen(pkt->dp_Arg1));
                UnLockDosList(LDF_VOLUMES | LDF_WRITE);
                if (err != 0)
                    break;

#ifdef AROS_FAST_BPTR
                /* ReadFATSuper() sets a null byte after the
                 * string, so this should be fine */
                CopyMem(glob->sb->volume.name + 1,
                    glob->sb->doslist->dol_Name,
                    glob->sb->volume.name[0] + 1);
#else
                CopyMem(glob->sb->volume.name,
                    BADDR(glob->sb->doslist->dol_Name),
                    glob->sb->volume.name[0] + 2);
#endif

                CopyMem(glob->sb->volume.name,
                    glob->sb->info->root_lock.name,
                    glob->sb->volume.name[0] + 1);

                res = DOSTRUE;

                break;
            }

        case ACTION_FORMAT:
            {
                D(
                    bug("[fat] FORMAT: name '");
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg1),
                        AROS_BSTR_strlen(pkt->dp_Arg1));
                    bug("'\n");
                )

                if (!glob->disk_inserted)
                {
                    err = ERROR_NO_DISK;
                    break;
                }

                err = FormatFATVolume(AROS_BSTR_ADDR(pkt->dp_Arg1),
                    AROS_BSTR_strlen(pkt->dp_Arg1), glob);
                if (err != 0)
                    break;

                SendEvent(IECLASS_DISKINSERTED, glob);

                res = DOSTRUE;

                break;
            }

        case ACTION_DELETE_OBJECT:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1);

                D(
                    bug("[fat] DELETE_OBJECT:"
                        " lock 0x%08x (dir %ld/%ld) path '",
                        pkt->dp_Arg1, fl != NULL ? fl->gl->dir_cluster : 0,
                        fl != NULL ? fl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg2),
                        AROS_BSTR_strlen(pkt->dp_Arg2));
                    bug("'\n");
                )

                if ((err = TestLock(fl, glob)))
                    break;

                err = OpDeleteFile(fl, AROS_BSTR_ADDR(pkt->dp_Arg2),
                    AROS_BSTR_strlen(pkt->dp_Arg2), glob);
                if (err == 0)
                    res = DOSTRUE;

                break;
            }

        case ACTION_RENAME_OBJECT:
            {
                struct ExtFileLock *sfl = BADDR(pkt->dp_Arg1), *dfl =
                    BADDR(pkt->dp_Arg3);

                D(
                    bug("[fat] RENAME_OBJECT:"
                    " srclock 0x%08x (dir %ld/%ld) name '",
                        pkt->dp_Arg1,
                        sfl != NULL ? sfl->gl->dir_cluster : 0,
                        sfl != NULL ? sfl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg2),
                        AROS_BSTR_strlen(pkt->dp_Arg2));
                    bug("' destlock 0x%08x (dir %ld/%ld) name '",
                        pkt->dp_Arg3,
                        dfl != NULL ? dfl->gl->dir_cluster : 0,
                        dfl != NULL ? dfl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg4),
                        AROS_BSTR_strlen(pkt->dp_Arg4));
                    bug("'\n");
                )

                if ((err = TestLock(sfl, glob)) != 0
                    || (err = TestLock(dfl, glob)) != 0)
                    break;

                err = OpRenameFile(sfl, AROS_BSTR_ADDR(pkt->dp_Arg2),
                    AROS_BSTR_strlen(pkt->dp_Arg2), dfl,
                    AROS_BSTR_ADDR(pkt->dp_Arg4),
                    AROS_BSTR_strlen(pkt->dp_Arg4), glob);
                if (err == 0)
                    res = DOSTRUE;

                break;
            }

        case ACTION_CREATE_DIR:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg1), *new;

                D(
                    bug("[fat] CREATE_DIR: lock 0x%08x (dir %ld/%ld) name '",
                        pkt->dp_Arg1,
                        fl != NULL ? fl->gl->dir_cluster : 0,
                        fl != NULL ? fl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg2),
                        AROS_BSTR_strlen(pkt->dp_Arg2));
                    bug("'\n");
                )

                if ((err = TestLock(fl, glob)))
                    break;

                if ((err = OpCreateDir(fl, AROS_BSTR_ADDR(pkt->dp_Arg2),
                    AROS_BSTR_strlen(pkt->dp_Arg2), &new, glob)) == 0)
                    res = (IPTR) MKBADDR(new);

                break;
            }

        case ACTION_SET_PROTECT:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
                ULONG prot = pkt->dp_Arg4;

                D(
                    bug("[fat] SET_PROTECT: lock 0x%08x (dir %ld/%ld) name '",
                        pkt->dp_Arg2, fl != NULL ? fl->gl->dir_cluster : 0,
                        fl != NULL ? fl->gl->dir_entry : 0);
                    RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg3),
                        AROS_BSTR_strlen(pkt->dp_Arg3));
                    bug("' prot 0x%08x\n", prot);
                )
                if ((err = TestLock(fl, glob)))
                    break;

                err = OpSetProtect(fl, AROS_BSTR_ADDR(pkt->dp_Arg3),
                    AROS_BSTR_strlen(pkt->dp_Arg3), prot, glob);

                break;
            }

        case ACTION_SET_DATE:
            {
                struct ExtFileLock *fl = BADDR(pkt->dp_Arg2);
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

                    D(
                        bug("[fat] SET_DATE: lock 0x%08x (dir %ld/%ld) name '",
                            pkt->dp_Arg2,
                            fl != NULL ? fl->gl->dir_cluster : 0,
                            fl != NULL ? fl->gl->dir_entry : 0);
                        RawPutChars(AROS_BSTR_ADDR(pkt->dp_Arg3),
                            AROS_BSTR_strlen(pkt->dp_Arg3));
                        bug("' ds '%s'\n", datestr);
                    )
                }
#endif

                if ((err = TestLock(fl, glob)))
                    break;

                err = OpSetDate(fl, AROS_BSTR_ADDR(pkt->dp_Arg3),
                    AROS_BSTR_strlen(pkt->dp_Arg3), ds, glob);

                break;
            }

        case ACTION_ADD_NOTIFY:
            {
                struct NotifyRequest *nr =
                    (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[fat] ADD_NOTIFY: nr 0x%08x name '%s'\n", nr,
                    nr->nr_FullName));

                err = OpAddNotify(nr, glob);

                break;
            }

        case ACTION_REMOVE_NOTIFY:
            {
                struct NotifyRequest *nr =
                    (struct NotifyRequest *)pkt->dp_Arg1;

                D(bug("[fat] REMOVE_NOTIFY: nr 0x%08x name '%s'\n", nr,
                    nr->nr_FullName));

                err = OpRemoveNotify(nr, glob);

                break;
            }

        default:
            D(bug("[fat] got unknown packet type %ld\n", pkt->dp_Type));

            err = ERROR_ACTION_NOT_KNOWN;
        }

        if (pkt != NULL)
        {
            pkt->dp_Res1 = res;
            pkt->dp_Res2 = err;
            if (!glob->quit)
            {
                D(bug("[fat] replying to packet: result 0x%lx, error %ld\n",
                    res, err));
                ReplyPacket(pkt, SysBase);
            }
        }

        RestartTimer(glob);
    }
}

#undef SysBase

void ReplyPacket(struct DosPacket *dp, struct ExecBase *SysBase)
{
    struct MsgPort *rp;
    struct Message *mn;

    rp = dp->dp_Port;
    mn = dp->dp_Link;
    dp->dp_Port = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    PutMsg(rp, mn);
}
