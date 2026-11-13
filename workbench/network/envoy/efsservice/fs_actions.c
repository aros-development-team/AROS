/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - the EFS packet (re/spec/efs-protocol.md §2,
          §3): one DOS action per command-3 transaction, executed against
          the export with dos.library calls. The 52-byte header is echoed,
          Res1/Res2 carry the DOS result, handles are 32-bit server IDs.
          Full File Security (§5.3) is enforced here, since the local
          filesystem knows nothing of the remote user.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <string.h>

#include "fs_intern.h"

/*
 * fib_DiskKey on the wire: a stable, non-zero key per object, derived from its full path.
 * AROS filesystems use the field as an ExNext cookie (RAM: stores a pointer to its scan
 * state there), so passing it on makes every entry carry the same key, and the 3.1 Dir
 * command then reports a circular directory. The original server passed its filesystem's
 * block number, which is unique per object; a case-insensitive hash of the path is.
 */
static ULONG PathKey(CONST_STRPTR path, CONST_STRPTR name)
{
    ULONG h = 0x811C9DC5UL;
    CONST_STRPTR p;
    int part;

    for (part = 0; part < 2; part++)
    {
        p = part ? name : path;
        if (!p)
            continue;
        if (part && *p && h != 0x811C9DC5UL)
        {
            h ^= '/';
            h *= 0x01000193UL;
        }
        for (; *p; p++)
        {
            UBYTE c = *p;
            if (c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            h ^= c;
            h *= 0x01000193UL;
        }
    }
    return h ? h : 1;
}

static ULONG LockKey(BPTR lock, CONST_STRPTR name)
{
    char path[512];

    if (!NameFromLock(lock, path, sizeof(path)))
        path[0] = '\0';
    return PathKey(path, name);
}
#include <proto/accounts.h>

#define UtilityBase  (srv->UtilLib)
#define AccountsBase (srv->AccLib)

struct Ctx
{
    struct FSServer     *srv;
    struct Mount        *m;
    struct Transaction  *t;
    UBYTE               *req, *resp;
    ULONG                reqlen, resplen;
    ULONG                act;
    ULONG                arg[6];
    ULONG                cflags;        /* per-mount flags OR-ed with the request's client flags */
};

#define ARG(n)  (ctx->arg[(n) - 1])

static void SetRes(struct Ctx *ctx, LONG res1, LONG res2)
{
    EfsPut32(ctx->resp + 12, (ULONG)res1);
    EfsPut32(ctx->resp + 16, (ULONG)res2);
}

static void Fail(struct Ctx *ctx, LONG res1, LONG res2)
{
    SetRes(ctx, res1, res2);
    ctx->t->trans_RespDataActual = EFS_HDR;
}

static struct LockRec *FindLock(struct Mount *m, ULONG handle)
{
    struct LockRec *l;
    ForeachNode(&m->Locks, l)
        if (l->Handle == handle)
            return l;
    return NULL;
}

static struct FileRec *FindFile(struct Mount *m, ULONG handle)
{
    struct FileRec *f;
    ForeachNode(&m->Files, f)
        if (f->Handle == handle)
            return f;
    return NULL;
}

static struct NotifyRec *FindNotify(struct Mount *m, ULONG handle)
{
    struct NotifyRec *n;
    ForeachNode(&m->Notifies, n)
        if (n->Handle == handle)
            return n;
    return NULL;
}

/* the directory a lock handle stands for; 0 = the export root */
static BPTR DirOf(struct Ctx *ctx, ULONG handle, struct LockRec **lrp)
{
    struct LockRec *l;

    if (lrp)
        *lrp = NULL;
    if (handle == 0)
        return ctx->m->Export->RootLock;
    if (!(l = FindLock(ctx->m, handle)))
        return BNULL;
    if (lrp)
        *lrp = l;
    return l->Lock;
}

static BOOL IsRoot(struct Ctx *ctx, BPTR lock)
{
    struct FSServer *srv = ctx->srv;
    return lock == ctx->m->Export->RootLock || SameLock(lock, ctx->m->Export->RootLock) == LOCK_SAME;
}

/*
 * A BSTR name at a request offset, resolved relative to a lock handle:
 * a component ending in ':' restarts at the export root (§2.1).
 */
static BOOL ResolveName(struct Ctx *ctx, ULONG handle, ULONG off, BPTR *dir, STRPTR rel, ULONG relsize)
{
    struct FSServer *srv = ctx->srv;
    char *colon;

    if (!EfsGetBSTR(ctx->req, ctx->reqlen, off, srv->Name, sizeof(srv->Name)))
    {
        Fail(ctx, 0, ERROR_INVALID_COMPONENT_NAME);
        return FALSE;
    }
    if ((colon = strrchr(srv->Name, ':')))
    {
        *dir = ctx->m->Export->RootLock;
        EfsPutCStr((UBYTE *)rel, relsize, colon + 1);
        return TRUE;
    }
    if (!(*dir = DirOf(ctx, handle, NULL)))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return FALSE;
    }
    EfsPutCStr((UBYTE *)rel, relsize, srv->Name);
    return TRUE;
}

/* "<VolumeName>:<path relative to the export root>" for a lock (§2.3) */
static void VolPath(struct Ctx *ctx, BPTR lock, STRPTR dst, ULONG size)
{
    struct FSServer *srv = ctx->srv;
    struct Export *e = ctx->m->Export;
    CONST_STRPTR rest = "";
    ULONG rootlen = strlen(e->RootPath);

    if (!IsRoot(ctx, lock) && NameFromLock(lock, srv->Path, sizeof(srv->Path)))
    {
        if (!Strnicmp(srv->Path, e->RootPath, rootlen))
        {
            rest = srv->Path + rootlen;
            if (*rest == '/')
                rest++;
        }
        else
            rest = FilePart(srv->Path);
    }
    EfsPutCStr((UBYTE *)dst, size, e->VolName);
    if (strlen(dst) + 1 + strlen(rest) < size)
    {
        strcat(dst, ":");
        strcat(dst, rest);
    }
}

/* the full local path of a name relative to a directory lock */
static void FullPath(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel, STRPTR dst, ULONG size)
{
    if (!NameFromLock(dir, dst, size))
        dst[0] = '\0';
    if (rel[0])
        AddPart(dst, rel, size);
}

