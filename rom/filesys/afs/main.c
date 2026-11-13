/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/dos.h>
#include <proto/exec.h>

#include <aros/macros.h>
#include <aros/debug.h>

#include <string.h>

#include "afshandler.h"
#include "cache.h"
#include "error.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "filehandles3.h"
#include "misc.h"
#include "afs_security.h"
#include "volumes.h"

#include "baseredef.h"

static void replypkt(struct DosPacket *dp, SIPTR res1)
{
        struct MsgPort *mp;
        struct Message *mn;
        
        mp = dp->dp_Port;
        mn = dp->dp_Link;
        mn->mn_Node.ln_Name = (char*)dp;
        dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
        dp->dp_Res1 = res1;
        PutMsg(mp, mn);
}

static void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2)
{
        dp->dp_Res2 = res2;
        replypkt(dp, res1);
}

static struct AFSBase *AFS_alloc(void)
{
    struct AFSBase *handler;

    handler = AllocMem(sizeof(*handler), MEMF_ANY | MEMF_CLEAR);
    if (handler == NULL)
        return NULL;

    handler->utilitybase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (handler->utilitybase != NULL) {
        handler->dosbase = (struct DosLibrary *)TaggedOpenLibrary(TAGGEDOPEN_DOS);
        if (handler->dosbase != NULL) {

            /* changeint task and sigbit */
            handler->port.mp_SigTask = FindTask(0);
            handler->port.mp_SigBit = SIGBREAKB_CTRL_F;

            /* Port for device commands */
            handler->timer_mp = CreateMsgPort();
            /* Open timer */
            handler->timer_request = (struct timerequest *)
                CreateIORequest(handler->timer_mp, sizeof(struct timerequest));
            if (handler->timer_request != NULL) {
                if (OpenDevice("timer.device", UNIT_VBLANK,
                            (APTR)handler->timer_request, 0) == 0) {
                    return handler;
                }
                DeleteIORequest((struct IORequest *)handler->timer_request);
            }
            CloseLibrary((struct Library *)handler->dosbase);
        }
        CloseLibrary((struct Library *)handler->utilitybase);
    }
    return NULL;
}

static void AFS_free(struct AFSBase *handler)
{
    if (handler->timer_flags & TIMER_ACTIVE) {
        AbortIO((struct IORequest *)handler->timer_request);
        WaitIO((struct IORequest *)handler->timer_request);
    }
    CloseDevice((struct IORequest *)handler->timer_request);
    DeleteIORequest((struct IORequest *)handler->timer_request);
    CloseLibrary((struct Library *)handler->dosbase);
    CloseLibrary((struct Library *)handler->utilitybase);
    FreeMem(handler, sizeof(*handler));
}

static struct Volume *AFS_open_volume(struct AFSBase *handler, struct DosPacket *dp, LONG *error)
{
    struct FileSysStartupMsg *fssm = BADDR(dp->dp_Arg2);

    D(bug("AFS: Volume on Device %b, Unit %ld, Flags %ld, Environ %p\n",
                fssm->fssm_Device, fssm->fssm_Unit,
                fssm->fssm_Flags, BADDR(fssm->fssm_Environ)));
    return initVolume(handler,
            NULL, AROS_BSTR_ADDR(fssm->fssm_Device),
            fssm->fssm_Unit,
            fssm->fssm_Flags,
            BADDR(fssm->fssm_Environ),
            error);
}

static BOOL AFS_close_volume(struct AFSBase *handler, struct Volume *volume, SIPTR *io_DosError)
{
    if (!volume->locklist) {
        if (!flushForRelease(handler, volume)) {
            *io_DosError = ERROR_UNKNOWN;
            return FALSE;
        }
        uninitVolume(handler, volume);
        return TRUE;
    }

    *io_DosError = ERROR_OBJECT_IN_USE;
    return FALSE;
}

static BOOL AFS_protect_volume(struct AFSBase *handler, struct Volume *volume, BOOL on, ULONG key, SIPTR *io_DosError)
{
    if (!volume->locklist)
    {
        LONG error = writeprotectVolume(handler, volume, on, key);
        if (error == 0)
        {
            return TRUE;
        } else {
            *io_DosError = error;
            return FALSE;
        }
    }
    *io_DosError = ERROR_OBJECT_IN_USE;
    return FALSE;
}

