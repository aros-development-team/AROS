/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - the export list from ENV:Envoy/EFS.prefs
          (re/spec/efs-protocol.md §6): FORM PREF or the legacy FORM EFSC,
          one VOLM chunk per export. A reload keeps the volume IDs of
          exports whose path is unchanged, so that the mounts on them
          survive.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <string.h>

#include "fs_intern.h"

#define UtilityBase (srv->UtilLib)

static void ExportFree(struct FSServer *srv, struct Export *e)
{
    if (e->RootLock)
        UnLock(e->RootLock);
    FreeVec(e->AccId);
    FreeVec(e->AccKind);
    FreeVec(e);
}

void ConfigFree(struct FSServer *srv)
{
    struct Export *e;

    while ((e = (struct Export *)RemHead((struct List *)&srv->Exports)))
        ExportFree(srv, e);
}

/* the volume the export root lives on: its name and creation date */
static void ExportVolume(struct FSServer *srv, struct Export *e)
{
    struct FileInfoBlock *fib;
    struct FileLock *fl = BADDR(e->RootLock);

    DateStamp(&e->VolDate);
    if (fl && fl->fl_Volume)
    {
        struct DeviceList *dl = BADDR(fl->fl_Volume);
        e->VolDate = dl->dl_VolumeDate;
    }
    if (e->Name[0] && e->Name[0] != ':' && !(e->Flags & EXPF_REMOVABLE))
    {
        EfsPutCStr((UBYTE *)e->VolName, sizeof(e->VolName), e->Name);
        return;
    }
    strcpy(e->VolName, "Unnamed");
    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if (Examine(e->RootLock, fib) && fib->fib_FileName[0])
            EfsPutCStr((UBYTE *)e->VolName, sizeof(e->VolName), fib->fib_FileName);
        FreeDosObject(DOS_FIB, fib);
    }
}

static void ExportOpen(struct FSServer *srv, struct Export *e)
{
    e->RootLock = Lock(e->Path, SHARED_LOCK);
    if (!e->RootLock)
    {
        FSLOG(srv, "export '%s': cannot lock %s (%ld)\n", e->Name, e->Path, (long)IoErr());
        return;
    }
    e->IsFS = IsFileSystem(e->Path);
    if (!NameFromLock(e->RootLock, e->RootPath, sizeof(e->RootPath)))
        strcpy(e->RootPath, e->Path);
    ExportVolume(srv, e);
    FSLOG(srv, "export '%s' = %s (%s), flags 0x%lx, %lu access entries, volume '%s' id 0x%lx\n",
          e->Name, e->Path, e->RootPath, (unsigned long)e->Flags, (unsigned long)e->NumAccess, e->VolName, (unsigned long)e->VolumeID);
}

static void AddExport(struct FSServer *srv, struct MinList *list, const UBYTE *chunk, ULONG len)
{
    struct Export *e;
    ULONG i;

    if (len < 132 || !(e = AllocVec(sizeof(struct Export), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    EfsGetCStr(chunk, len, 0, e->Path, sizeof(e->Path));
    EfsGetCStr(chunk, len, 64, e->Name, sizeof(e->Name));
    e->Flags = EfsGet32(chunk + 128);
    e->NumAccess = (len - 132) / 4;
    if (e->NumAccess)
    {
        e->AccId = AllocVec(e->NumAccess * sizeof(UWORD), MEMF_PUBLIC);
        e->AccKind = AllocVec(e->NumAccess, MEMF_PUBLIC);
        if (!e->AccId || !e->AccKind)
        {
            ExportFree(srv, e);
            return;
        }
        for (i = 0; i < e->NumAccess; i++)
        {
            e->AccId[i] = EfsGet16(chunk + 132 + 4 * i);
            e->AccKind[i] = chunk[132 + 4 * i + 2];
        }
    }
    AddTail((struct List *)list, (struct Node *)e);
}

static BOOL ParseFile(struct FSServer *srv, struct MinList *list, const UBYTE *data, ULONG len)
{
    ULONG pos = 12, formlen;

    if (len < 12 || memcmp(data, "FORM", 4) || (memcmp(data + 8, "PREF", 4) && memcmp(data + 8, "EFSC", 4)))
        return FALSE;
    formlen = EfsGet32(data + 4) + 8;
    if (formlen > len)
        formlen = len;
    while (pos + 8 <= formlen)
    {
        ULONG clen = EfsGet32(data + pos + 4);
        if (pos + 8 + clen > formlen)
            break;
        if (!memcmp(data + pos, "VOLM", 4))
            AddExport(srv, list, data + pos + 8, clen);
        pos += 8 + clen + (clen & 1);
    }
    return TRUE;
}

static BOOL LoadFile(struct FSServer *srv, struct MinList *list, CONST_STRPTR path)
{
    BPTR fh;
    UBYTE *data;
    LONG size;
    BOOL ok = FALSE;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return FALSE;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size > 0 && size < 1024 * 1024 && (data = AllocVec(size, MEMF_PUBLIC)))
    {
        if (Read(fh, data, size) == size)
            ok = ParseFile(srv, list, data, size);
        FreeVec(data);
    }
    Close(fh);
    return ok;
}

struct Export *ConfigFindExport(struct FSServer *srv, CONST_STRPTR name)
{
    struct Export *e;

    ForeachNode(&srv->Exports, e)
    {
        /* the match name: the export name, or the path when there is no usable name (§5.2) */
        CONST_STRPTR match = (e->Name[0] && e->Name[0] != ':' && !(e->Flags & EXPF_REMOVABLE)) ? e->Name : e->Path;
        if (!Stricmp(match, name))
            return e;
    }
    return NULL;
}

void ConfigLoad(struct FSServer *srv)
{
    struct MinList fresh;
    struct Export *e, *old, *next;
    struct Mount *m;

    NEWLIST((struct List *)&fresh);
    if (!LoadFile(srv, &fresh, FS_PREFS_ENV))
        LoadFile(srv, &fresh, FS_PREFS_ENVARC);

    /* take over the volume identity of unchanged exports */
    ForeachNode(&fresh, e)
    {
        ForeachNode(&srv->Exports, old)
        {
            if (!Stricmp(old->Path, e->Path))
            {
                e->VolumeID = old->VolumeID;
                break;
            }
        }
        if (!e->VolumeID)
            e->VolumeID = srv->NextVolID++;
        ExportOpen(srv, e);
    }
    /* mounts follow their export by path; others lose it */
    ForeachNode(&srv->Mounts, m)
    {
        struct Export *found = NULL;
        if (m->Export)
        {
            ForeachNode(&fresh, e)
                if (!Stricmp(e->Path, m->Export->Path) && e->RootLock)
                    found = e;
        }
        m->Export = found;
    }
    ForeachNodeSafe(&srv->Exports, old, next)
    {
        Remove((struct Node *)old);
        ExportFree(srv, old);
    }
    while ((e = (struct Export *)RemHead((struct List *)&fresh)))
        AddTail((struct List *)&srv->Exports, (struct Node *)e);
}
