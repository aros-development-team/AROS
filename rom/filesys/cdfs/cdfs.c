/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "cdfs.h"
#include "iso9660.h"
#include "bcache.h"

static LONG CDFS_DeviceMount(struct CDFS *cdfs, struct CDFSDevice *dev)
{
    struct CDFSVolume *vol;
    LONG err = ERROR_NO_FREE_STORE;
    const struct CDFSOps *op = &ISO9660_Ops;

    vol = AllocVec(sizeof(*vol), MEMF_PUBLIC | MEMF_CLEAR);
    if (vol) {
        vol->cv_Device = dev;
        vol->cv_CDFSBase = cdfs;
        vol->cv_DosVolume.dl_Type = DLT_VOLUME;
        vol->cv_DosVolume.dl_DiskType = AROS_MAKE_ID('C','D','F','S');
        vol->cv_DosVolume.dl_Task = &((struct Process *)FindTask(NULL))->pr_MsgPort;
        NEWLIST(&vol->cv_FileLocks);

        err = op->op_Mount(vol);
        if (err == RETURN_OK) {
            struct Library *DOSBase;
            DOSBase = OpenLibrary("dos.library",0);

            vol->cv_Ops = op;

            if (DOSBase) {
                /* Look for an identical volume */
                struct DosList *dl;
                dl = LockDosList(LDF_VOLUMES | LDF_READ);
                while (dl != BNULL) {
                    dl = FindDosEntry(dl, AROS_BSTR_ADDR(vol->cv_DosVolume.dl_Name), LDF_VOLUMES | LDF_READ);
                    if (dl && dl->dol_misc.dol_volume.dol_DiskType == vol->cv_DosVolume.dl_DiskType
                           && memcmp(&vol->cv_DosVolume.dl_VolumeDate, &dl->dol_misc.dol_volume.dol_VolumeDate, sizeof(struct DateStamp)) == 0
                           && dl->dol_misc.dol_volume.dol_LockList != BNULL  // Dismounted
                       ) {
                        /* Identical match found */
                        break;
                    }
                }
                if (dl) {
                    /* Dispose of the test volume */
                    vol->cv_Ops->op_Unmount(vol);
                    FreeVec(vol);

                    /* Use the old volume */
                    vol = dev->cd_Volume = B_VOLUME(MKBADDR(dl));
                    D(bug("%s: Remounting '%b'\n", __func__, dl->dol_Name));

                    /* Mark volume as online */
                    vol->cv_DosVolume.dl_Task = &(((struct Process *)FindTask(NULL))->pr_MsgPort);
                    vol->cv_DosVolume.dl_LockList = BNULL;
                }
                UnLockDosList(LDF_VOLUMES | LDF_READ);
                CloseLibrary(DOSBase);
            }
            dev->cd_Volume = vol;
            return RETURN_OK;
        }
        FreeVec(vol);
    }

    dev->cd_Volume = NULL;
    return err;
}


static VOID CDFS_DeviceUnmount(struct CDFS *cdfs, struct CDFSDevice *dev)
{
    struct CDFSVolume *vol = dev->cd_Volume;

    if (vol == NULL)
        return;

    /* Detach from the device */
    dev->cd_Volume = NULL;
    /* Still has locks? */
    if (!IsListEmpty(&vol->cv_FileLocks)) {
        struct CDFSLock *cl;
        BPTR *flp;

        /* Mark the volume as dismounted  */
        vol->cv_DosVolume.dl_Task = NULL;

        /* Make the dismounted filelock list */
        flp = &vol->cv_DosVolume.dl_LockList;
        ForeachNode(&vol->cv_FileLocks, cl) {
            *flp = MKBADDR(&cl->cl_FileLock);
            flp = &cl->cl_FileLock.fl_Link;
        }
        *flp = BNULL;

        /* Put the volume in the CDFS volume list */
        ObtainSemaphore(&cdfs->cb_Semaphore);
        AddHead(&cdfs->cb_Volumes, (struct Node *)&vol->cv_Node);
        ReleaseSemaphore(&cdfs->cb_Semaphore);
    } else {
        struct Library *DOSBase = OpenLibrary("dos.library",0);
        if (DOSBase) {
            LockDosList(LDF_VOLUMES | LDF_WRITE | LDF_DELETE);
            RemDosEntry((struct DosList *)&vol->cv_DosVolume);
            UnLockDosList(LDF_VOLUMES | LDF_WRITE | LDF_DELETE);
        }
        CloseLibrary(DOSBase);

        /* Dispose of the volume */
        vol->cv_Ops->op_Unmount(vol);
        FreeVec(vol);
    }
}

