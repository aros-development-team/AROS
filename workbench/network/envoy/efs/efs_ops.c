/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFS client - DOS packets to EFS packets (re/spec/efs-protocol.md
          §2, §7). Locks and file handles are our own records that carry the
          server's 32-bit handles and the path needed to re-establish them.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "efs_intern.h"
#include <envoy/accounts.h>
#include <proto/nipc.h>

#define FAIL(dp, e)     do { (dp)->dp_Res1 = DOSFALSE; (dp)->dp_Res2 = (e); } while (0)
#define OK(dp)          do { (dp)->dp_Res1 = DOSTRUE; (dp)->dp_Res2 = 0; } while (0)

void EfsReplyPkt(struct Globals *glob, struct DosPacket *dp)
{
    struct MsgPort *rp = dp->dp_Port;
    struct Message *mn = dp->dp_Link;

    dp->dp_Port = glob->Port;
    PutMsg(rp, mn);
}

/* ---- locks and files ------------------------------------------------------ */

struct EfsLock *LockFromBPTR(struct Globals *glob, BPTR b)
{
    struct EfsLock *l;
    struct FileLock *want = (struct FileLock *)BADDR(b);

    if (!b)
        return NULL;
    ForeachNode(&glob->Locks, l)
        if (&l->FL == want)
            return l;
    return NULL;
}

static struct EfsVolume *VolumeFor(struct Globals *glob, ULONG volid)
{
    struct EfsVolume *v;
    ForeachNode(&glob->Volumes, v)
        if (v->ID == volid)
            return v;
    return glob->CurVol;
}

struct EfsLock *NewLock(struct Globals *glob, ULONG handle, ULONG volid, LONG mode, CONST_STRPTR path)
{
    struct EfsLock *l;

    if (!(l = AllocVec(sizeof(struct EfsLock), MEMF_PUBLIC | MEMF_CLEAR)))
        return NULL;
    l->Handle = handle;
    l->VolID = volid;
    l->Gen = glob->Gen;
    l->Mode = mode;
    l->Vol = VolumeFor(glob, volid);
    l->ScanNext = 0xFFFFFFFFUL;
    CopyStr(l->Path, sizeof(l->Path), path);
    l->FL.fl_Key = (IPTR)l;
    l->FL.fl_Access = mode;
    l->FL.fl_Task = glob->Port;
    l->FL.fl_Volume = MKBADDR((l->Vol ? l->Vol->DosList : NULL));
    AddTail((struct List *)&glob->Locks, (struct Node *)&l->Node);
    return l;
}

void ResetScan(struct Globals *glob, struct EfsLock *l)
{
    if (l->Scan)
        FreeVec(l->Scan);
    l->Scan = NULL;
    l->ScanEntries = 0;
    l->ScanNext = 0xFFFFFFFFUL;
    l->ScanMore = FALSE;
    l->ScanStarted = FALSE;
}

void FreeLockRec(struct Globals *glob, struct EfsLock *l)
{
    Remove((struct Node *)&l->Node);
    ResetScan(glob, l);
    FreeVec(l);
}

/* lock argument -> server handle (0 = export root); FALSE with *err on a bad or stale lock */
static BOOL LockHandle(struct Globals *glob, BPTR b, ULONG *handle, LONG *err)
{
    struct EfsLock *l;

    if (!b)
    {
        *handle = 0;
        return TRUE;
    }
    if (!(l = LockFromBPTR(glob, b)))
    {
        *err = ERROR_INVALID_LOCK;
        return FALSE;
    }
    if (l->Gen != glob->Gen && !RefreshLock(glob, l))
    {
        *err = ERROR_INVALID_LOCK;
        return FALSE;
    }
    *handle = l->Handle;
    return TRUE;
}

struct EfsFile *FileFromArg(struct Globals *glob, SIPTR arg)
{
    struct EfsFile *f, *want = (struct EfsFile *)arg;

    if (!arg)
        return NULL;
    ForeachNode(&glob->Files, f)
        if (f == want)
            return f;
    return NULL;
}

static BOOL FileHandle(struct Globals *glob, SIPTR arg, struct EfsFile **fp, LONG *err)
{
    struct EfsFile *f = FileFromArg(glob, arg);

    if (!f || (f->Gen != glob->Gen && !RefreshFile(glob, f)))
    {
        *err = ERROR_OBJECT_NOT_FOUND;
        return FALSE;
    }
    *fp = f;
    return TRUE;
}

void FreeFileRec(struct Globals *glob, struct EfsFile *f)
{
    struct EfsRecord *r;

    Remove((struct Node *)&f->Node);
    while ((r = (struct EfsRecord *)RemHead((struct List *)&f->Records)))
        FreeVec(r);
    FreeVec(f);
}

/* the name argument of a packet, relative to its lock */
static void NameArg(struct Globals *glob, BSTR b, char *dst, ULONG size)
{
    char tmp[EFS_MAXPATH];
    GetBstrArg(glob, b, tmp, sizeof(tmp));
    CopyStr(dst, size, StripDevice(tmp));
}