static VOID startFlushTimer(struct AFSBase *handler)
{
    struct timerequest *request;

    /* Set up delay for next flush */
    if (handler->timer_flags & TIMER_ACTIVE) {
        handler->timer_flags |= TIMER_RESTART;
    } else {
        /* D(bug("[afs] Starting timer\n")); */
        request = handler->timer_request;
        request->tr_node.io_Command = TR_ADDREQUEST;
        request->tr_time.tv_secs = 1;
        request->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)handler->timer_request);
        handler->timer_flags = TIMER_ACTIVE;
    }
}

static VOID stopFlushMotor(struct AFSBase *handler, struct Volume *volume)
{
    if (volume->ioh.flags & IOHF_MOTOR_OFF) {
        motorOff(handler, &volume->ioh);
        volume->ioh.flags &= ~IOHF_MOTOR_OFF;
    }
}

static VOID onFlushTimer(struct AFSBase *handler, struct Volume *volume)
{
    handler->timer_flags &= ~TIMER_ACTIVE;
    if (volume->writefailed || volume->cachepending)
    {
        handler->timer_flags &= ~TIMER_RESTART;
        stopFlushMotor(handler, volume);
        return; /* Retry/Cancel already ran; only an explicit flush retries now. */
    }

    if (handler->timer_flags & TIMER_RESTART) {
        startFlushTimer(handler);
    } else {
        struct BlockCache *blockbuffer;

        /* D(bug("[afs] Alarm rang.\n")); */
        if (((volume->dostype == ID_DOS_DISK) || (volume->dostype == ID_DOS_muFS_DISK)) && mediumPresent(&volume->ioh))
        {
            /* Check if publishing the volume node needs to be retried */
            if ((volume->volumenode != NULL) && !volume->volumenodeadded)
                    addDosVolume(handler, volume);

            if (!flushCache(handler, volume))
            {
                    if (!volume->writefailed)
                            startFlushTimer(handler);
                    else
                            stopFlushMotor(handler, volume);
                    return;
            }
            blockbuffer = getBlock(handler, volume, volume->rootblock);
            if (blockbuffer == NULL)
            {
                    volume->writefailed = TRUE;
                    volume->state = ID_VALIDATING;
            }
            if (blockbuffer == NULL || ((blockbuffer->flags & BCF_WRITE) != 0
                    && !writeBlock(handler, volume, blockbuffer, -1)))
            {
                    if (!volume->writefailed)
                            startFlushTimer(handler);
                    else
                            stopFlushMotor(handler, volume);
                    return;
            }
            if (volume->ioh.flags & IOHF_MOTOR_OFF) {
                    D(bug("[afs 0x%08lX] turning off motor\n", volume));
                    motorOff(handler, &volume->ioh);
                    volume->ioh.flags &= ~IOHF_MOTOR_OFF;
            }
        }
    }
}

/*
 * ACTION_FLUSH: write every dirty block and the root, then ask the device
 * to commit its own cache, without detaching the volume. A failed device
 * commit is reported: a caller asking for a barrier must not be told the
 * data is durable when it is not.
 */
static LONG flushVolumeNow(struct AFSBase *handler, struct Volume *volume)
{
    return flush(handler, volume) ? 0 : ERROR_UNKNOWN;
}

/* The handle a lock argument refers to; a zero lock is the root. */
AFS_SHARED struct AfsHandle *lockhandle(struct Volume *volume, SIPTR lock)
{
    struct FileLock *fl = BADDR(lock);

    return fl ? (struct AfsHandle *)fl->fl_Key : &volume->ah;
}

/* Wrap a freshly opened handle into a FileLock; closes it again on failure. */
static SIPTR makelock(struct AFSBase *handler, struct Volume *volume,
    struct MsgPort *mp, struct AfsHandle *ah, LONG access, SIPTR *res2)
{
    struct FileLock *fl;

    if (ah == NULL)
        return DOSFALSE;
    fl = AllocMem(sizeof(*fl), MEMF_CLEAR);
    if (fl == NULL) {
        closef(handler, ah);
        *res2 = ERROR_NO_FREE_STORE;
        return DOSFALSE;
    }
    fl->fl_Link = BNULL;
    fl->fl_Key = (SIPTR)ah;
    fl->fl_Access = access;
    fl->fl_Task = mp;
    fl->fl_Volume = MKBADDR(&volume->devicelist);
    *res2 = 0;
    return (SIPTR)MKBADDR(fl);
}