static LONG CDFS_DeviceOpen(struct CDFS *cdfs, struct FileSysStartupMsg *fssm, struct CDFSDevice **devp)
{
    struct CDFSDevice *dev;
    LONG err;

    dev = AllocVec(sizeof(*dev), MEMF_PUBLIC | MEMF_CLEAR);
    if (dev) {
        err = BCache_Create(SysBase, fssm, &dev->cd_BCache);
        if (err == RETURN_OK) {
            D(bug("%s: BCache created\n", __func__));
            ObtainSemaphore(&cdfs->cb_Semaphore);
            AddTail(&cdfs->cb_Devices, (struct Node *)&dev->cd_Node);
            ReleaseSemaphore(&cdfs->cb_Semaphore);
            D(bug("%s: Device opened\n", __func__));
            *devp = dev;
            return RETURN_OK;
        }
        FreeVec(dev);
    } else {
        err = ERROR_NO_FREE_STORE;
    }
    *devp = NULL;
    return err;
}

static VOID CDFS_DeviceClose(struct CDFS *cdfs, struct CDFSDevice *dev)
{
    if (dev->cd_Volume) {
        CDFS_DeviceUnmount(cdfs, dev);
    }
    BCache_Delete(dev->cd_BCache);
    ObtainSemaphore(&cdfs->cb_Semaphore);
    Remove((struct Node *)dev);
    ReleaseSemaphore(&cdfs->cb_Semaphore);
    FreeVec(dev);
}

static struct CDFSVolume *CDFS_DevicePresent(struct CDFS *cdfs, struct CDFSDevice *dev, struct CDFSLock **fl, SIPTR *res, SIPTR *res2)
{
    *res2 = BCache_Present(dev->cd_BCache);
    if (*res2 != RETURN_OK) {
        D(bug("%s: BCache error %d\n", *res2));
        *res = DOSFALSE;
    } else
        *res = DOSTRUE;

    if (*res != DOSTRUE && dev->cd_Volume) {
        D(bug("%s: Disk change detected, unmounting volume\n", __func__));
        CDFS_DeviceUnmount(cdfs, dev);
        if (*res2 == RETURN_WARN) {
            BCache_Invalidate(dev->cd_BCache);
            *res2 = CDFS_DeviceMount(cdfs, dev);
            D(bug("%s: New disk present, %svolume present\n", __func__, (*res2 == RETURN_OK) ? "" : "no "));
            if (*res2 == RETURN_OK)
                *res = DOSTRUE;
        }
    }

    /* Mount a volume if possible */
    if (*res == DOSTRUE && dev->cd_Volume == NULL) {
        BCache_Invalidate(dev->cd_BCache);
        *res2 = CDFS_DeviceMount(cdfs, dev);
        D(bug("%s: Disk present, %svolume present\n", __func__, (*res2 == RETURN_OK) ? "" : "no "));
        if (*res2 == RETURN_OK)
            *res = DOSTRUE;
    }