/* the response of a lock-creating action (LOCATE, CREATE_DIR, COPY_DIR, PARENT) */
static void LockResult(struct Globals *glob, struct DosPacket *dp, LONG res1, LONG res2, LONG mode)
{
    struct EfsLock *l;
    ULONG actual = glob->Trans->trans_RespDataActual;
    char path[EFS_MAXPATH];

    if (res1 == 0)
    {
        dp->dp_Res1 = 0;
        dp->dp_Res2 = res2;
        return;
    }
    if (actual <= EFS_HDR)
        path[0] = '\0';
    else
    {
        ULONG n = strnlen((CONST_STRPTR)glob->Buf + EFS_HDR, actual - EFS_HDR);
        if (n > sizeof(path) - 1) n = sizeof(path) - 1;
        memcpy(path, glob->Buf + EFS_HDR, n);
        path[n] = '\0';
    }
    if (!path[0])
    {
        /* no path, nothing to replay with: the original fails here too */
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    if (!(l = NewLock(glob, res1, res2, mode, path)))
    {
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    dp->dp_Res1 = (SIPTR)MKBADDR(&l->FL);
    dp->dp_Res2 = 0;
}

/* a temporary lock on the export root for scans on a zero lock */
static struct EfsLock *RootScanLock(struct Globals *glob)
{
    LONG res1, res2;
    ULONG end;
    struct EfsLock *l;

    ForeachNode(&glob->Locks, l)
        if (l->FL.fl_Key == 0)              /* our marker: not handed out */
            return l;
    HdrSet(glob, glob->Buf, ACTION_LOCATE_OBJECT, ++glob->NextCookie, 0, EFS_HDR, (ULONG)SHARED_LOCK, glob->Flags & 0xFFFF, 1, 0);
    end = PutBName(glob->Buf, EFS_HDR, "");
    if (!Forward(glob, ACTION_LOCATE_OBJECT, end, 1, &res1, &res2) || !res1)
        return NULL;
    if (!(l = NewLock(glob, res1, res2, SHARED_LOCK, (CONST_STRPTR)glob->Buf + EFS_HDR)))
        return NULL;
    l->FL.fl_Key = 0;
    return l;
}

/* ---- the actions ---------------------------------------------------------- */

static void ActLocate(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    ULONG parent, end;
    LONG err, res1, res2, mode = (action == ACTION_CREATE_DIR) ? SHARED_LOCK : (LONG)dp->dp_Arg3;
    char name[EFS_MAXPATH];

    if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &parent, &err))
    {
        FAIL(dp, err);
        return;
    }
    NameArg(glob, (BSTR)dp->dp_Arg2, name, sizeof(name));
    HdrSet(glob, glob->Buf, action, ++glob->NextCookie, parent, EFS_HDR, (ULONG)mode, glob->Flags & 0xFFFF, 0, 0);
    end = PutBName(glob->Buf, EFS_HDR, name);
    if (!Forward(glob, action, end, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    LockResult(glob, dp, res1, res2, mode);
}

/* COPY_DIR, PARENT (lock) and COPY_DIR_FH, PARENT_FH (file) */
static void ActDupParent(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    ULONG handle;
    LONG err, res1, res2, mode = SHARED_LOCK;

    if (action == ACTION_COPY_DIR_FH || action == ACTION_PARENT_FH)
    {
        struct EfsFile *f;
        if (!FileHandle(glob, dp->dp_Arg1, &f, &err))
        {
            FAIL(dp, err);
            return;
        }
        handle = f->Handle;
    }
    else
    {
        struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg1);
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &handle, &err))
        {
            FAIL(dp, err);
            return;
        }
        if (l && action == ACTION_COPY_DIR)
            mode = l->Mode;
        if (action == ACTION_PARENT && !handle)
        {
            dp->dp_Res1 = 0;                /* parent of the root */
            dp->dp_Res2 = 0;
            return;
        }
    }
    HdrSet(glob, glob->Buf, action, ++glob->NextCookie, handle, 0, 0, 0, 0, 0);
    if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    LockResult(glob, dp, res1, res2, mode);
}

static void ActFreeLock(struct Globals *glob, struct DosPacket *dp)
{
    struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg1);
    LONG res1, res2;

    if (!l)
    {
        OK(dp);
        return;
    }
    if (l->Gen == glob->Gen && l->Handle && glob->Connected)
    {
        HdrSet(glob, glob->Buf, ACTION_FREE_LOCK, ++glob->NextCookie, l->Handle, 0, 0, 0, 0, 0);
        Forward(glob, ACTION_FREE_LOCK, EFS_BUFSIZE, 1, &res1, &res2);
        glob->Retry = FALSE;                /* the lock goes whatever happened */
    }
    FreeLockRec(glob, l);
    OK(dp);
}