static LONG gethandletype(struct AFSBase *handler,  struct AfsHandle  *h)
{
    UBYTE buffer[sizeof(struct ExAllData) + MAXFILENAMELENGTH];
    struct ExAllData *ead = (APTR)&buffer[0];
    ULONG size = sizeof(buffer);
    ULONG mode = ED_TYPE;
    IPTR dirpos;
    LONG res2;
    
    res2 = examine(handler, h, ead, size, mode, &dirpos);
    if (res2 == 0)
        return ead->ed_Type;
    return 0;
}

static CONST_STRPTR skipdevname(CONST_STRPTR fn)
{
    CONST_STRPTR cp;
    /* Skip past device names */
    for (cp = fn; *cp; cp++) {
        if (*cp == '/')
            break;
        if (*cp == ':') {
            fn = cp+1;
            break;
        }
    }
    return fn;
}

/*******************************************
 Name  : AFS_work
 Descr.: main loop (get packets and answer (or not))
 Input : proc - our process structure
 Output: -
********************************************/
LONG AFS_work(struct ExecBase *SysBase)
{
    struct MsgPort *mp;
    struct DosPacket *dp;
    struct Message *mn;
    struct AFSBase *handler;
    struct Volume *volume;
    LONG retval;
    BOOL dead = FALSE;
    BOOL nodisk;

    D(bug("[AFS] started\n"));
    mp = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    WaitPort(mp);
    dp = (struct DosPacket *)GetMsg(mp)->mn_Node.ln_Name;
    D(bug("[AFS] start message recevied. port=%p, path='%b'\n", mp, dp->dp_Arg1));

    handler = AFS_alloc();

    if (handler == NULL) {
        D(bug("[AFS] Can't allocate an instance of the handler for this volume\n"));
        replypkt(dp, DOSFALSE);
        return RETURN_FAIL;
    }

    NEWLIST(&handler->offlinevols);
    volume = AFS_open_volume(handler, dp, &retval);
    if (volume == NULL) {
        D(bug("[AFS] Can't open volume\n"));
        replypkt2(dp, DOSFALSE, retval);
        AFS_free(handler);
        return RETURN_FAIL;
    }

    /* make non-packet functions to see our volume */
    NEWLIST(&handler->device_list);
    AddHead(&handler->device_list, &volume->ln);

    /* Say that we are going to persist */
    ((struct DeviceNode *)BADDR(dp->dp_Arg3))->dn_Task = mp;

    replypkt(dp, DOSTRUE);

    while (!dead) {
        ULONG packetmask = 1L << mp->mp_SigBit;
        ULONG timermask = 1L << handler->timer_mp->mp_SigBit;
        ULONG changemask = 1L << SIGBREAKB_CTRL_F;
        ULONG sigs;

        sigs = Wait(packetmask | timermask | changemask);

        /* Process a media-change interrupt before touching cached disk data,
         * even when it woke the packet port rather than SIGBREAKB_CTRL_F. */
        checkDeviceFlags(handler);
        if (sigs & timermask)
            onFlushTimer(handler, volume);

        if (!(sigs & packetmask))
            continue;

        /* DOS Packet processing */

        while ((mn = GetMsg(mp)) != NULL) {
            SIPTR res2 = 0;
            SIPTR ok = DOSFALSE;

            dp = (struct DosPacket *)mn->mn_Node.ln_Name;

            D(bug("[AFS] packet %p:%d\n", dp, dp->dp_Type));
            if (volume->cachepending && dp->dp_Type != ACTION_END
                    && dp->dp_Type != ACTION_FREE_LOCK
                    && dp->dp_Type != ACTION_IS_FILESYSTEM
                    && dp->dp_Type != ACTION_IS_SECFS
                    && dp->dp_Type != ACTION_GET_SECFS_VERSION
                    && dp->dp_Type != ACTION_DISK_INFO) {
                replypkt2(dp, DOSFALSE, ERROR_NO_DISK);
                continue;
            }
            if (!volume->cachepending)
                startFlushTimer(handler);
            afsSecBeginPacket(handler, volume, dp);

            nodisk = !mediumPresent(&volume->ioh);

            switch (dp->dp_Type) {
            case ACTION_DIE:
                if (!AFS_close_volume(handler, volume, &res2)) {
                    ok = FALSE;
                    break;
                }
                dead = DOSTRUE;
                ok = TRUE;
                break;
            case ACTION_IS_FILESYSTEM:
                ok = TRUE;
                break;
            case ACTION_IS_SECFS:
                ok = afsSecIsActive(handler) ? DOSTRUE : DOSFALSE;
                break;
            case ACTION_GET_SECFS_VERSION:
                ok = secFS_API_VERSION;
                break;
            case ACTION_INHIBIT:
                res2 = inhibit(handler, volume, dp->dp_Arg1);
                goto res2_reply;
            case ACTION_END:
                closef(handler, (struct AfsHandle *)dp->dp_Arg1);
                ok = DOSTRUE;
                break;
            case ACTION_MORE_CACHE:
                {
                    LONG numbuff = dp->dp_Arg1;

                    if (numbuff) {
                        if (!flush(handler, volume)) {
                            res2 = ERROR_UNKNOWN;
                            break;
                        }
                        volume->numbuffers += numbuff;

                        if (volume->numbuffers < 1)
                            volume->numbuffers = 1;

                        Forbid();
                        freeCache(handler, volume->blockcache);
                        for (;;) {
                            volume->blockcache = initCache(handler, volume, volume->numbuffers);
                            if (volume->blockcache)
                                break;
                            volume->numbuffers /= 2;
                            if (volume->numbuffers < 1)
                                volume->numbuffers = 1;
                        }
                    }
                    ok = DOSTRUE;
                    res2 = volume->numbuffers;
                }
                break;
            case ACTION_DISK_INFO:
                ok = getDiskInfo(volume, BADDR(dp->dp_Arg1)) ? DOSFALSE : DOSTRUE;
                break;
            case ACTION_SAME_LOCK:
            {
                struct FileLock   *f1 = BADDR(dp->dp_Arg1);
                struct FileLock   *f2 = BADDR(dp->dp_Arg2);
                ok = (sameLock((struct AfsHandle*)f1->fl_Key, (struct AfsHandle*)f2->fl_Key) == LOCK_SAME) ? DOSTRUE : DOSFALSE;
                break;
            }
            case ACTION_FINDINPUT:
            case ACTION_FINDOUTPUT:
            case ACTION_FINDUPDATE:
            {
                struct FileHandle *fh = BADDR(dp->dp_Arg1);
                CONST_STRPTR       fn;
                struct AfsHandle  *dh;
                struct AfsHandle  *ah;

                if (nodisk)
                    goto nodisk_reply;

                dh = lockhandle(volume, dp->dp_Arg2);
                fn = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg3));

                /* multi-user: check access before touching anything */
                if (afsSecIsActive(handler)) {
                    LONG err;

                    if (dp->dp_Type == ACTION_FINDINPUT) {
                        err = afsSecCheckName(handler, dh, fn, AFS_ACCESS_READ);
                    } else if (dp->dp_Type == ACTION_FINDUPDATE) {
                        err = afsSecCheckName(handler, dh, fn, AFS_ACCESS_WRITE);
                        if (err == ERROR_OBJECT_NOT_FOUND)
                            err = afsSecCheckParent(handler, dh, fn, AFS_ACCESS_WRITE);
                    } else {
                        err = afsSecCheckName(handler, dh, fn, AFS_ACCESS_WRITE | AFS_ACCESS_DELETE);
                        if (err == ERROR_OBJECT_NOT_FOUND)
                            err = 0;
                        if (err == 0)
                            err = afsSecCheckParent(handler, dh, fn, AFS_ACCESS_WRITE);
                    }
                    if (err) {
                        res2 = err;
                        break;
                    }
                }

                /* MODE_* directly matches its corresponding ACTION_* counterpart. */
                if (dp->dp_Type == ACTION_FINDOUTPUT) {
                    ah = openfile(handler, dh, fn, dp->dp_Type, afsSecNewProtection(handler, 0), &res2);
                } else {
                    ah = openf(handler, dh, fn, dp->dp_Type, &res2);
                }
                if (ah != NULL) {
                    if (dp->dp_Type != ACTION_FINDOUTPUT) {
                        LONG type = gethandletype(handler, ah);
                        if (type >= 0) {
                            /* was directory */
                            res2 = ERROR_OBJECT_WRONG_TYPE;
                            closef(handler, ah);
                            break;
                        }
                    }
                    fh->fh_Arg1 = (SIPTR)ah;
                    ok = DOSTRUE;
                }
                break;
            }
            case ACTION_LOCATE_OBJECT:
            {
                CONST_STRPTR       fn;
                struct AfsHandle  *dh;
                ULONG mode = 0;

                if (nodisk)
                    goto nodisk_reply;

                dh = lockhandle(volume, dp->dp_Arg1);

                if (dp->dp_Arg3 == ACCESS_READ)
                    mode = MODE_OLDFILE;
                else if (dp->dp_Arg3 == ACCESS_WRITE)
                    mode = MODE_NEWFILE;

                fn = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg2));

                /* multi-user: locking needs read access to the object */
                if (afsSecIsActive(handler)) {
                    LONG err = afsSecCheckName(handler, dh, fn, AFS_ACCESS_READ);
                    if (err && err != ERROR_OBJECT_NOT_FOUND) {
                        res2 = err;
                        break;
                    }
                }

                ok = makelock(handler, volume, mp,
                    openf(handler, dh, fn, mode, &res2), dp->dp_Arg3, &res2);
                break;
            }
            case ACTION_COPY_DIR:   /* Aka DupLock() */
            case ACTION_COPY_DIR_FH:
            {
                struct AfsHandle  *oh;

                if (nodisk)
                    goto nodisk_reply;

                if (dp->dp_Type == ACTION_COPY_DIR)
                    oh = lockhandle(volume, dp->dp_Arg1);
                else
                    oh = (APTR)dp->dp_Arg1;

                ok = makelock(handler, volume, mp,
                    openf(handler, oh, "", MODE_OLDFILE, &res2), ACCESS_READ, &res2);
                break;
            }
            case ACTION_FREE_LOCK:
            {
                struct FileLock  *fl;

                if (!volume->cachepending && nodisk)
                    goto nodisk_reply;

                fl = BADDR(dp->dp_Arg1);
                if (fl != NULL) {
                    closef(handler, (APTR)(fl->fl_Key));
                    FreeMem(fl, sizeof(*fl));
                }
                ok = DOSTRUE;
                break;
            }
            case ACTION_READ:
                if (nodisk)
                    goto nodisk_reply;
                ok = readf(handler, (struct AfsHandle *)(dp->dp_Arg1), (APTR)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
                break;
            case ACTION_WRITE:
                if (nodisk)
                    goto nodisk_reply;
                ok = writef(handler, (struct AfsHandle *)(dp->dp_Arg1), (APTR)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
                break;
            case ACTION_SEEK:
                if (nodisk)
                    goto nodisk_reply;
                ok = seek(handler, (struct AfsHandle *)(dp->dp_Arg1), (LONG)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
                break;
            case ACTION_SET_FILE_SIZE:
                if (nodisk)
                    goto nodisk_reply;
                ok = setFileSize(handler, (struct AfsHandle *)(dp->dp_Arg1),(LONG)dp->dp_Arg2, (LONG)dp->dp_Arg3, &res2);
                break;
            case ACTION_FH_FROM_LOCK:
            {
                struct FileHandle    *fh = BADDR(dp->dp_Arg1);
                struct FileLock      *fl = BADDR(dp->dp_Arg2);
                fh->fh_Arg1 = (SIPTR)fl->fl_Key;
                FreeMem(fl, sizeof(*fl));
                ok = DOSTRUE;
                break;
            }
            case ACTION_EXAMINE_OBJECT:
            case ACTION_EXAMINE_FH:
            {
                struct AfsHandle     *ah;
                struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
                UBYTE buffer[sizeof(struct ExAllData) + MAXFILENAMELENGTH + MAXCOMMENTLENGTH];
                struct ExAllData     *ead = (APTR)&buffer[0];
                SIPTR dirpos;

                if (nodisk)
                    goto nodisk_reply;

                if (dp->dp_Type == ACTION_EXAMINE_OBJECT)
                    ah = lockhandle(volume, dp->dp_Arg1);
                else
                    ah = (APTR)dp->dp_Arg1;

                if (fib == NULL) {
                    ok = DOSTRUE;
                    break;
                }

                res2 = examine(handler, ah, ead, sizeof(buffer), ED_OWNER, &dirpos);
                if (res2 != 0)
                    break;

                if (ah == &volume->ah) {
                    fib->fib_DirEntryType = ST_ROOT;
                } else {
                    fib->fib_DirEntryType = ead->ed_Type;
                }

                fib->fib_DiskKey = ah->header_block;
                strncpy(&fib->fib_FileName[1], ead->ed_Name, sizeof(fib->fib_FileName)-1);
                fib->fib_FileName[0] = strlen(&fib->fib_FileName[1]);
                fib->fib_Protection = ead->ed_Prot;
                fib->fib_EntryType = fib->fib_DirEntryType;
                fib->fib_Size = ead->ed_Size;
                fib->fib_NumBlocks = (ead->ed_Size + ah->volume->sectorsize - 1) / ah->volume->sectorsize;
                fib->fib_Date.ds_Days = ead->ed_Days;
                fib->fib_Date.ds_Minute = ead->ed_Mins;
                fib->fib_Date.ds_Tick = ead->ed_Ticks;
                if (ead->ed_Comment) {
                    strncpy(&fib->fib_Comment[1], ead->ed_Comment, sizeof(fib->fib_Comment)-1);
                    fib->fib_Comment[0] = strlen(&fib->fib_Comment[1]);
                } else {
                    fib->fib_Comment[0] = 0;
                }
                fib->fib_OwnerUID = ead->ed_OwnerUID;
                fib->fib_OwnerGID = ead->ed_OwnerGID;
                ok = DOSTRUE;
                break;
            }
            case ACTION_EXAMINE_NEXT:
            {
                struct AfsHandle     *ah;
                struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);

                if (nodisk)
                    goto nodisk_reply;

                if (fib == NULL) {
                    ok = DOSTRUE;
                    break;
                }

                ah = lockhandle(volume, dp->dp_Arg1);

                res2 = examineNext(handler, ah, fib);
                if (res2 != 0)
                    break;

                if (ah == &volume->ah) {
                    fib->fib_DirEntryType = ST_ROOT;
                }
                ok = DOSTRUE;
                break;
            }
            case ACTION_PARENT:
            case ACTION_PARENT_FH:
            {
                struct AfsHandle  *oh;
                struct AfsHandle  *ah;

                if (nodisk)
                    goto nodisk_reply;

                if (dp->dp_Type == ACTION_PARENT)
                    oh = (APTR)((struct FileLock *)BADDR(dp->dp_Arg1))->fl_Key;
                else
                    oh = (APTR)dp->dp_Arg1;
                ah = openf(handler, oh, "/", MODE_OLDFILE, &res2);
                if (ah == NULL) {
                    if (res2 == ERROR_OBJECT_NOT_FOUND && gethandletype(handler, oh) == ST_ROOT)
                        res2 = 0; /* need to return res2 = 0 (not 205) if oh == ST_ROOT */
                    break;
                }
                ok = makelock(handler, volume, mp, ah, ACCESS_READ, &res2);
                break;
            }
            case ACTION_INFO:
                /* reads the header as before; its result never mattered */
                gethandletype(handler, lockhandle(volume, dp->dp_Arg1));
                res2 = getDiskInfo(volume, BADDR(dp->dp_Arg2));
                goto res2_reply;
            case ACTION_RENAME_OBJECT:
            {
                struct AfsHandle  *oh = lockhandle(volume, dp->dp_Arg1);
                CONST_STRPTR       on = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg2));
                CONST_STRPTR       nn = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg4));

                /* multi-user: property access to the object, write access to both directories */
                if ((res2 = afsSecCheckNameProperty(handler, oh, on)) == 0 &&
                    (res2 = afsSecCheckParent(handler, oh, on, AFS_ACCESS_WRITE)) == 0 &&
                    (res2 = afsSecCheckParent(handler, oh, nn, AFS_ACCESS_WRITE)) == 0)
                    res2 = renameObject(handler, oh, on, nn);
                goto res2_reply;
            }
            case ACTION_RENAME_DISK:
                ok = relabel(handler, volume, AROS_BSTR_ADDR(dp->dp_Arg1), &res2);
                break;
            case ACTION_CREATE_DIR:
            {
                struct FileLock   *flnew;
                struct AfsHandle  *h = lockhandle(volume, dp->dp_Arg1), *ah;
                CONST_STRPTR       n = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg2));

                /* multi-user: write access to the parent directory */
                if ((res2 = afsSecCheckParent(handler, h, n, AFS_ACCESS_WRITE)))
                    break;
                flnew = AllocMem(sizeof(*flnew), MEMF_CLEAR);
                if (flnew != NULL) {
                    ah = createDir(handler, h, n, afsSecNewProtection(handler, 0), &res2);
                    if (res2 == 0) {
                        flnew->fl_Link = BNULL;
                        flnew->fl_Key = (SIPTR)ah;
                        flnew->fl_Access = ACCESS_READ;
                        flnew->fl_Task = mp;
                        flnew->fl_Volume = MKBADDR(&volume->devicelist);
                        ok = (SIPTR)MKBADDR(flnew);
                    }
                } else {
                    res2 = ERROR_NO_FREE_STORE;
                }
                break;
            }
            case ACTION_DELETE_OBJECT:
            {
                struct AfsHandle  *h = lockhandle(volume, dp->dp_Arg1);
                CONST_STRPTR       n = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg2));

                /* multi-user: delete access to the object, write access to its directory */
                if ((res2 = afsSecCheckName(handler, h, n, AFS_ACCESS_DELETE)) == 0 &&
                    (res2 = afsSecCheckParent(handler, h, n, AFS_ACCESS_WRITE)) == 0)
                    res2 = deleteObject(handler, h, n);
                goto res2_reply;
            }
            case ACTION_SET_COMMENT:
            case ACTION_SET_PROTECT:
            case ACTION_SET_DATE:
            {
                struct AfsHandle  *h = lockhandle(volume, dp->dp_Arg2);
                CONST_STRPTR       n = skipdevname(AROS_BSTR_ADDR(dp->dp_Arg3));

                if ((res2 = afsSecCheckNameProperty(handler, h, n)) == 0) {
                    if (dp->dp_Type == ACTION_SET_COMMENT)
                        res2 = setComment(handler, h, n, AROS_BSTR_ADDR(dp->dp_Arg4));
                    else if (dp->dp_Type == ACTION_SET_PROTECT)
                        res2 = setProtect(handler, h, n, dp->dp_Arg4);
                    else
                        res2 = setDate(handler, h, n, (struct DateStamp *)dp->dp_Arg4);
                }
                goto res2_reply;
            }
            case ACTION_SET_OWNER:
                if (nodisk)
                    goto nodisk_reply;
                res2 = afsSecSetOwner(handler, lockhandle(volume, dp->dp_Arg2),
                    skipdevname(AROS_BSTR_ADDR(dp->dp_Arg3)), (ULONG)dp->dp_Arg4);
                goto res2_reply;
            case ACTION_FLUSH:
                if (volume->inhibitcounter != 0 || nodisk)
                    goto nodisk_reply;
                res2 = flushVolumeNow(handler, volume);
                goto res2_reply;
            case ACTION_FORMAT:
                res2 = format(handler, volume, AROS_BSTR_ADDR(dp->dp_Arg1), dp->dp_Arg2);
                goto res2_reply;
            case ACTION_SERIALIZE_DISK:
                ok = relabel(handler, volume, NULL, &res2);
                break;
            case ACTION_WRITE_PROTECT:
                ok = AFS_protect_volume(handler, volume, dp->dp_Arg1, dp->dp_Arg2, &res2) ? DOSFALSE : DOSTRUE;
                break;
            default:
                res2 = ERROR_ACTION_NOT_KNOWN;
                D(bug("[AFS] %d not implemented\n", dp->dp_Type));
                break;
            }
            goto reply;

nodisk_reply:
            res2 = ERROR_NO_DISK;
            goto reply;
res2_reply:
            ok = res2 ? DOSFALSE : DOSTRUE;
reply:
            afsSecEndPacket(handler);
            replypkt2(dp, ok, res2);
        }
    }

    AFS_free(handler);
    replypkt(dp, DOSTRUE);

    return RETURN_OK;
}