    /* Fix up *fl if it is NULL to the Root directory link */
    if (*res == DOSTRUE && *fl == NULL) {
        if (dev->cd_Volume) {
            *fl = B_LOCK(dev->cd_Volume->cv_DosVolume.dl_Lock);
        } else {
            *res2 = ERROR_NOT_A_DOS_DISK;
            *res = DOSFALSE;
        }
    }

    /* Verify that *fl points to the currently mounted volume */
    if (*res == DOSTRUE && *fl) {
        struct CDFSVolume *lv = B_VOLUME((*fl)->cl_FileLock.fl_Volume);
        if (lv != dev->cd_Volume) {
            D(bug("%s: Attempt access lock from volume %p, but mounted is %p\n", __func__, lv, dev->cd_Volume));
            *res2 = ERROR_INVALID_LOCK;
            *res = DOSFALSE;
        }
    }

    return (*res == DOSTRUE) ? dev->cd_Volume : NULL;
}

#undef SysBase

static struct CDFS *CDFS_Init(struct ExecBase *SysBase)
{
    struct CDFS *dispose, *cdfs;

    dispose = AllocVec(sizeof(*cdfs), MEMF_ANY | MEMF_CLEAR);
    if (dispose == NULL)
        return NULL;

    Forbid();
    cdfs = (struct CDFS *)FindSemaphore("CDFS");
    if (cdfs == NULL) {
        if ((dispose->cb_UtilityBase = OpenLibrary("utility.library",0))) {
            cdfs = dispose;
            dispose = NULL;
            InitSemaphore(&cdfs->cb_Semaphore);
            cdfs->cb_Semaphore.ss_Link.ln_Name = "CDFS";
            /* Set up SysBase link */
            cdfs->cb_SysBase = SysBase;
            NEWLIST(&cdfs->cb_Devices);
            AddSemaphore(&cdfs->cb_Semaphore);
        }
    }
    Permit();

    if (dispose)
        FreeVec(dispose);

    return cdfs;
}

#if DEBUG
#define ACTION_(x)  ACTION_##x: D(bug("%s: ACTION_" #x "\n", __func__)); goto case_##x; case_##x
#else
#define ACTION_(x)  ACTION_##x
#endif