static void ActSameLock(struct Globals *glob, struct DosPacket *dp)
{
    ULONG h1, h2;
    LONG err, res1, res2;

    if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &h1, &err) || !LockHandle(glob, (BPTR)dp->dp_Arg2, &h2, &err))
    {
        FAIL(dp, err);
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_SAME_LOCK, ++glob->NextCookie, h1, h2, 0, 0, 0, 0);
    if (!Forward(glob, ACTION_SAME_LOCK, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
}

static void FileResult(struct Globals *glob, struct DosPacket *dp, struct FileHandle *fh, LONG res1, LONG res2, ULONG action)
{
    struct EfsFile *f;
    ULONG actual = glob->Trans->trans_RespDataActual;

    if (res1 == 0)
    {
        FAIL(dp, res2);
        return;
    }
    if (actual <= EFS_HDR || !glob->Buf[EFS_HDR])
    {
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    if (!(f = AllocVec(sizeof(struct EfsFile), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    f->Handle = res1;
    f->VolID = res2;
    f->Gen = glob->Gen;
    f->OpenAction = action;
    f->Vol = VolumeFor(glob, res2);
    NEWLIST((struct List *)&f->Records);
    {
        ULONG n = strnlen((CONST_STRPTR)glob->Buf + EFS_HDR, actual - EFS_HDR);
        if (n > sizeof(f->Path) - 1) n = sizeof(f->Path) - 1;
        memcpy(f->Path, glob->Buf + EFS_HDR, n);
        f->Path[n] = '\0';
    }
    AddTail((struct List *)&glob->Files, (struct Node *)&f->Node);
    fh->fh_Arg1 = (SIPTR)f;
    fh->fh_Port = 0;
    OK(dp);
}

static void ActOpen(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    struct FileHandle *fh = BADDR(dp->dp_Arg1);
    ULONG dir;
    LONG err, res1, res2;
    char name[EFS_MAXPATH];

    if (action != ACTION_FINDINPUT && glob->WriteProtect)
    {
        FAIL(dp, ERROR_DISK_WRITE_PROTECTED);
        return;
    }
    if (!LockHandle(glob, (BPTR)dp->dp_Arg2, &dir, &err))
    {
        FAIL(dp, err);
        return;
    }
    NameArg(glob, (BSTR)dp->dp_Arg3, name, sizeof(name));
    HdrSet(glob, glob->Buf, action, ++glob->NextCookie, 0, dir, EFS_HDR, glob->Flags & 0xFFFF, 0, 0);
    PutBName(glob->Buf, EFS_HDR, name);
    if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    FileResult(glob, dp, fh, res1, res2, action);
}

static void ActFhFromLock(struct Globals *glob, struct DosPacket *dp)
{
    struct FileHandle *fh = BADDR(dp->dp_Arg1);
    struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg2);
    ULONG handle;
    LONG err, res1, res2;

    if (!l || !LockHandle(glob, (BPTR)dp->dp_Arg2, &handle, &err) || !handle)
    {
        FAIL(dp, ERROR_INVALID_LOCK);
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_FH_FROM_LOCK, ++glob->NextCookie, 0, handle, 0, 0, 0, 0);
    if (!Forward(glob, ACTION_FH_FROM_LOCK, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    FileResult(glob, dp, fh, res1, res2, ACTION_FINDUPDATE);
    if (dp->dp_Res1)
        FreeLockRec(glob, l);              /* the server consumed the lock */
}

static void ActEnd(struct Globals *glob, struct DosPacket *dp)
{
    struct EfsFile *f = FileFromArg(glob, dp->dp_Arg1);
    LONG res1, res2;

    if (!f)
    {
        OK(dp);
        return;
    }
    if (f->Gen == glob->Gen && glob->Connected)
    {
        HdrSet(glob, glob->Buf, ACTION_END, ++glob->NextCookie, f->Handle, 0, 0, 0, 0, 0);
        Forward(glob, ACTION_END, EFS_BUFSIZE, 1, &res1, &res2);
        glob->Retry = FALSE;
    }
    FreeFileRec(glob, f);
    OK(dp);
}

static void ActSeek(struct Globals *glob, struct DosPacket *dp)
{
    struct EfsFile *f;
    LONG err, res1, res2, off = (LONG)dp->dp_Arg2, mode = (LONG)dp->dp_Arg3;

    if (!FileHandle(glob, dp->dp_Arg1, &f, &err))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = err;
        return;
    }
    if (mode == OFFSET_CURRENT && off == 0)
    {
        dp->dp_Res1 = f->Pos;
        dp->dp_Res2 = 0;
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_SEEK, ++glob->NextCookie, f->Handle, (ULONG)off, (ULONG)mode, 0, 0, 0);
    if (!Forward(glob, ACTION_SEEK, EFS_BUFSIZE, 1, &res1, &res2))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = res2;
        return;
    }
    if (res1 == -1)
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = res2;
        return;
    }
    if (mode == OFFSET_BEGINNING)
        f->Pos = off;
    else if (mode == OFFSET_CURRENT)
        f->Pos += off;
    else
    {
        LONG r1, r2;
        HdrSet(glob, glob->Buf, ACTION_SEEK, ++glob->NextCookie, f->Handle, 0, 0, 1, 0, 0);
        if (Forward(glob, ACTION_SEEK, EFS_BUFSIZE, 1, &r1, &r2) && r1 != -1)
            f->Pos = r1;
        glob->Retry = FALSE;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = 0;
}

static void ActSetFileSize(struct Globals *glob, struct DosPacket *dp)
{
    struct EfsFile *f;
    LONG err, res1, res2;

    if (glob->WriteProtect)
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = ERROR_DISK_WRITE_PROTECTED;
        return;
    }
    if (!FileHandle(glob, dp->dp_Arg1, &f, &err))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = err;
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_SET_FILE_SIZE, ++glob->NextCookie, f->Handle, (ULONG)dp->dp_Arg2, (ULONG)dp->dp_Arg3, 0, 0, 0);
    if (!Forward(glob, ACTION_SET_FILE_SIZE, EFS_BUFSIZE, 1, &res1, &res2))
    {
        dp->dp_Res1 = -1;
        dp->dp_Res2 = res2;
        return;
    }
    if (res1 != -1 && f->Pos > res1)
        f->Pos = res1;
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
}

static void ActChangeMode(struct Globals *glob, struct DosPacket *dp)
{
    ULONG type = (ULONG)dp->dp_Arg1, handle;
    LONG err, res1, res2;

    if (type == CHANGE_LOCK)
    {
        if (!LockHandle(glob, (BPTR)dp->dp_Arg2, &handle, &err))
        {
            FAIL(dp, err);
            return;
        }
    }
    else if (type == CHANGE_FH)
    {
        struct EfsFile *f;
        if (!FileHandle(glob, dp->dp_Arg2, &f, &err))
        {
            FAIL(dp, err);
            return;
        }
        handle = f->Handle;
    }
    else
    {
        FAIL(dp, ERROR_OBJECT_WRONG_TYPE);
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_CHANGE_MODE, ++glob->NextCookie, type == CHANGE_LOCK ? 0 : 1, handle, (ULONG)dp->dp_Arg3, 0, 0, 0);
    if (!Forward(glob, ACTION_CHANGE_MODE, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
    if (res1 && type == CHANGE_LOCK)
    {
        struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg2);
        if (l)
            l->Mode = l->FL.fl_Access = (LONG)dp->dp_Arg3;
    }
}

/* DELETE (as 5679), RENAME, SET_PROTECT, SET_COMMENT, SET_DATE, SET_OWNER, MAKE_LINK */
static void ActNameOp(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    ULONG h1 = 0, h2 = 0, end, off2, wire = action, tmult = 1;
    LONG err, res1, res2;
    char name[EFS_MAXPATH], name2[EFS_MAXPATH];

    if (glob->WriteProtect)
    {
        FAIL(dp, ERROR_DISK_WRITE_PROTECTED);
        return;
    }
    switch (action)
    {
    case ACTION_DELETE_OBJECT:
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &h1, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg2, name, sizeof(name));
        HdrSet(glob, glob->Buf, EFSACT_DELETE, ++glob->NextCookie, h1, EFS_HDR, 0, 0, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        wire = EFSACT_DELETE;
        tmult = 4;
        break;

    case ACTION_RENAME_OBJECT:
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &h1, &err) || !LockHandle(glob, (BPTR)dp->dp_Arg3, &h2, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg2, name, sizeof(name));
        NameArg(glob, (BSTR)dp->dp_Arg4, name2, sizeof(name2));
        end = PutBName(glob->Buf, EFS_HDR, name);
        off2 = Align4(end);
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, h1, EFS_HDR, h2, off2, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        PutBName(glob->Buf, off2, name2);
        break;

    case ACTION_SET_PROTECT:
    case ACTION_SET_OWNER:
        if (!LockHandle(glob, (BPTR)dp->dp_Arg2, &h1, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg3, name, sizeof(name));
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, 0, h1, EFS_HDR, (ULONG)dp->dp_Arg4, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        break;

    case ACTION_SET_COMMENT:
        if (!LockHandle(glob, (BPTR)dp->dp_Arg2, &h1, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg3, name, sizeof(name));
        GetBstrArg(glob, (BSTR)dp->dp_Arg4, name2, sizeof(name2));
        if (strlen(name2) > 79)
            name2[79] = '\0';
        end = PutBName(glob->Buf, EFS_HDR, name);
        off2 = Align4(end);
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, 0, h1, EFS_HDR, off2, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        PutBName(glob->Buf, off2, name2);
        break;

    case ACTION_SET_DATE:
    {
        struct DateStamp *ds = (struct DateStamp *)dp->dp_Arg4;
        if (!LockHandle(glob, (BPTR)dp->dp_Arg2, &h1, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg3, name, sizeof(name));
        end = PutBName(glob->Buf, EFS_HDR, name);
        off2 = Align4(end);
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, 0, h1, EFS_HDR, off2, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        PutL(glob->Buf, off2, ds ? ds->ds_Days : 0);
        PutL(glob->Buf, off2 + 4, ds ? ds->ds_Minute : 0);
        PutL(glob->Buf, off2 + 8, ds ? ds->ds_Tick : 0);
        break;
    }

    case ACTION_MAKE_LINK:
        if ((LONG)dp->dp_Arg4 != LINK_HARD)
        {
            FAIL(dp, ERROR_OBJECT_WRONG_TYPE);
            return;
        }
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &h1, &err) || !LockHandle(glob, (BPTR)dp->dp_Arg3, &h2, &err)) { FAIL(dp, err); return; }
        NameArg(glob, (BSTR)dp->dp_Arg2, name, sizeof(name));
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, h1, EFS_HDR, h2, 0, 0, 0);
        PutBName(glob->Buf, EFS_HDR, name);
        break;

    default:
        FAIL(dp, ERROR_ACTION_NOT_KNOWN);
        return;
    }
    if (!Forward(glob, wire, EFS_BUFSIZE, tmult, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
}

static void ActExamine(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
    ULONG handle;
    LONG err, res1, res2;

    if (action == ACTION_EXAMINE_FH)
    {
        struct EfsFile *f;
        if (!FileHandle(glob, dp->dp_Arg1, &f, &err)) { FAIL(dp, err); return; }
        handle = f->Handle;
    }
    else
    {
        struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg1);
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &handle, &err)) { FAIL(dp, err); return; }
        if (l)
            ResetScan(glob, l);             /* an Examine restarts the directory scan */
    }
    if (!fib)
    {
        FAIL(dp, ERROR_REQUIRED_ARG_MISSING);
        return;
    }
    HdrSet(glob, glob->Buf, action, ++glob->NextCookie, handle, EFS_HDR, 0, glob->Flags & 0xFFFF, 0, 0);
    memset(glob->Buf + EFS_HDR, 0, EFS_FIBSIZE);
    if (!Forward(glob, action, EFS_HDR + EFS_FIBSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    if (res1 && glob->Trans->trans_RespDataActual >= EFS_HDR + EFS_FIBSIZE)
    {
        WireToFib(glob, glob->Buf + EFS_HDR, fib);
        if (action == ACTION_EXAMINE_OBJECT && !handle && glob->CurVol)
            SetBString(fib->fib_FileName, sizeof(fib->fib_FileName), (const UBYTE *)glob->CurVol->Name, strlen(glob->CurVol->Name));
    }
    dp->dp_Res1 = res1 ? DOSTRUE : DOSFALSE;
    dp->dp_Res2 = res1 ? 0 : res2;
}

/* ExNext from action-5678 blocks (§2.6) */
static void ActExNext(struct Globals *glob, struct DosPacket *dp)
{
    struct FileInfoBlock *fib = BADDR(dp->dp_Arg2);
    struct EfsLock *l;
    LONG res1, res2;
    int rounds = 0;

    if (!dp->dp_Arg1)
        l = RootScanLock(glob);
    else
        l = LockFromBPTR(glob, (BPTR)dp->dp_Arg1);
    if (!l || !fib)
    {
        FAIL(dp, l ? ERROR_REQUIRED_ARG_MISSING : ERROR_INVALID_LOCK);
        return;
    }
    if (l->Gen != glob->Gen && !RefreshLock(glob, l))
    {
        FAIL(dp, ERROR_INVALID_LOCK);
        return;
    }
    while (l->ScanNext == 0xFFFFFFFFUL)
    {
        ULONG entries;
        if ((l->ScanStarted && !l->ScanMore) || rounds++ > 8)
        {
            FAIL(dp, ERROR_NO_MORE_ENTRIES);
            ResetScan(glob, l);
            return;
        }
        HdrSet(glob, glob->Buf, EFSACT_EXNEXT, ++glob->NextCookie, l->Handle, EFS_HDR, EFS_SCANBLOCK, ED_OWNER, glob->Flags & 0xFFFF, 0);
        memset(glob->Buf + EFS_HDR, 0, EFS_SCANBLOCK);
        if (!Forward(glob, EFSACT_EXNEXT, EFS_BUFSIZE, 1, &res1, &res2))
        {
            FAIL(dp, res2);
            return;
        }
        entries = GetL(glob->Buf, WH_ARG5);
        l->ScanStarted = TRUE;
        l->ScanMore = res1 != 0;
        l->ScanType = GetL(glob->Buf, WH_ARG4);
        if (!l->Scan && !(l->Scan = AllocVec(EFS_SCANBLOCK, MEMF_PUBLIC)))
        {
            FAIL(dp, ERROR_NO_FREE_STORE);
            return;
        }
        memcpy(l->Scan, glob->Buf + EFS_HDR, EFS_SCANBLOCK);
        l->ScanEntries = entries;
        l->ScanNext = entries ? 0 : 0xFFFFFFFFUL;
        if (!entries && !l->ScanMore)
        {
            FAIL(dp, (res2 && res2 != ERROR_NO_MORE_ENTRIES) ? res2 : ERROR_NO_MORE_ENTRIES);
            ResetScan(glob, l);
            return;
        }
    }
    {
        ULONG off = l->ScanNext, next;
        ExRecToFib(glob, l->Scan, off, l->ScanType, fib);
        next = GetL(l->Scan, off);
        l->ScanNext = (next && next < EFS_SCANBLOCK - 40 && --l->ScanEntries) ? next : 0xFFFFFFFFUL;
        OK(dp);
    }
}

/* ExAll: a block from the server, converted to this system's ExAllData records */
static void ActExAll(struct Globals *glob, struct DosPacket *dp)
{
    UBYTE *buf = (UBYTE *)dp->dp_Arg2;
    ULONG size = (ULONG)dp->dp_Arg3, type = (ULONG)dp->dp_Arg4, wire, handle, entries, rtype, off, count = 0, used = 0;
    struct ExAllControl *eac = (struct ExAllControl *)dp->dp_Arg5;
    struct Transaction *t;
    struct ExAllData *prev = NULL;
    UBYTE *req, *resp;
    LONG err, res1, res2;
    ULONG terr;

    if (type < ED_NAME || type > ED_OWNER || !buf || !eac)
    {
        FAIL(dp, ERROR_BAD_NUMBER);
        return;
    }
    if (!dp->dp_Arg1)
    {
        struct EfsLock *l = RootScanLock(glob);
        if (!l) { FAIL(dp, ERROR_INVALID_LOCK); return; }
        handle = l->Handle;
    }
    else if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &handle, &err))
    {
        FAIL(dp, err);
        return;
    }
    if (!glob->Connected && !Reconnect(glob))
    {
        FAIL(dp, ERROR_SEEK_ERROR);
        return;
    }
    wire = size * 2 / 3;
    if (wire > 16384) wire = 16384;
    if (wire < 128)  wire = 128;
    if (!(t = AllocTransaction(TRN_AllocReqBuffer, EFS_HDR, TRN_AllocRespBuffer, EFS_HDR + wire, TAG_DONE)))
    {
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    req = t->trans_RequestData;
    resp = t->trans_ResponseData;
    HdrSet(glob, req, ACTION_EXAMINE_ALL, ++glob->NextCookie, handle, EFS_HDR, wire, type, glob->Flags & 0xFFFF, 0);
    t->trans_Command = EFSCMD_PACKET;
    t->trans_ReqDataActual = EFS_HDR;
    t->trans_RespDataLength = EFS_HDR + wire;
    t->trans_Timeout = (UWORD)(6 + glob->ExtraTimeout);
    terr = DoTrans(glob, t);
    if (terr)
    {
        FreeTransaction(t);
        glob->Connected = FALSE;
        glob->Retry = TRUE;
        FAIL(dp, ERROR_SEEK_ERROR);
        return;
    }
    res1 = (LONG)GetL(resp, WH_RES1);
    res2 = (LONG)GetL(resp, WH_RES2);
    entries = GetL(resp, WH_ARG5);
    rtype = GetL(resp, WH_ARG4);
    if (rtype < type && type == ED_OWNER)
        type = rtype;

    for (off = 0; entries > 0 && off < wire; entries--)
    {
        const UBYTE *blk = resp + EFS_HDR;
        ULONG nameoff = GetL(blk, off + 4), comoff = type >= ED_COMMENT ? GetL(blk, off + 32) : 0, next = GetL(blk, off);
        BOOL hasname = nameoff && nameoff < wire, hascom = comoff && comoff < wire;
        CONST_STRPTR name = hasname ? (CONST_STRPTR)blk + nameoff : (CONST_STRPTR)"";
        CONST_STRPTR com = hascom ? (CONST_STRPTR)blk + comoff : (CONST_STRPTR)"";
        ULONG nlen = hasname ? strnlen(name, wire - nameoff) : 0, clen = hascom ? strnlen(com, wire - comoff) : 0;
        ULONG need = Align4(sizeof(struct ExAllData) + nlen + 1 + (type >= ED_COMMENT ? clen + 1 : 0));
        struct ExAllData *ed;
        BOOL take = TRUE;

        if (used + need > size)
            break;                          /* no room: the rest of this block is lost */
        if (eac->eac_MatchString && !MatchPatternNoCase(eac->eac_MatchString, (STRPTR)name))
            take = FALSE;
        if (take)
        {
            ed = (struct ExAllData *)(buf + used);
            memset(ed, 0, sizeof(*ed));
            ed->ed_Name = (UBYTE *)ed + sizeof(*ed);
            memcpy(ed->ed_Name, name, nlen);
            ed->ed_Name[nlen] = '\0';
            if (type >= ED_TYPE)  ed->ed_Type = (LONG)GetL(blk, off + 8);
            if (type >= ED_SIZE)  ed->ed_Size = GetL(blk, off + 12);
            if (type >= ED_PROTECTION) ed->ed_Prot = GetL(blk, off + 16);
            if (type >= ED_DATE)
            {
                ed->ed_Days = GetL(blk, off + 20);
                ed->ed_Mins = GetL(blk, off + 24);
                ed->ed_Ticks = GetL(blk, off + 28);
            }
            if (type >= ED_COMMENT)
            {
                ed->ed_Comment = ed->ed_Name + nlen + 1;
                memcpy(ed->ed_Comment, com, clen);
                ed->ed_Comment[clen] = '\0';
            }
            if (type >= ED_OWNER)
            {
                ed->ed_OwnerUID = GetW(blk, off + 36);
                ed->ed_OwnerGID = GetW(blk, off + 38);
            }
            if (eac->eac_MatchFunc)
            {
                LONG mtype = type;
                if (!CallHookPkt(eac->eac_MatchFunc, ed, &mtype))
                    take = FALSE;
            }
        }
        if (take)
        {
            if (prev)
                prev->ed_Next = ed;
            prev = ed;
            used += need;
            count++;
        }
        if (!next || next >= wire)
            break;
        off = next;
    }
    if (prev)
        prev->ed_Next = NULL;
    eac->eac_Entries = count;
    FreeTransaction(t);
    if (res1)
    {
        OK(dp);
    }
    else
    {
        dp->dp_Res1 = DOSFALSE;
        dp->dp_Res2 = (res2 && res2 != ERROR_NO_MORE_ENTRIES) ? res2 : ERROR_NO_MORE_ENTRIES;
    }
}

static void ActExAllEnd(struct Globals *glob, struct DosPacket *dp)
{
    struct EfsLock *l = LockFromBPTR(glob, (BPTR)dp->dp_Arg1);
    ULONG handle;
    LONG err, res1, res2;

    if (l)
        ResetScan(glob, l);
    if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &handle, &err) || !glob->Connected)
    {
        OK(dp);
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_EXAMINE_ALL_END, ++glob->NextCookie, handle, 0, 0, 0, 0, 0);
    Forward(glob, ACTION_EXAMINE_ALL_END, EFS_BUFSIZE, 1, &res1, &res2);
    glob->Retry = FALSE;
    OK(dp);
}