/* answer with a new lock handle, the volume ID and the path (§2.3) */
static void AnswerLock(struct Ctx *ctx, BPTR lock, BOOL readonly, BOOL setarg3)
{
    struct FSServer *srv = ctx->srv;
    struct LockRec *l;
    ULONG n;

    if (!(l = AllocVec(sizeof(struct LockRec), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        UnLock(lock);
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    l->Handle = ServerNewHandle(srv);
    l->Lock = lock;
    l->ReadOnly = readonly;
    AddTail((struct List *)&ctx->m->Locks, (struct Node *)l);
    VolPath(ctx, lock, srv->Path2, sizeof(srv->Path2));
    n = strlen(srv->Path2) + 1;
    if (EFS_HDR + n > ctx->resplen)
        n = ctx->resplen - EFS_HDR;
    CopyMem(srv->Path2, ctx->resp + EFS_HDR, n);
    ctx->resp[EFS_HDR + n - 1] = '\0';
    SetRes(ctx, (LONG)l->Handle, (LONG)ctx->m->Export->VolumeID);
    if (setarg3)
        EfsPut32(ctx->resp + 28, (ULONG)-2);
    ctx->t->trans_RespDataActual = EFS_HDR + n;
}

/* ---- rights (§5.3) --------------------------------------------------------- */

static ULONG RightsOnLock(struct Ctx *ctx, BPTR lock)
{
    struct FSServer *srv = ctx->srv;
    struct FileInfoBlock *fib;
    ULONG r = RIGHTS_ALL;

    if (!(ctx->m->Flags & MNTF_FULLSEC) || !lock || IsRoot(ctx, lock))
        return RIGHTS_ALL;
    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if (Examine(lock, fib))
            r = AuthRights(srv, ctx->m, fib, NULL);
        FreeDosObject(DOS_FIB, fib);
    }
    return r;
}

/* rights on a named object; RIGHTS_ALL when it does not exist (the caller then fails anyway) */
static ULONG RightsOnName(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel, BOOL *exists)
{
    BPTR old, l;
    ULONG r = RIGHTS_ALL;

    if (exists)
        *exists = FALSE;
    if (!(ctx->m->Flags & MNTF_FULLSEC) && !exists)
        return RIGHTS_ALL;
    old = CurrentDir(dir);
    if ((l = Lock(rel, SHARED_LOCK)))
    {
        if (exists)
            *exists = TRUE;
        r = RightsOnLock(ctx, l);
        UnLock(l);
    }
    CurrentDir(old);
    return r;
}

/* may this user change protection or owner of the object? (§5.3) */
static BOOL MayChangeAttrs(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel)
{
    struct FSServer *srv = ctx->srv;
    struct FileInfoBlock *fib;
    BPTR old, l;
    BOOL ok = TRUE;

    if (!(ctx->m->Flags & MNTF_FULLSEC) || (ctx->m->AccFlags & UFLAGF_AdminAll))
        return TRUE;
    ok = FALSE;
    old = CurrentDir(dir);
    if ((l = Lock(rel, SHARED_LOCK)))
    {
        if ((fib = AllocDosObject(DOS_FIB, NULL)))
        {
            if (Examine(l, fib) && fib->fib_OwnerUID != 0 && ctx->m->Authenticated && fib->fib_OwnerUID == ctx->m->Uid)
                ok = TRUE;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(l);
    }
    CurrentDir(old);
    return ok;
}

/* ---- Workbench files in the export root (§5.5) -------------------------------- */

#define WB_NONE     0
#define WB_DISKINFO 1
#define WB_BACKDROP 2

static int WBRule(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel)
{
    struct FSServer *srv = ctx->srv;

    if (strchr(rel, '/') || !IsRoot(ctx, dir))
        return WB_NONE;
    if ((ctx->cflags & MNTF_PROTDISKINFO) && !Stricmp(rel, "Disk.info"))
        return WB_DISKINFO;
    if ((ctx->cflags & MNTF_HIDEBACKDROP) && !Stricmp(rel, ".backdrop"))
        return WB_BACKDROP;
    return WB_NONE;
}

/* owner and protection of every object this mount creates (§5.4) */
static void NewObjectAttrs(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel)
{
    BPTR old = CurrentDir(dir);
    ULONG owner = ctx->m->Authenticated ? (((ULONG)ctx->m->Uid << 16) | ctx->m->Gid) : 0;

    SetOwner((STRPTR)rel, owner);                   /* nobody for anonymous mounts */
    SetProtection((STRPTR)rel, 0x0000AA00);
    CurrentDir(old);
}

static void Changed(struct Ctx *ctx, BPTR dir, CONST_STRPTR rel)
{
    struct FSServer *srv = ctx->srv;
    FullPath(ctx, dir, rel, srv->Path, sizeof(srv->Path));
    ServerNotifyChanged(srv, ctx->m, srv->Path);
}

/* ---- scans (§2.6) ---------------------------------------------------------- */

static void ScanEnd(struct Ctx *ctx, struct LockRec *l)
{
    struct FSServer *srv = ctx->srv;

    if (l->Eac)
    {
        if (l->EacMore)
            ExAllEnd(l->Lock, l->EacBuf, l->EacBufSize, l->EacType, l->Eac);
        FreeDosObject(DOS_EXALLCONTROL, l->Eac);
        l->Eac = NULL;
    }
    FreeVec(l->EacBuf);
    l->EacBuf = NULL;
    FreeVec(l->EacPattern);
    l->EacPattern = NULL;
    l->EacMore = FALSE;
    l->EacNext = NULL;
}

/* fetch the next block of a directory scan into the lock's native buffer */
static BOOL ScanBlock(struct Ctx *ctx, struct LockRec *l, ULONG size, LONG *type)
{
    struct FSServer *srv = ctx->srv;
    BOOL more;

    if (!l->Eac)
    {
        if (!(l->Eac = AllocDosObject(DOS_EXALLCONTROL, NULL)))
            return FALSE;
        l->EacType = *type;
        if ((ctx->cflags & MNTF_HIDEBACKDROP) && IsRoot(ctx, l->Lock) && (l->EacPattern = AllocVec(40, MEMF_PUBLIC)))
        {
            if (ParsePatternNoCase("~(.backdrop)", l->EacPattern, 40) >= 0)
                l->Eac->eac_MatchString = l->EacPattern;
        }
    }
    if (!l->EacBuf || l->EacBufSize < size * 2 + 1024)
    {
        FreeVec(l->EacBuf);
        l->EacBufSize = size * 2 + 1024;
        if (!(l->EacBuf = AllocVec(l->EacBufSize, MEMF_PUBLIC | MEMF_CLEAR)))
            return FALSE;
    }
    more = ExAll(l->Lock, l->EacBuf, l->EacBufSize, l->EacType, l->Eac);
    if (!more && IoErr() == ERROR_BAD_NUMBER && l->EacType == ED_OWNER)
    {
        l->EacType = ED_COMMENT;                    /* no owner support: lower the type (§2.6) */
        more = ExAll(l->Lock, l->EacBuf, l->EacBufSize, l->EacType, l->Eac);
    }
    *type = l->EacType;
    l->EacMore = more;
    l->EacNext = l->Eac->eac_Entries ? (struct ExAllData *)l->EacBuf : NULL;
    return TRUE;
}

static void ActScan(struct Ctx *ctx, BOOL exall)
{
    struct FSServer *srv = ctx->srv;
    struct LockRec *l;
    BPTR lock;
    ULONG off = ARG(2), size = ARG(3), count = 0, used;
    LONG type = (LONG)ARG(4);

    if (!(lock = DirOf(ctx, ARG(1), &l)))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    if (!l)
    {
        /* the root has no record: give it one so the scan state has a home */
        BPTR dup = DupLock(lock);
        if (!dup || !(l = AllocVec(sizeof(struct LockRec), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            if (dup)
                UnLock(dup);
            Fail(ctx, 0, ERROR_NO_FREE_STORE);
            return;
        }
        l->Handle = ServerNewHandle(srv);
        l->Lock = dup;
        AddTail((struct List *)&ctx->m->Locks, (struct Node *)l);
        EfsPut32(ctx->resp + 20, l->Handle);        /* the client may continue with this handle */
    }
    if (type < 1 || type > 7)
    {
        Fail(ctx, 0, ERROR_BAD_NUMBER);
        return;
    }
    if (!(RightsOnLock(ctx, l->Lock) & RIGHT_R))
    {
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    if (off + size > ctx->resplen)
        size = ctx->resplen > off ? ctx->resplen - off : 0;
    if (size < 64)
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    if (!ScanBlock(ctx, l, size, &type))
    {
        ScanEnd(ctx, l);
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    used = EfsPutExAll(ctx->resp + off, size, l->EacNext, type, &count);
    (void)used;
    EfsPut32(ctx->resp + 32, (ULONG)type);          /* Arg4: type actually used */
    EfsPut32(ctx->resp + 36, count);                /* Arg5: entries in this block */
    if (l->EacMore)
        SetRes(ctx, -1, 0);
    else
    {
        LONG err = IoErr();
        SetRes(ctx, 0, (err == 0 || count) ? ERROR_NO_MORE_ENTRIES : err);
        ScanEnd(ctx, l);
    }
    ctx->t->trans_RespDataActual = off + size;
}

/* EXAMINE_NEXT (24): one entry per call out of the same scan state */
static void ActExNext(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct LockRec *l;
    BPTR lock;
    ULONG off = ARG(2);
    LONG type = ED_OWNER;
    struct ExAllData *ed;
    struct FileInfoBlock fib;

    if (!(lock = DirOf(ctx, ARG(1), &l)) || !l)
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    if (!(RightsOnLock(ctx, l->Lock) & RIGHT_R))
    {
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    if (off + 260 > ctx->resplen)
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    if (!l->EacNext)
    {
        if (l->Eac && !l->EacMore)
        {
            ScanEnd(ctx, l);
            Fail(ctx, 0, ERROR_NO_MORE_ENTRIES);
            return;
        }
        if (!ScanBlock(ctx, l, 2048, &type) || !l->EacNext)
        {
            ScanEnd(ctx, l);
            Fail(ctx, 0, ERROR_NO_MORE_ENTRIES);
            return;
        }
    }
    ed = l->EacNext;
    l->EacNext = ed->ed_Next;
    memset(&fib, 0, sizeof(fib));
    fib.fib_DirEntryType = fib.fib_EntryType = ed->ed_Type;
    EfsPutCStr((UBYTE *)fib.fib_FileName, sizeof(fib.fib_FileName), ed->ed_Name);
    fib.fib_Size = ed->ed_Size;
    fib.fib_NumBlocks = ((ed->ed_Size + 511) >> 9) + 1;
    fib.fib_Protection = ed->ed_Prot;
    fib.fib_Date.ds_Days = ed->ed_Days;
    fib.fib_Date.ds_Minute = ed->ed_Mins;
    fib.fib_Date.ds_Tick = ed->ed_Ticks;
    if (l->EacType >= ED_COMMENT && ed->ed_Comment)
        EfsPutCStr((UBYTE *)fib.fib_Comment, sizeof(fib.fib_Comment), ed->ed_Comment);
    if (l->EacType >= ED_OWNER)
    {
        fib.fib_OwnerUID = ed->ed_OwnerUID;
        fib.fib_OwnerGID = ed->ed_OwnerGID;
    }
    fib.fib_DiskKey = LockKey(l->Lock, ed->ed_Name);
    if (ctx->m->Flags & MNTF_FULLSEC)
        fib.fib_Protection = AuthRewriteProtection(srv, ctx->m, &fib);
    if ((ctx->cflags & MNTF_PROTDISKINFO) && IsRoot(ctx, l->Lock) && !Stricmp(fib.fib_FileName, "Disk.info"))
        fib.fib_Protection = (fib.fib_Protection | FIBF_WRITE | FIBF_DELETE) & ~(FIBF_GRP_WRITE | FIBF_GRP_DELETE | FIBF_OTR_WRITE | FIBF_OTR_DELETE);
    EfsPutFIB(ctx->resp + off, &fib);
    SetRes(ctx, -1, 0);
    ctx->t->trans_RespDataActual = off + 260;
}

/* ---- the actions --------------------------------------------------------------- */

static void ActLocate(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old, lock;
    char rel[256];
    int wb;

    if (!ResolveName(ctx, ARG(1), ARG(2), &dir, rel, sizeof(rel)))
        return;
    wb = WBRule(ctx, dir, rel);
    if (wb == WB_BACKDROP)
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    if (!(RightsOnLock(ctx, dir) & RIGHT_E))
    {
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    old = CurrentDir(dir);
    lock = Lock(rel, SHARED_LOCK);                  /* always shared on the server (§2.3) */
    CurrentDir(old);
    if (!lock)
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    if (ctx->m->Flags & MNTF_FULLSEC)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        BOOL denied = FALSE;
        if (fib)
        {
            if (Examine(lock, fib) && fib->fib_DirEntryType > 0 && !IsRoot(ctx, lock) && !(AuthRights(srv, ctx->m, fib, NULL) & RIGHT_E))
                denied = TRUE;
            FreeDosObject(DOS_FIB, fib);
        }
        if (denied)
        {
            UnLock(lock);
            Fail(ctx, 0, ERROR_READ_PROTECTED);
            return;
        }
    }
    AnswerLock(ctx, lock, wb == WB_DISKINFO, TRUE);
}

static void ActCreateDir(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old, lock;
    char rel[256];

    if (ctx->m->Flags & MNTF_READONLY)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!ResolveName(ctx, ARG(1), ARG(2), &dir, rel, sizeof(rel)))
        return;
    if (WBRule(ctx, dir, rel) != WB_NONE || !(RightsOnLock(ctx, dir) & RIGHT_W))
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    old = CurrentDir(dir);
    lock = CreateDir(rel);
    if (lock)
    {
        UnLock(lock);                               /* CreateDir() gives an exclusive lock; the server hands out shared ones */
        lock = Lock(rel, SHARED_LOCK);
    }
    CurrentDir(old);
    if (!lock)
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    NewObjectAttrs(ctx, dir, rel);
    Changed(ctx, dir, rel);
    AnswerLock(ctx, lock, FALSE, TRUE);
}

static void ActCopyDir(struct Ctx *ctx, BOOL fromfh)
{
    struct FSServer *srv = ctx->srv;
    BPTR lock, dup;
    struct LockRec *l = NULL;

    if (fromfh)
    {
        struct FileRec *f = FindFile(ctx->m, ARG(1));
        if (!f)
        {
            Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
            return;
        }
        dup = DupLockFromFH(f->FH);
    }
    else
    {
        if (!(lock = DirOf(ctx, ARG(1), &l)))
        {
            Fail(ctx, 0, ERROR_INVALID_LOCK);
            return;
        }
        dup = DupLock(lock);
    }
    if (!dup)
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    AnswerLock(ctx, dup, l ? l->ReadOnly : FALSE, FALSE);
}

static void ActParent(struct Ctx *ctx, BOOL fromfh)
{
    struct FSServer *srv = ctx->srv;
    BPTR lock, parent;

    if (fromfh)
    {
        struct FileRec *f = FindFile(ctx->m, ARG(1));
        if (!f)
        {
            Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
            return;
        }
        parent = ParentOfFH(f->FH);
    }
    else
    {
        if (!(lock = DirOf(ctx, ARG(1), NULL)))
        {
            Fail(ctx, 0, ERROR_INVALID_LOCK);
            return;
        }
        if (IsRoot(ctx, lock))
        {
            Fail(ctx, 0, 0);                        /* parent of the export root (§2.3) */
            return;
        }
        parent = ParentDir(lock);
    }
    if (!parent)
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    if (!(RightsOnLock(ctx, parent) & RIGHT_E))
    {
        UnLock(parent);
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    AnswerLock(ctx, parent, FALSE, FALSE);
}

static void ActFreeLock(struct Ctx *ctx)
{
    struct LockRec *l;

    if (ARG(1) == 0)
    {
        SetRes(ctx, -1, 0);
        return;
    }
    if (!(l = FindLock(ctx->m, ARG(1))))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    LockRecFree(ctx->srv, l);
    SetRes(ctx, -1, 0);
}

static void ActSameLock(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR l1, l2;

    if (!(l1 = DirOf(ctx, ARG(1), NULL)) || !(l2 = DirOf(ctx, ARG(2), NULL)))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    /* the packet answers DOSTRUE for the same object and DOSFALSE otherwise; SameLock()'s
       LOCK_SAME_VOLUME (1) must not reach the client, which reads any non-zero as "same" */
    SetRes(ctx, SameLock(l1, l2) == LOCK_SAME ? DOSTRUE : DOSFALSE, 0);
}

static void ActDelete(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old;
    char rel[256];
    BOOL ok;

    if (ctx->m->Flags & MNTF_READONLY)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!ResolveName(ctx, ARG(1), ARG(2), &dir, rel, sizeof(rel)))
        return;
    if (WBRule(ctx, dir, rel) != WB_NONE)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!(RightsOnName(ctx, dir, rel, NULL) & RIGHT_D))
    {
        Fail(ctx, 0, ERROR_DELETE_PROTECTED);
        return;
    }
    old = CurrentDir(dir);
    ok = DeleteFile(rel);
    CurrentDir(old);
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
    if (ok)
        Changed(ctx, dir, rel);
}

static void ActRename(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir1, dir2;
    char rel1[256], rel2[256];
    BOOL ok;

    if (ctx->m->Flags & MNTF_READONLY)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!ResolveName(ctx, ARG(1), ARG(2), &dir1, rel1, sizeof(rel1)) ||
        !ResolveName(ctx, ARG(3), ARG(4), &dir2, rel2, sizeof(rel2)))
        return;
    if (SameLock(dir1, dir2) == LOCK_DIFFERENT)
    {
        Fail(ctx, 0, ERROR_RENAME_ACROSS_DEVICES);
        return;
    }
    if (WBRule(ctx, dir1, rel1) != WB_NONE || WBRule(ctx, dir2, rel2) != WB_NONE ||
        !(RightsOnName(ctx, dir1, rel1, NULL) & RIGHT_W) || !(RightsOnLock(ctx, dir2) & RIGHT_W))
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    FullPath(ctx, dir1, rel1, srv->Path, sizeof(srv->Path));
    FullPath(ctx, dir2, rel2, srv->Path2, sizeof(srv->Path2));
    ok = Rename(srv->Path, srv->Path2);
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
    if (ok)
    {
        ServerNotifyChanged(srv, ctx->m, srv->Path);
        ServerNotifyChanged(srv, ctx->m, srv->Path2);
    }
}

/* SET_PROTECT, SET_COMMENT, SET_DATE, SET_OWNER: Arg2 lock, Arg3 name, Arg4 value (§2.5) */
static void ActSetAttr(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old;
    char rel[256], comment[82];
    struct DateStamp ds;
    BOOL ok = FALSE;

    if (ctx->m->Flags & MNTF_READONLY)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!ResolveName(ctx, ARG(2), ARG(3), &dir, rel, sizeof(rel)))
        return;
    if (WBRule(ctx, dir, rel) != WB_NONE)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (ctx->act == ACTION_SET_PROTECT || ctx->act == ACTION_SET_OWNER)
    {
        if (!MayChangeAttrs(ctx, dir, rel))
        {
            Fail(ctx, 0, ERROR_WRITE_PROTECTED);
            return;
        }
    }
    else if (!(RightsOnName(ctx, dir, rel, NULL) & RIGHT_W))
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    old = CurrentDir(dir);
    switch (ctx->act)
    {
    case ACTION_SET_PROTECT:
    {
        ULONG mask = ARG(4);
        if (!(ctx->m->Authenticated && ctx->m->Uid == EFS_NOUSER))
            mask &= 0x3FFFFFFF;                     /* set-uid bits only for root (architecture §5.2) */
        ok = SetProtection(rel, mask);
        break;
    }
    case ACTION_SET_COMMENT:
        EfsGetBSTR(ctx->req, ctx->reqlen, ARG(4), comment, sizeof(comment));
        ok = SetComment(rel, comment);
        break;
    case ACTION_SET_DATE:
        if (ARG(4) + 12 <= ctx->reqlen)
        {
            ds.ds_Days = EfsGet32(ctx->req + ARG(4));
            ds.ds_Minute = EfsGet32(ctx->req + ARG(4) + 4);
            ds.ds_Tick = EfsGet32(ctx->req + ARG(4) + 8);
            ok = SetFileDate(rel, &ds);
        }
        else
            SetIoErr(ERROR_BAD_NUMBER);
        break;
    case ACTION_SET_OWNER:
        ok = SetOwner(rel, ARG(4));
        break;
    }
    CurrentDir(old);
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
    if (ok)
        Changed(ctx, dir, rel);
}

static void ActMakeLink(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old, target;
    char rel[256];
    BOOL ok;

    if (ARG(4) != 0)
    {
        Fail(ctx, 0, ERROR_ACTION_NOT_KNOWN);       /* soft links are refused (§2.5) */
        return;
    }
    if (ctx->m->Flags & MNTF_READONLY)
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!ResolveName(ctx, ARG(1), ARG(2), &dir, rel, sizeof(rel)))
        return;
    if (!(target = DirOf(ctx, ARG(3), NULL)))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    if (!(RightsOnLock(ctx, dir) & RIGHT_W))
    {
        Fail(ctx, 0, ERROR_WRITE_PROTECTED);
        return;
    }
    old = CurrentDir(dir);
    ok = MakeLink(rel, (SIPTR)target, FALSE);
    CurrentDir(old);
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
}

static void ActExamine(struct Ctx *ctx, BOOL fromfh)
{
    struct FSServer *srv = ctx->srv;
    struct FileInfoBlock *fib;
    BPTR lock = BNULL;
    struct LockRec *l = NULL;
    struct FileRec *f = NULL;
    ULONG off = ARG(2);
    BOOL ok;

    if (fromfh)
    {
        if (!(f = FindFile(ctx->m, ARG(1))))
        {
            Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
            return;
        }
    }
    else if (!(lock = DirOf(ctx, ARG(1), &l)))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    if (off + 260 > ctx->resplen)
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    if (l)
        ScanEnd(ctx, l);                            /* an Examine restarts any scan on this lock */
    if (!(fib = AllocDosObject(DOS_FIB, NULL)))
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    ok = fromfh ? ExamineFH(f->FH, fib) : Examine(lock, fib);
    if (!ok)
    {
        Fail(ctx, 0, IoErr());
        FreeDosObject(DOS_FIB, fib);
        return;
    }
    if (fromfh)
    {
        char path[512];
        fib->fib_DiskKey = PathKey(NameFromFH(f->FH, path, sizeof(path)) ? path : "", NULL);
    }
    else
        fib->fib_DiskKey = LockKey(lock, NULL);
    if (!fromfh && IsRoot(ctx, lock))
    {
        fib->fib_DirEntryType = fib->fib_EntryType = ST_ROOT;
        EfsPutCStr((UBYTE *)fib->fib_FileName, sizeof(fib->fib_FileName), ctx->m->Export->VolName);
    }
    else if (ctx->m->Flags & MNTF_FULLSEC)
        fib->fib_Protection = AuthRewriteProtection(srv, ctx->m, fib);
    if (l && l->ReadOnly)
        fib->fib_Protection = (fib->fib_Protection | FIBF_WRITE | FIBF_DELETE) & ~(FIBF_GRP_WRITE | FIBF_GRP_DELETE | FIBF_OTR_WRITE | FIBF_OTR_DELETE);
    EfsPutFIB(ctx->resp + off, fib);
    FreeDosObject(DOS_FIB, fib);
    SetRes(ctx, -1, 0);
    ctx->t->trans_RespDataActual = off + 260;
}

static void ActInfo(struct Ctx *ctx, BOOL disk)
{
    struct FSServer *srv = ctx->srv;
    struct InfoData *id;
    BPTR lock;
    ULONG off = disk ? ARG(1) : ARG(2);

    if (off + 36 > ctx->resplen)
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    lock = disk ? ctx->m->Export->RootLock : DirOf(ctx, ARG(1), NULL);
    if (!lock)
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    if (!(id = AllocVec(sizeof(struct InfoData), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    if (Info(lock, id))
    {
        if ((ctx->m->Flags & MNTF_READONLY) && id->id_DiskState == ID_VALIDATED)
            id->id_DiskState = ID_WRITE_PROTECTED;
    }
    else
    {
        memset(id, 0, sizeof(*id));                 /* no volume: the synthetic block (§2.7) */
        id->id_DiskState = ID_VALIDATING;
        id->id_NumBlocks = 1;
        id->id_NumBlocksUsed = 1;
        id->id_BytesPerBlock = 512;
        id->id_DiskType = ID_NO_DISK_PRESENT;
    }
    EfsPutInfoData(ctx->resp + off, id);
    FreeVec(id);
    SetRes(ctx, -1, 0);
    ctx->t->trans_RespDataActual = off + 36;
}

/* answer with a new file handle, the volume ID and the path (§2.4) */
static void AnswerFile(struct Ctx *ctx, BPTR fh, BPTR dir, CONST_STRPTR rel, BOOL readonly, BOOL noread)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    BPTR old, lock;
    ULONG n;

    if (!(f = AllocVec(sizeof(struct FileRec), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        Close(fh);
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    f->Handle = ServerNewHandle(srv);
    f->FH = fh;
    f->ReadOnly = readonly;
    f->NoRead = noread;
    FullPath(ctx, dir, rel, f->Name, sizeof(f->Name));
    AddTail((struct List *)&ctx->m->Files, (struct Node *)f);
    old = CurrentDir(dir);
    if ((lock = Lock(rel, SHARED_LOCK)))
    {
        VolPath(ctx, lock, srv->Path2, sizeof(srv->Path2));
        UnLock(lock);
    }
    else
        VolPath(ctx, dir, srv->Path2, sizeof(srv->Path2));
    CurrentDir(old);
    n = strlen(srv->Path2) + 1;
    if (EFS_HDR + n > ctx->resplen)
        n = ctx->resplen - EFS_HDR;
    CopyMem(srv->Path2, ctx->resp + EFS_HDR, n);
    ctx->resp[EFS_HDR + n - 1] = '\0';
    SetRes(ctx, (LONG)f->Handle, (LONG)ctx->m->Export->VolumeID);
    ctx->t->trans_RespDataActual = EFS_HDR + n;
}

static void ActOpen(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BPTR dir, old, fh;
    char rel[256];
    ULONG act = ctx->act, rights;
    BOOL exists, readonly = FALSE, noread = FALSE;
    int wb;

    if (!ResolveName(ctx, ARG(2), ARG(3), &dir, rel, sizeof(rel)))
        return;
    rights = RightsOnName(ctx, dir, rel, &exists);
    if (act == ACTION_FINDUPDATE)
        act = exists ? ACTION_FINDINPUT : ACTION_FINDOUTPUT;
    wb = WBRule(ctx, dir, rel);
    if (act == ACTION_FINDOUTPUT)
    {
        if ((ctx->m->Flags & MNTF_READONLY) || wb != WB_NONE)
        {
            Fail(ctx, 0, ERROR_WRITE_PROTECTED);
            return;
        }
        if (!(RightsOnLock(ctx, dir) & RIGHT_E))
        {
            Fail(ctx, 0, ERROR_READ_PROTECTED);
            return;
        }
        if (exists && !(rights & RIGHT_D))
        {
            Fail(ctx, 0, ERROR_DELETE_PROTECTED);
            return;
        }
        old = CurrentDir(dir);
        fh = Open(rel, MODE_NEWFILE);
        if (fh)
        {
            Close(fh);
            NewObjectAttrs(ctx, dir, rel);
            fh = Open(rel, MODE_OLDFILE);           /* re-opened shared, as the original does */
        }
        CurrentDir(old);
        if (!fh)
        {
            Fail(ctx, 0, IoErr());
            return;
        }
        Changed(ctx, dir, rel);
        AnswerFile(ctx, fh, dir, rel, FALSE, FALSE);
        return;
    }
    /* FINDINPUT */
    if (wb == WB_BACKDROP)
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    if (!(rights & RIGHT_R))
    {
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    readonly = !(rights & RIGHT_W) || wb == WB_DISKINFO || (ctx->m->Flags & MNTF_READONLY);
    old = CurrentDir(dir);
    fh = Open(rel, MODE_OLDFILE);
    CurrentDir(old);
    if (!fh)
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    AnswerFile(ctx, fh, dir, rel, readonly, noread);
}

static void ActFHFromLock(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct LockRec *l;
    BPTR fh;
    ULONG rights;

    if (ARG(2) == 0 || !(l = FindLock(ctx->m, ARG(2))))
    {
        Fail(ctx, 0, ERROR_INVALID_LOCK);
        return;
    }
    rights = RightsOnLock(ctx, l->Lock);
    if (!(rights & RIGHT_R))
    {
        Fail(ctx, 0, ERROR_READ_PROTECTED);
        return;
    }
    VolPath(ctx, l->Lock, srv->Path2, sizeof(srv->Path2));
    if (!(fh = OpenFromLock(l->Lock)))
    {
        Fail(ctx, 0, IoErr());
        return;
    }
    l->Lock = BNULL;                                /* consumed by OpenFromLock() */
    LockRecFree(srv, l);
    {
        struct FileRec *f;
        ULONG n;
        if (!(f = AllocVec(sizeof(struct FileRec), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            Close(fh);
            Fail(ctx, 0, ERROR_NO_FREE_STORE);
            return;
        }
        f->Handle = ServerNewHandle(srv);
        f->FH = fh;
        f->ReadOnly = !(rights & RIGHT_W) || (ctx->m->Flags & MNTF_READONLY);
        NameFromFH(fh, f->Name, sizeof(f->Name));
        AddTail((struct List *)&ctx->m->Files, (struct Node *)f);
        n = strlen(srv->Path2) + 1;
        if (EFS_HDR + n > ctx->resplen)
            n = ctx->resplen - EFS_HDR;
        CopyMem(srv->Path2, ctx->resp + EFS_HDR, n);
        ctx->resp[EFS_HDR + n - 1] = '\0';
        SetRes(ctx, (LONG)f->Handle, (LONG)ctx->m->Export->VolumeID);
        ctx->t->trans_RespDataActual = EFS_HDR + n;
    }
}

static void ActEnd(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    BOOL dirty;
    char name[256];

    if (!(f = FindFile(ctx->m, ARG(1))))
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    dirty = f->Dirty;
    strcpy(name, f->Name);
    FileRecFree(srv, f);
    SetRes(ctx, -1, 0);
    if (dirty)
        ServerNotifyChanged(srv, ctx->m, name);
}

static void ActReadWrite(struct Ctx *ctx, BOOL write)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    ULONG len = ARG(3), pos = ARG(5);
    LONG r;

    if (!(f = FindFile(ctx->m, ARG(1))))
    {
        Fail(ctx, -1, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    if (write && f->ReadOnly)
    {
        Fail(ctx, -1, ERROR_WRITE_PROTECTED);
        return;
    }
    if (!write && f->NoRead)
    {
        Fail(ctx, -1, ERROR_READ_PROTECTED);
        return;
    }
    if ((ctx->m->Flags & MNTF_ABSPOS) && (LONG)pos != f->Pos)
    {
        if (Seek(f->FH, (LONG)pos, OFFSET_BEGINNING) < 0)
        {
            Fail(ctx, -1, IoErr());
            return;
        }
        f->Pos = (LONG)pos;
    }
    if (write)
    {
        if (EFS_HDR + len > ctx->reqlen)
            len = ctx->reqlen > EFS_HDR ? ctx->reqlen - EFS_HDR : 0;
        r = Write(f->FH, ctx->req + EFS_HDR, len);
        if (r > 0)
            f->Dirty = TRUE;
        ctx->t->trans_RespDataActual = EFS_HDR;
    }
    else
    {
        if (EFS_HDR + len > ctx->resplen)
            len = ctx->resplen - EFS_HDR;
        r = Read(f->FH, ctx->resp + EFS_HDR, len);
        ctx->t->trans_RespDataActual = EFS_HDR + (r > 0 ? r : 0);
    }
    if (r > 0)
        f->Pos += r;
    SetRes(ctx, r, r < 0 ? IoErr() : 0);
}

static void ActSeek(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    LONG r;

    if (!(f = FindFile(ctx->m, ARG(1))))
    {
        Fail(ctx, -1, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    r = Seek(f->FH, (LONG)ARG(2), (LONG)ARG(3));
    SetRes(ctx, r, r < 0 ? IoErr() : 0);
    if (r >= 0)
        f->Pos = Seek(f->FH, 0, OFFSET_CURRENT);
}

static void ActSetFileSize(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    LONG r;

    if (!(f = FindFile(ctx->m, ARG(1))))
    {
        Fail(ctx, -1, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    if (f->ReadOnly)
    {
        Fail(ctx, -1, ERROR_WRITE_PROTECTED);
        return;
    }
    r = SetFileSize(f->FH, (LONG)ARG(2), (LONG)ARG(3));
    SetRes(ctx, r, r < 0 ? IoErr() : 0);
    if (r >= 0)
    {
        f->Dirty = TRUE;
        if (f->Pos > r)
            f->Pos = r;
    }
}

static void ActChangeMode(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    BOOL ok;

    if (ARG(1) == 1)
    {
        struct FileRec *f = FindFile(ctx->m, ARG(2));
        if (!f)
        {
            Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
            return;
        }
        if (f->ReadOnly && (LONG)ARG(3) != SHARED_LOCK)
        {
            Fail(ctx, 0, ERROR_WRITE_PROTECTED);
            return;
        }
        ok = ChangeMode(CHANGE_FH, f->FH, (LONG)ARG(3));
    }
    else
    {
        struct LockRec *l = FindLock(ctx->m, ARG(2));
        if (!l)
        {
            Fail(ctx, 0, ERROR_INVALID_LOCK);
            return;
        }
        ok = ChangeMode(CHANGE_LOCK, l->Lock, (LONG)ARG(3));
    }
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
}

static void ActRecord(struct Ctx *ctx, BOOL lock)
{
    struct FSServer *srv = ctx->srv;
    struct FileRec *f;
    BOOL ok;

    if (!(f = FindFile(ctx->m, ARG(1))))
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    if (lock)
        ok = LockRecord(f->FH, ARG(2), ARG(3), ARG(4), ARG(5));
    else
        ok = UnLockRecord(f->FH, ARG(2), ARG(3));
    SetRes(ctx, ok ? -1 : 0, ok ? 0 : IoErr());
}

static void ActAddNotify(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    struct NotifyRec *n;
    char name[256], *colon;
    CONST_STRPTR rest;

    if (ARG(3) != ctx->m->Export->VolumeID)
    {
        Fail(ctx, 0, ERROR_NO_DISK);
        return;
    }
    if (!EfsGetCStr(ctx->req, ctx->reqlen, ARG(2), name, sizeof(name)) || !(n = AllocVec(sizeof(struct NotifyRec), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        Fail(ctx, 0, ERROR_NO_FREE_STORE);
        return;
    }
    rest = (colon = strrchr(name, ':')) ? colon + 1 : name;
    strcpy(n->Name, ctx->m->Export->RootPath);
    if (rest[0])
        AddPart(n->Name, rest, sizeof(n->Name));
    n->Handle = ServerNewHandle(srv);
    n->ClientKey = ARG(1);
    n->Mount = ctx->m;
    n->NR.nr_Name = n->Name;
    n->NR.nr_Flags = NRF_SEND_MESSAGE | (ARG(4) & NRF_NOTIFY_INITIAL);
    n->NR.nr_stuff.nr_Msg.nr_Port = srv->NotifyPort;
    n->NR.nr_UserData = (IPTR)n;
    n->Started = StartNotify(&n->NR);
    AddTail((struct List *)&ctx->m->Notifies, (struct Node *)n);
    if (!n->Started && (ARG(4) & NRF_NOTIFY_INITIAL))
    {
        BPTR l = Lock(n->Name, SHARED_LOCK);
        if (l)
        {
            UnLock(l);
            ServerSendEvent(srv, ctx->m, EFS_ACT_NOTIFYEVENT, n->ClientKey, FALSE);
        }
    }
    SetRes(ctx, (LONG)n->Handle, 0);
}

static void ActRemoveNotify(struct Ctx *ctx)
{
    struct NotifyRec *n;

    if (!(n = FindNotify(ctx->m, ARG(1))))
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    NotifyRecFree(ctx->srv, n);
    SetRes(ctx, -1, 0);
}

/* 20000-20003: name <-> ID through accounts.library (§2.10) */
static void ActAccounts(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    char name[64];
    ULONG err;

    if (!AccountsBase)
    {
        Fail(ctx, 0, ERROR_ACTION_NOT_KNOWN);
        return;
    }
    switch (ctx->act)
    {
    case EFS_ACT_USERNAME_TO_UID:
    case EFS_ACT_GROUPNAME_TO_GID:
        EfsGetCStr(ctx->req, ctx->reqlen, ARG(1), name, sizeof(name));
        if (ctx->act == EFS_ACT_USERNAME_TO_UID)
        {
            struct UserInfo *ui = AllocUserInfo();
            if (!ui) { Fail(ctx, 0, ERROR_NO_FREE_STORE); return; }
            err = NameToUser(name, ui);
            SetRes(ctx, err ? 0 : ui->ui_UserID, err);
            FreeUserInfo(ui);
        }
        else
        {
            struct GroupInfo *gi = AllocGroupInfo();
            if (!gi) { Fail(ctx, 0, ERROR_NO_FREE_STORE); return; }
            err = NameToGroup(name, gi);
            SetRes(ctx, err ? 0 : gi->gi_GroupID, err);
            FreeGroupInfo(gi);
        }
        break;
    case EFS_ACT_UID_TO_USERINFO:
    case EFS_ACT_GID_TO_GROUPINFO:
        if (ARG(2) + 40 > ctx->resplen)
        {
            Fail(ctx, 0, ERROR_NO_FREE_STORE);
            return;
        }
        if (ctx->act == EFS_ACT_UID_TO_USERINFO)
        {
            struct UserInfo *ui = AllocUserInfo();
            if (!ui) { Fail(ctx, 0, ERROR_NO_FREE_STORE); return; }
            err = IDToUser(ARG(1) & 0xFFFF, ui);
            if (!err)
                EfsPutUserInfo(ctx->resp + ARG(2), ui);
            FreeUserInfo(ui);
        }
        else
        {
            struct GroupInfo *gi = AllocGroupInfo();
            if (!gi) { Fail(ctx, 0, ERROR_NO_FREE_STORE); return; }
            err = IDToGroup(ARG(1) & 0xFFFF, gi);
            if (!err)
                EfsPutGroupInfo(ctx->resp + ARG(2), gi);
            FreeGroupInfo(gi);
        }
        SetRes(ctx, err ? 0 : -1, err);
        if (!err)
            ctx->t->trans_RespDataActual = ARG(2) + 40;
        break;
    }
}

/* 20004: log in again for the same export (§2.10) */
static void ActChangeLogin(struct Ctx *ctx)
{
    struct FSServer *srv = ctx->srv;
    char user[36], password[36];
    struct UserInfo ui;
    struct Export *e;
    BOOL authenticated, credfail;
    CONST_STRPTR match;

    EfsGetCStr(ctx->req, ctx->reqlen, ARG(1), user, sizeof(user));
    EfsGetCStr(ctx->req, ctx->reqlen, ARG(2), password, sizeof(password));
    e = ctx->m->Export;
    match = (e->Name[0] && e->Name[0] != ':' && !(e->Flags & EXPF_REMOVABLE)) ? e->Name : e->Path;
    if (AuthSelectExport(srv, match, user, password, &ui, &authenticated, &credfail) != e)
    {
        Fail(ctx, 0, ERROR_OBJECT_NOT_FOUND);
        return;
    }
    ctx->m->Authenticated = authenticated;
    ctx->m->Uid = ui.ui_UserID;
    ctx->m->Gid = ui.ui_PrimaryGroupID;
    ctx->m->AccFlags = ui.ui_Flags;
    EfsPutCStr((UBYTE *)ctx->m->User, sizeof(ctx->m->User), user);
    SetRes(ctx, -1, 0);
}

/* ---- dispatch ------------------------------------------------------------------ */

void ActionsHandle(struct FSServer *srv, struct Mount *m, struct Transaction *t)
{
    struct Ctx c, *ctx = &c;
    int i;

    c.srv = srv;
    c.m = m;
    c.t = t;
    c.req = t->trans_RequestData;
    c.resp = t->trans_ResponseData;
    c.reqlen = t->trans_ReqDataActual;
    c.resplen = t->trans_RespDataLength;
    c.act = EfsGet32(c.req + 8);
    for (i = 0; i < 6; i++)
        c.arg[i] = EfsGet32(c.req + 20 + 4 * i);
    c.cflags = m->Flags;
    switch (c.act)
    {
    case ACTION_LOCATE_OBJECT: case ACTION_FINDINPUT: case ACTION_FINDOUTPUT: case ACTION_FINDUPDATE:
    case ACTION_EXAMINE_OBJECT: case ACTION_EXAMINE_FH:
        c.cflags |= c.arg[3] & 0xFFFF;              /* Arg4: client flags */
        break;
    case ACTION_EXAMINE_ALL: case EFS_ACT_EXNEXT:
        c.cflags |= c.arg[4] & 0xFFFF;              /* Arg5 */
        break;
    }
    SetRes(ctx, 0, 122);
    t->trans_RespDataActual = EFS_HDR;

    switch (c.act)
    {
    case ACTION_LOCATE_OBJECT:  ActLocate(ctx); break;
    case ACTION_CREATE_DIR:     ActCreateDir(ctx); break;
    case ACTION_COPY_DIR:       ActCopyDir(ctx, FALSE); break;
    case ACTION_COPY_DIR_FH:    ActCopyDir(ctx, TRUE); break;
    case ACTION_PARENT:         ActParent(ctx, FALSE); break;
    case ACTION_PARENT_FH:      ActParent(ctx, TRUE); break;
    case ACTION_FREE_LOCK:      ActFreeLock(ctx); break;
    case ACTION_SAME_LOCK:      ActSameLock(ctx); break;
    case ACTION_DELETE_OBJECT:
    case EFS_ACT_DELETE:        ActDelete(ctx); break;
    case ACTION_RENAME_OBJECT:  ActRename(ctx); break;
    case ACTION_SET_PROTECT: case ACTION_SET_COMMENT: case ACTION_SET_DATE: case ACTION_SET_OWNER:
                                ActSetAttr(ctx); break;
    case ACTION_MAKE_LINK:      ActMakeLink(ctx); break;
    case ACTION_EXAMINE_OBJECT: ActExamine(ctx, FALSE); break;
    case ACTION_EXAMINE_FH:     ActExamine(ctx, TRUE); break;
    case ACTION_EXAMINE_NEXT:   ActExNext(ctx); break;
    case EFS_ACT_EXNEXT:
    case ACTION_EXAMINE_ALL:    ActScan(ctx, c.act == ACTION_EXAMINE_ALL); break;
    case ACTION_EXAMINE_ALL_END:
    {
        struct LockRec *l = FindLock(m, c.arg[0]);
        if (l)
            ScanEnd(ctx, l);
        SetRes(ctx, -1, 0);
        break;
    }
    case ACTION_DISK_INFO:      ActInfo(ctx, TRUE); break;
    case ACTION_INFO:           ActInfo(ctx, FALSE); break;
    case ACTION_FLUSH:          SetRes(ctx, -1, 0); break;
    case ACTION_FINDUPDATE: case ACTION_FINDINPUT: case ACTION_FINDOUTPUT:
                                ActOpen(ctx); break;
    case ACTION_FH_FROM_LOCK:   ActFHFromLock(ctx); break;
    case ACTION_END:            ActEnd(ctx); break;
    case ACTION_READ:           ActReadWrite(ctx, FALSE); break;
    case ACTION_WRITE:          ActReadWrite(ctx, TRUE); break;
    case ACTION_SEEK:           ActSeek(ctx); break;
    case ACTION_SET_FILE_SIZE:  ActSetFileSize(ctx); break;
    case ACTION_CHANGE_MODE:    ActChangeMode(ctx); break;
    case ACTION_LOCK_RECORD:    ActRecord(ctx, TRUE); break;
    case ACTION_FREE_RECORD:    ActRecord(ctx, FALSE); break;
    case ACTION_ADD_NOTIFY:     ActAddNotify(ctx); break;
    case ACTION_REMOVE_NOTIFY:  ActRemoveNotify(ctx); break;
    case EFS_ACT_USERNAME_TO_UID: case EFS_ACT_GROUPNAME_TO_GID:
    case EFS_ACT_UID_TO_USERINFO: case EFS_ACT_GID_TO_GROUPINFO:
                                ActAccounts(ctx); break;
    case EFS_ACT_CHANGELOGIN:   ActChangeLogin(ctx); break;
    case EFS_ACT_PROBE:
    default:                    SetRes(ctx, 0, ERROR_ACTION_NOT_KNOWN); break;
    }
    FSLOG(srv, "mount 0x%lx action %lu -> %ld/%ld (%lu bytes)\n", (unsigned long)m->ID, (unsigned long)c.act,
          (long)(LONG)EfsGet32(c.resp + 12), (long)(LONG)EfsGet32(c.resp + 16), (unsigned long)t->trans_RespDataActual);
}