LONG CDFS_Handler(struct ExecBase *SysBase)
{
    struct MsgPort *mp;
    struct DosPacket *dp;
    struct Message *mn;
    struct CDFS *cdfs;
    struct CDFSDevice *dev;
    LONG retval;
    BOOL die = FALSE;
    ULONG sigpacket;

    void reply(struct DosPacket *dp, SIPTR res, SIPTR res2)
    {
        struct MsgPort *mp;
        struct Message *mn;

        D(bug("Reply %d => %p (%d)\n", dp->dp_Type, res, res2));
        mp = dp->dp_Port;
        mn = dp->dp_Link;
        mn->mn_Node.ln_Name = (char*)dp;
        dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
        dp->dp_Res1 = res;
        dp->dp_Res2 = res2;
        PutMsg(mp, mn);
    }

    D(bug("%s: start\n", __func__));

    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    WaitPort(mp);
    dp = (struct DosPacket *)GetMsg(mp)->mn_Node.ln_Name;

    D(bug("%s: mp=%p, path='%b'\n", __func__, mp, dp->dp_Arg1));

    cdfs = CDFS_Init(SysBase);
    if (cdfs == NULL) {
        retval = ERROR_NO_FREE_STORE;
        D(bug("%s: %b - error %d\n", __func__, dp->dp_Arg1, retval));
        reply(dp, DOSFALSE, retval);
        return RETURN_FAIL;
    }

    /* Open the device */
    retval = CDFS_DeviceOpen(cdfs, (struct FileSysStartupMsg *)BADDR(dp->dp_Arg2), &dev);
    if (retval != RETURN_OK) {
        D(bug("%s: Open %b - error %d\n", __func__, dp->dp_Arg1, retval));
        reply(dp, DOSFALSE, retval);
        return RETURN_FAIL;
    }

    /* Mark this as a persistent handler */
    ((struct DeviceNode *)BADDR(dp->dp_Arg3))->dn_Task = mp;

    D(bug("%s: Opened %b\n", __func__, dp->dp_Arg1));
    reply(dp, DOSTRUE, 0);

    sigpacket = 1 << mp->mp_SigBit;
    while (!die) {
        ULONG sigmask;

        sigmask = Wait(sigpacket);

        D(bug("%s: Signal 0x%08x\n", __func__, sigmask));

        if (!(sigmask & sigpacket))
            continue;

        while ((mn = GetMsg(mp)) != NULL) {
            SIPTR res2 = 0;
            SIPTR res  = DOSFALSE;

            dp = (struct DosPacket *)mn->mn_Node.ln_Name;
            D(bug("%s: Packet %d %p %p %p\n", __func__, dp->dp_Type, dp->dp_Arg1, dp->dp_Arg2, dp->dp_Arg3));

            switch (dp->dp_Type) {
            case ACTION_(DIE):
                CDFS_DeviceUnmount(cdfs, dev);
                res2 = 0;
                res = DOSTRUE;
                break;

            /* We don't permit actions that would write to the CDROM */
            case ACTION_(FORMAT):
            case ACTION_(FINDOUTPUT):
            case ACTION_(FINDUPDATE):
            case ACTION_(WRITE):
            case ACTION_(SET_FILE_SIZE):
            case ACTION_(RENAME_OBJECT):
            case ACTION_(RENAME_DISK):
            case ACTION_(CREATE_DIR):
            case ACTION_(DELETE_OBJECT):
            case ACTION_(SET_COMMENT):
            case ACTION_(SET_PROTECT):
            case ACTION_(SET_DATE):
            case ACTION_(INHIBIT):
            case ACTION_(SERIALIZE_DISK):
            case ACTION_(WRITE_PROTECT):
                res = DOSFALSE;
                res2 = ERROR_WRITE_PROTECTED;
                break;

            case ACTION_(COPY_DIR_FH):
                if ((BPTR)dp->dp_Arg1 == BNULL) {
                    res2 = ERROR_OBJECT_NOT_FOUND;
                    res = DOSFALSE;
                    break;
                }
                /* FALLTHROUGH */
            case ACTION_(COPY_DIR):
                {
                    struct CDFSLock *fl, *nl;
                    struct CDFSVolume *vol;

                    fl = B_LOCK(dp->dp_Arg1);

                    vol = CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2);
                    if (!vol) break;

                    res2 = vol->cv_Ops->op_Locate(vol, fl, "", MODE_OLDFILE, &nl);
                    if (res2 != RETURN_OK) {
                        res = DOSFALSE;
                        break;
                    }

                    nl->cl_FileLock.fl_Link = BNULL;
                    nl->cl_FileLock.fl_Access = ACCESS_READ;
                    nl->cl_FileLock.fl_Task = mp;
                    nl->cl_FileLock.fl_Volume = MKB_VOLUME(vol);
                    AddHead(&vol->cv_FileLocks, (struct Node *)&nl->cl_Node);
                    res = (SIPTR)MKB_LOCK(nl);
                    res2 = 0;
                }
                break;
            case ACTION_(DISK_INFO):
                CopyMem(&dev->cd_Volume->cv_InfoData, BADDR(dp->dp_Arg1), sizeof(struct InfoData));
                res = DOSTRUE;
                break;
            case ACTION_(FREE_LOCK):
            case ACTION_(END):
                {
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);

                    if (fl != NULL) {
                        struct CDFSVolume *vol = B_VOLUME(fl->cl_FileLock.fl_Volume);
                        Remove((struct Node *)&fl->cl_Node);
                        vol->cv_Ops->op_Close(vol, fl);
                    }
                    res = DOSTRUE;
                }
                break;
            case ACTION_(EXAMINE_FH):
#if 0
                if ((BPTR)dp->dp_Arg1 == BNULL) {
                    res2 = ERROR_OBJECT_NOT_FOUND;
                    res = DOSFALSE;
                    break;
                }
#endif
                /* FALLTHROUGH */
            case ACTION_(EXAMINE_OBJECT):
                {
                    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2);
                    if (!vol) break;

                    if (fib == NULL) {
                        res = DOSTRUE;
                        res2 = 0;
                        break;
                    }

                    D(bug("%s: Examine %p.%s\n", __func__, fl, &fl->cl_FileInfoBlock.fib_FileName[1]));
                    CopyMem(&fl->cl_FileInfoBlock, fib, sizeof(struct FileInfoBlock));
                    fib->fib_DiskKey = 0;

                    res = DOSTRUE;
                    res2 = 0;
                }
                break;
            case ACTION_(EXAMINE_NEXT):
                {
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);
                    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2);
                    if (!vol) break;

                    res2 = vol->cv_Ops->op_ExamineNext(vol, fl, fib);
                    res = res2 ? DOSFALSE : DOSTRUE;
                }
                break;
            case ACTION_(FH_FROM_LOCK):
                {
                    struct FileHandle *fh = BADDR(dp->dp_Arg1);
                    fh->fh_Arg1 = dp->dp_Arg2;
                }
                break;
            case ACTION_(FINDINPUT):
                {
                    struct FileHandle *fh = BADDR(dp->dp_Arg1);
                    struct CDFSLock   *dl = B_LOCK(dp->dp_Arg2);
                    CONST_STRPTR       fn = AROS_BSTR_ADDR(dp->dp_Arg3);
                    struct CDFSLock   *fl;
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &dl, &res, &res2);
                    if (!vol) break;

                    res2 = vol->cv_Ops->op_Locate(vol, dl, fn, MODE_OLDFILE, &fl);

                    if (res2 == RETURN_OK) {
                        LONG type = fl->cl_FileInfoBlock.fib_DirEntryType;
                        if (type == ST_USERDIR || type == ST_ROOT) {
                            res2 = ERROR_OBJECT_WRONG_TYPE;
                            res = DOSFALSE;
                        } else {
                            fl->cl_FileLock.fl_Link = BNULL;
                            fl->cl_FileLock.fl_Access = ACCESS_READ;
                            fl->cl_FileLock.fl_Task = mp;
                            fl->cl_FileLock.fl_Volume = MKB_VOLUME(vol);
                            AddHead(&vol->cv_FileLocks, (struct Node *)&fl->cl_Node);
                            fh->fh_Arg1 = (SIPTR)MKB_LOCK(fl);
                            res = DOSTRUE;
                        }
                    } else {
                        res = DOSFALSE;
                    }
                }
                break;
            case ACTION_(INFO):
                {
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);
                    if (CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2) == NULL)
                        break;

                    if (fl && fl->cl_FileInfoBlock.fib_DirEntryType == 0) {
                        res = DOSFALSE;
                        res2 = ERROR_OBJECT_NOT_FOUND;
                        break;
                    }

                    CopyMem(&dev->cd_Volume->cv_InfoData, BADDR(dp->dp_Arg2), sizeof(struct InfoData));
                    res = DOSTRUE;
                    res2 = 0;
                }
                break;
            case ACTION_(IS_FILESYSTEM):
                res = DOSTRUE;
                break;
            case ACTION_(LOCATE_OBJECT):
                {
                    struct CDFSLock *dl = B_LOCK(dp->dp_Arg1);
                    struct CDFSLock *fl;
                    struct CDFSVolume *vol;
                    CONST_STRPTR     fn = AROS_BSTR_ADDR(dp->dp_Arg2);
                    ULONG mode;

                    vol = CDFS_DevicePresent(cdfs, dev, &dl, &res, &res2);
                    if (!vol)
                        break;

                    mode = (dp->dp_Arg3 == ACCESS_READ) ? MODE_OLDFILE : MODE_READWRITE;

                    res2 = vol->cv_Ops->op_Locate(vol, dl, fn, mode, &fl);
                    if (res2 != 0) {
                        res = DOSFALSE;
                        break;
                    }

                    fl->cl_FileLock.fl_Link = BNULL;
                    fl->cl_FileLock.fl_Access = dp->dp_Arg3;
                    fl->cl_FileLock.fl_Task = mp;
                    fl->cl_FileLock.fl_Volume = MKB_VOLUME(vol);
                    AddHead(&vol->cv_FileLocks, (struct Node *)&fl->cl_Node);
                    res = (SIPTR)MKB_LOCK(fl);
                    res2 = 0;
                }
                break;
            case ACTION_(PARENT_FH):
                if ((BPTR)dp->dp_Arg1 == BNULL) {
                    res2 = ERROR_OBJECT_NOT_FOUND;
                    res = DOSFALSE;
                    break;
                }
                /* FALLTHROUGH */
            case ACTION_(PARENT):
                {
                    struct CDFSLock   *ol = B_LOCK(dp->dp_Arg1);
                    struct CDFSLock   *fl;
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &ol, &res, &res2);
                    if (!vol)
                        break;

                    if (ol->cl_FileInfoBlock.fib_DirEntryType == ST_ROOT) {
                        res2 = 0;
                        res = 0;
                        break;
                    }

                    res2 = vol->cv_Ops->op_Locate(vol, ol, "/", MODE_OLDFILE, &fl);
                    if (res2) {
                        res = DOSFALSE;
                        break;
                    }

                    fl->cl_FileLock.fl_Link = BNULL;
                    fl->cl_FileLock.fl_Access = ACCESS_READ;
                    fl->cl_FileLock.fl_Task = mp;
                    fl->cl_FileLock.fl_Volume = MKB_VOLUME(vol);
                    AddHead(&vol->cv_FileLocks, (struct Node *)&fl->cl_Node);
                    res = (SIPTR)MKB_LOCK(fl);
                    res2 = 0;
                }
                break;
            case ACTION_(READ):
                {
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2);
                    if (!vol) break;

                    res2 = vol->cv_Ops->op_Read(vol, fl, (APTR)dp->dp_Arg2, dp->dp_Arg3, &res);
                }
                break;
            /* ACTION_SAME_LOCK: Not needed, since fl_Key is unique per file */
            case ACTION_(SEEK):
                {
                    struct CDFSLock *fl = B_LOCK(dp->dp_Arg1);
                    struct CDFSVolume *vol;

                    vol = CDFS_DevicePresent(cdfs, dev, &fl, &res, &res2);
                    if (!vol) break;

                    res2 = vol->cv_Ops->op_Seek(vol, fl, dp->dp_Arg2, dp->dp_Arg3, &res);
                }
                break;
            default:
                res = DOSFALSE;
                res2 = ERROR_ACTION_NOT_KNOWN;
                D(bug("%s: Action %d not implemented\n", __func__, dp->dp_Type));
                break;
            }

            reply(dp, res, res2);
        }
    }

    CDFS_DeviceClose(cdfs, dev);

    /* Remove the 'CDFS' semaphore if we this is the last
     * handler, and there are no volumes nor devices left.
     */
    ObtainSemaphore(&cdfs->cb_Semaphore);
    if (IsListEmpty(&cdfs->cb_Devices) && IsListEmpty(&cdfs->cb_Volumes)) {
        RemSemaphore(&cdfs->cb_Semaphore);
        ReleaseSemaphore(&cdfs->cb_Semaphore);
        CloseLibrary(cdfs->cb_UtilityBase);
        FreeVec(cdfs);
    } else {
        ReleaseSemaphore(&cdfs->cb_Semaphore);
    }

    return RETURN_OK;
}