static void ActInfo(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    struct InfoData *id = BADDR((BPTR)(action == ACTION_INFO ? dp->dp_Arg2 : dp->dp_Arg1));
    ULONG handle = 0;
    LONG err, res1, res2;
    const UBYTE *d;

    if (!id)
    {
        FAIL(dp, ERROR_REQUIRED_ARG_MISSING);
        return;
    }
    if (action == ACTION_INFO)
    {
        if (!LockHandle(glob, (BPTR)dp->dp_Arg1, &handle, &err)) { FAIL(dp, err); return; }
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, handle, EFS_HDR, 0, 0, 0, 0);
    }
    else
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, EFS_HDR, 0, 0, 0, 0, 0);

    memset(id, 0, sizeof(*id));
    if (!glob->Connected && !Reconnect(glob))
    {
        /* answered locally while the server is away */
        id->id_DiskState = ID_VALIDATING;
        id->id_NumBlocks = 1;
        id->id_NumBlocksUsed = 1;
        id->id_BytesPerBlock = 512;
        id->id_DiskType = (LONG)-1;
        id->id_VolumeNode = MKBADDR((glob->CurVol ? glob->CurVol->DosList : NULL));
        OK(dp);
        return;
    }
    if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    if (!res1 || glob->Trans->trans_RespDataActual < EFS_HDR + 36)
    {
        FAIL(dp, res2);
        return;
    }
    d = glob->Buf + EFS_HDR;
    id->id_NumSoftErrors = (LONG)GetL(d, 0);
    id->id_UnitNumber = (LONG)GetL(d, 4);
    id->id_DiskState = (LONG)GetL(d, 8);
    id->id_NumBlocks = (LONG)GetL(d, 12);
    id->id_NumBlocksUsed = (LONG)GetL(d, 16);
    id->id_BytesPerBlock = (LONG)GetL(d, 20);
    id->id_DiskType = (GetL(d, 24) == 0x42555359UL) ? (LONG)0x42555359UL : ID_DOS_DISK;
    id->id_VolumeNode = MKBADDR((glob->CurVol ? glob->CurVol->DosList : NULL));
    id->id_InUse = GetL(d, 32);
    if (id->id_DiskState == ID_VALIDATED && glob->WriteProtect)
        id->id_DiskState = ID_WRITE_PROTECTED;
    OK(dp);
}

static void ActFlush(struct Globals *glob, struct DosPacket *dp)
{
    LONG res1, res2;

    if (!glob->Connected)
    {
        OK(dp);
        return;
    }
    HdrSet(glob, glob->Buf, ACTION_FLUSH, ++glob->NextCookie, 0, 0, 0, 0, 0, 0);
    if (!Forward(glob, ACTION_FLUSH, EFS_BUFSIZE, 1, &res1, &res2))
    {
        glob->Retry = FALSE;
        OK(dp);
        return;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
}

static void ActRecord(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    struct EfsFile *f;
    LONG err, res1, res2;
    ULONG tmult = 1;

    if (!FileHandle(glob, dp->dp_Arg1, &f, &err))
    {
        FAIL(dp, err);
        return;
    }
    HdrSet(glob, glob->Buf, action, ++glob->NextCookie, f->Handle, (ULONG)dp->dp_Arg2, (ULONG)dp->dp_Arg3, (ULONG)dp->dp_Arg4, (ULONG)dp->dp_Arg5, 0);
    if (action == ACTION_LOCK_RECORD)
    {
        /* the lock may wait up to its timeout on the server */
        ULONG secs = (ULONG)dp->dp_Arg5 / 50;
        glob->Trans->trans_Timeout = 0;
        tmult = 1;
        if (!Forward(glob, action, EFS_BUFSIZE, tmult, &res1, &res2))
        {
            FAIL(dp, res2);
            return;
        }
        (void)secs;
    }
    else if (!Forward(glob, action, EFS_BUFSIZE, tmult, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
    if (res1 && action == ACTION_LOCK_RECORD)
    {
        struct EfsRecord *r = AllocVec(sizeof(struct EfsRecord), MEMF_PUBLIC | MEMF_CLEAR);
        if (r)
        {
            r->Offset = (ULONG)dp->dp_Arg2;
            r->Length = (ULONG)dp->dp_Arg3;
            r->Mode = (ULONG)dp->dp_Arg4;
            r->Timeout = (ULONG)dp->dp_Arg5;
            AddTail((struct List *)&f->Records, (struct Node *)&r->Node);
        }
    }
    else if (res1 && action == ACTION_FREE_RECORD)
    {
        struct EfsRecord *r;
        ForeachNode(&f->Records, r)
        {
            if (r->Offset == (ULONG)dp->dp_Arg2 && r->Length == (ULONG)dp->dp_Arg3)
            {
                Remove((struct Node *)&r->Node);
                FreeVec(r);
                break;
            }
        }
    }
}

static void ActAddNotify(struct Globals *glob, struct DosPacket *dp)
{
    struct NotifyRequest *nr = (struct NotifyRequest *)dp->dp_Arg1;
    struct EfsNotify *n;
    LONG res1, res2;
    ULONG end, key;

    if (!nr || !nr->nr_FullName)
    {
        FAIL(dp, ERROR_REQUIRED_ARG_MISSING);
        return;
    }
    key = ++glob->NextKey;
    HdrSet(glob, glob->Buf, ACTION_ADD_NOTIFY, ++glob->NextCookie, key, EFS_HDR, glob->CurVol ? glob->CurVol->ID : 0, nr->nr_Flags, 0, 0);
    end = PutCName(glob->Buf, EFS_HDR, nr->nr_FullName);
    if (!Forward(glob, ACTION_ADD_NOTIFY, end, 1, &res1, &res2))
    {
        FAIL(dp, res2);
        return;
    }
    if (!res1)
    {
        FAIL(dp, res2);
        return;
    }
    if (!(n = AllocVec(sizeof(struct EfsNotify), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        FAIL(dp, ERROR_NO_FREE_STORE);
        return;
    }
    n->NR = nr;
    n->Key = key;
    n->Handle = res1;
    n->VolID = glob->CurVol ? glob->CurVol->ID : 0;
    nr->nr_Handler = glob->Port;
    nr->nr_MsgCount = 0;
    AddTail((struct List *)&glob->Notifies, (struct Node *)&n->Node);
    OK(dp);
}

static void ActRemoveNotify(struct Globals *glob, struct DosPacket *dp)
{
    struct NotifyRequest *nr = (struct NotifyRequest *)dp->dp_Arg1;
    struct EfsNotify *n;
    LONG res1, res2;

    ForeachNode(&glob->Notifies, n)
    {
        if (n->NR == nr)
        {
            if (glob->Connected)
            {
                HdrSet(glob, glob->Buf, ACTION_REMOVE_NOTIFY, ++glob->NextCookie, n->Handle, 0, 0, 0, 0, 0);
                Forward(glob, ACTION_REMOVE_NOTIFY, EFS_BUFSIZE, 1, &res1, &res2);
                glob->Retry = FALSE;
            }
            Remove((struct Node *)&n->Node);
            FreeVec(n);
            OK(dp);
            return;
        }
    }
    FAIL(dp, ERROR_OBJECT_NOT_FOUND);
}

/* 20000..20004 (§2.10, §7.3) */
static void ActUser(struct Globals *glob, struct DosPacket *dp, ULONG action)
{
    LONG res1, res2;
    ULONG end;

    switch (action)
    {
    case EFSACT_NAME2UID:
    case EFSACT_NAME2GID:
        if (!dp->dp_Arg1) { FAIL(dp, ERROR_REQUIRED_ARG_MISSING); return; }
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, EFS_HDR, 0, 0, 0, 0, 0);
        PutCName(glob->Buf, EFS_HDR, (CONST_STRPTR)dp->dp_Arg1);
        if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2)) { FAIL(dp, res2); return; }
        dp->dp_Res1 = res1;
        dp->dp_Res2 = res2;
        return;

    case EFSACT_UID2INFO:
    case EFSACT_GID2INFO:
        if (!dp->dp_Arg2) { FAIL(dp, ERROR_REQUIRED_ARG_MISSING); return; }
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, (ULONG)dp->dp_Arg1 & 0xFFFF, EFS_HDR, 0, 0, 0, 0);
        if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2)) { FAIL(dp, res2); return; }
        if (res1 && glob->Trans->trans_RespDataActual >= EFS_HDR + 40)
        {
            struct UserInfo *ui = (struct UserInfo *)dp->dp_Arg2;      /* same layout as GroupInfo */
            const UBYTE *d = glob->Buf + EFS_HDR;
            memcpy(ui->ui_UserName, d, 32);
            ui->ui_UserID = GetW(d, 32);
            ui->ui_PrimaryGroupID = GetW(d, 34);
            ui->ui_Flags = GetL(d, 36);
        }
        dp->dp_Res1 = res1 ? DOSTRUE : DOSFALSE;
        dp->dp_Res2 = res2;
        return;

    case EFSACT_LOGIN:
    {
        ULONG off2;
        if (!dp->dp_Arg1) { FAIL(dp, ERROR_REQUIRED_ARG_MISSING); return; }
        end = PutCName(glob->Buf, EFS_HDR, (CONST_STRPTR)dp->dp_Arg1);
        off2 = Align4(end);
        HdrSet(glob, glob->Buf, action, ++glob->NextCookie, EFS_HDR, off2, 0, 0, 0, 0);
        PutCName(glob->Buf, EFS_HDR, (CONST_STRPTR)dp->dp_Arg1);
        PutCName(glob->Buf, off2, dp->dp_Arg2 ? (CONST_STRPTR)dp->dp_Arg2 : (CONST_STRPTR)"");
        if (!Forward(glob, action, EFS_BUFSIZE, 1, &res1, &res2)) { FAIL(dp, res2); return; }
        dp->dp_Res1 = res1 ? DOSTRUE : DOSFALSE;
        dp->dp_Res2 = res2;
        return;
    }
    }
}

static void Dispatch(struct Globals *glob, struct DosPacket *dp)
{
    switch (dp->dp_Type)
    {
    case ACTION_LOCATE_OBJECT:
        ActLocate(glob, dp, ACTION_LOCATE_OBJECT);
        break;
    case ACTION_CREATE_DIR:
        if (glob->WriteProtect)
            FAIL(dp, ERROR_DISK_WRITE_PROTECTED);
        else
            ActLocate(glob, dp, ACTION_CREATE_DIR);
        break;
    case ACTION_COPY_DIR:
    case ACTION_PARENT:
    case ACTION_COPY_DIR_FH:
    case ACTION_PARENT_FH:
        ActDupParent(glob, dp, dp->dp_Type);
        break;
    case ACTION_FREE_LOCK:
        ActFreeLock(glob, dp);
        break;
    case ACTION_SAME_LOCK:
        ActSameLock(glob, dp);
        break;
    case ACTION_FINDUPDATE:
    case ACTION_FINDINPUT:
    case ACTION_FINDOUTPUT:
        ActOpen(glob, dp, dp->dp_Type);
        break;
    case ACTION_FH_FROM_LOCK:
        ActFhFromLock(glob, dp);
        break;
    case ACTION_END:
        ActEnd(glob, dp);
        break;
    case ACTION_SEEK:
        ActSeek(glob, dp);
        break;
    case ACTION_SET_FILE_SIZE:
        ActSetFileSize(glob, dp);
        break;
    case ACTION_CHANGE_MODE:
        ActChangeMode(glob, dp);
        break;
    case ACTION_READ:
        DoReadWrite(glob, dp, FALSE);
        break;
    case ACTION_WRITE:
        DoReadWrite(glob, dp, TRUE);
        break;
    case ACTION_DELETE_OBJECT:
    case ACTION_RENAME_OBJECT:
    case ACTION_SET_PROTECT:
    case ACTION_SET_COMMENT:
    case ACTION_SET_DATE:
    case ACTION_SET_OWNER:
    case ACTION_MAKE_LINK:
        ActNameOp(glob, dp, dp->dp_Type);
        break;
    case ACTION_EXAMINE_OBJECT:
    case ACTION_EXAMINE_FH:
        ActExamine(glob, dp, dp->dp_Type);
        break;
    case ACTION_EXAMINE_NEXT:
        ActExNext(glob, dp);
        break;
    case ACTION_EXAMINE_ALL:
        ActExAll(glob, dp);
        break;
    case ACTION_EXAMINE_ALL_END:
        ActExAllEnd(glob, dp);
        break;
    case ACTION_DISK_INFO:
    case ACTION_INFO:
        ActInfo(glob, dp, dp->dp_Type);
        break;
    case ACTION_FLUSH:
        ActFlush(glob, dp);
        break;
    case ACTION_LOCK_RECORD:
    case ACTION_FREE_RECORD:
        ActRecord(glob, dp, dp->dp_Type);
        break;
    case ACTION_ADD_NOTIFY:
        ActAddNotify(glob, dp);
        break;
    case ACTION_REMOVE_NOTIFY:
        ActRemoveNotify(glob, dp);
        break;
    case EFSACT_NAME2UID:
    case EFSACT_NAME2GID:
    case EFSACT_UID2INFO:
    case EFSACT_GID2INFO:
    case EFSACT_LOGIN:
        ActUser(glob, dp, dp->dp_Type);
        break;
    case EFSACT_QUERYMOUNT:
        if (dp->dp_Arg1 && dp->dp_Arg3 > 0)
            CopyStr((char *)dp->dp_Arg1, (ULONG)dp->dp_Arg3, glob->Host);
        if (dp->dp_Arg2 && dp->dp_Arg4 > 0)
            CopyStr((char *)dp->dp_Arg2, (ULONG)dp->dp_Arg4, glob->Export);
        dp->dp_Res1 = -1;
        dp->dp_Res2 = 0;
        break;
    case ACTION_IS_FILESYSTEM:
        dp->dp_Res1 = (glob->CurVol && (glob->CurVol->Flags & VOLF_FILESYSTEM)) ? DOSTRUE : DOSFALSE;
        dp->dp_Res2 = 0;
        break;
    case ACTION_CURRENT_VOLUME:
    {
        struct EfsFile *f = FileFromArg(glob, dp->dp_Arg1);
        struct EfsVolume *v = (f && f->Vol) ? f->Vol : glob->CurVol;
        dp->dp_Res1 = (SIPTR)MKBADDR((v ? v->DosList : NULL));
        dp->dp_Res2 = 0;
        break;
    }
    case ACTION_INHIBIT:
        OK(dp);
        break;
    case ACTION_WRITE_PROTECT:
        glob->WriteProtect = dp->dp_Arg1 != 0;
        OK(dp);
        break;
    case ACTION_DIE:
        glob->Quit = TRUE;
        OK(dp);
        break;
    case ACTION_RENAME_DISK:
    case ACTION_MORE_CACHE:
    case ACTION_FORMAT:
    case ACTION_READ_LINK:
    default:
        FAIL(dp, ERROR_ACTION_NOT_KNOWN);
        break;
    }
}

/* actions whose Res1 == 0 means failure: such a failure must carry an error code */
static BOOL BoolAction(LONG type)
{
    switch (type)
    {
    case ACTION_LOCATE_OBJECT: case ACTION_CREATE_DIR: case ACTION_COPY_DIR: case ACTION_COPY_DIR_FH:
    case ACTION_PARENT_FH: case ACTION_FINDUPDATE: case ACTION_FINDINPUT: case ACTION_FINDOUTPUT:
    case ACTION_FH_FROM_LOCK: case ACTION_DELETE_OBJECT: case ACTION_RENAME_OBJECT: case ACTION_SET_PROTECT:
    case ACTION_SET_COMMENT: case ACTION_SET_DATE: case ACTION_SET_OWNER: case ACTION_MAKE_LINK:
    case ACTION_EXAMINE_OBJECT: case ACTION_EXAMINE_FH: case ACTION_EXAMINE_NEXT: case ACTION_EXAMINE_ALL:
    case ACTION_INFO: case ACTION_DISK_INFO: case ACTION_CHANGE_MODE: case ACTION_LOCK_RECORD:
    case ACTION_FREE_RECORD: case ACTION_ADD_NOTIFY: case ACTION_REMOVE_NOTIFY: case ACTION_SET_FILE_SIZE:
    case EFSACT_UID2INFO: case EFSACT_GID2INFO: case EFSACT_LOGIN:
        return TRUE;
    }
    return FALSE;
}

static void EnsureError(struct DosPacket *dp)
{
    BOOL failed;

    if (dp->dp_Res2)
        return;
    if (dp->dp_Type == ACTION_READ || dp->dp_Type == ACTION_WRITE || dp->dp_Type == ACTION_SEEK || dp->dp_Type == ACTION_SET_FILE_SIZE)
        failed = dp->dp_Res1 == -1;
    else
        failed = BoolAction(dp->dp_Type) && dp->dp_Res1 == 0;
    if (failed)
        dp->dp_Res2 = ERROR_SEEK_ERROR;     /* §4.3: what the original reports for a lost request */
}

void HandlePacket(struct Globals *glob, struct DosPacket *dp)
{
    glob->Retry = FALSE;
    EFSLOG("packet %ld args %lx %lx %lx %lx\n", (long)dp->dp_Type, (unsigned long)dp->dp_Arg1, (unsigned long)dp->dp_Arg2, (unsigned long)dp->dp_Arg3, (unsigned long)dp->dp_Arg4);
    Dispatch(glob, dp);
    EFSLOG("  -> %ld %ld\n", (long)dp->dp_Res1, (long)dp->dp_Res2);
    if (glob->Retry)
    {
        /* the connection went away under this packet: reconnect once and redo it */
        glob->Retry = FALSE;
        if (glob->Connected || Reconnect(glob))
            Dispatch(glob, dp);
        if (glob->Retry)
        {
            glob->Retry = FALSE;
            dp->dp_Res1 = (dp->dp_Type == ACTION_READ || dp->dp_Type == ACTION_WRITE || dp->dp_Type == ACTION_SEEK || dp->dp_Type == ACTION_SET_FILE_SIZE) ? -1 : 0;
            dp->dp_Res2 = ERROR_SEEK_ERROR;
        }
    }
    EnsureError(dp);
    EfsReplyPkt(glob, dp);
}
