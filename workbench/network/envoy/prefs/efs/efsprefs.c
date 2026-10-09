/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Exports - reading and writing EFS.prefs
          (re/spec/efs-protocol.md §6). The output has the layout the
          original editor writes: FORM PREF, a PRHD chunk of 6 zero bytes,
          then one VOLM chunk of 132 + 4*N bytes per export.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <string.h>

#include "efsprefs.h"
#include "misc.h"
#include <aros/debug.h>

struct MinList Exports;

static ULONG Get32(const UBYTE *p) { return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3]; }
static void Put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void CopyField(char *dst, const UBYTE *src)
{
    ULONG n = 0;
    while (n < EXP_NAMELEN - 1 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

void EfsPrefs_Init(void)
{
    NEWLIST((struct List *)&Exports);
}

static void FreeList(struct MinList *l)
{
    struct Node *n;
    while ((n = RemHead((struct List *)l)))
        FreeVec(n);
}

void EfsPrefs_Free(void)
{
    FreeList(&Exports);
}

struct Export *EfsPrefs_New(void)
{
    return AllocVec(sizeof(struct Export), MEMF_CLEAR);
}

/* the original editor forces Removable for an export without a usable name */
void EfsPrefs_Normalise(struct Export *e)
{
    if (!e->Name[0] || e->Name[0] == ':')
        e->Flags |= EXPF_REMOVABLE;
}

BOOL EfsPrefs_ImportFH(BPTR fh)
{
    struct MinList list;
    UBYTE head[12], *data;
    ULONG formlen, pos = 0;
    BOOL ok = FALSE;

    NEWLIST((struct List *)&list);
    if (Read(fh, head, 12) != 12 || memcmp(head, "FORM", 4) ||
        (memcmp(head + 8, "PREF", 4) && memcmp(head + 8, "EFSC", 4)))
        return FALSE;
    formlen = Get32(head + 4);
    if (formlen < 4 || formlen > 1024 * 1024 || !(data = AllocVec(formlen, MEMF_ANY)))
        return FALSE;
    formlen = Read(fh, data, formlen - 4) == (LONG)(formlen - 4) ? formlen - 4 : 0;
    if (formlen)
    {
        ok = TRUE;
        while (pos + 8 <= formlen)
        {
            ULONG clen = Get32(data + pos + 4);
            if (pos + 8 + clen > formlen)
                break;
            if (!memcmp(data + pos, "VOLM", 4) && clen >= 132)
            {
                struct Export *e = EfsPrefs_New();
                const UBYTE *c = data + pos + 8;
                ULONG i, n = (clen - 132) / 4;
                if (!e)
                {
                    ok = FALSE;
                    break;
                }
                CopyField(e->Path, c);
                CopyField(e->Name, c + 64);
                e->Flags = Get32(c + 128);
                for (i = 0; i < n && i < EXP_MAXACCESS; i++)
                {
                    e->AccessID[i] = (c[132 + 4 * i] << 8) | c[133 + 4 * i];
                    e->AccessGroup[i] = c[134 + 4 * i] ? 1 : 0;
                    e->NumAccess++;
                }
                AddTail((struct List *)&list, (struct Node *)e);
            }
            pos += 8 + clen + (clen & 1);
        }
    }
    FreeVec(data);
    if (ok)
    {
        FreeList(&Exports);
        while (!IsListEmpty((struct List *)&list))
            AddTail((struct List *)&Exports, RemHead((struct List *)&list));
    }
    else
        FreeList(&list);
    return ok;
}

BOOL EfsPrefs_ExportFH(BPTR fh)
{
    struct Export *e;
    ULONG total = 4 + 8 + 6, pos = 0;
    UBYTE *data;
    BOOL ok;

    ForeachNode(&Exports, e)
        total += 8 + 132 + 4 * e->NumAccess;
    if (!(data = AllocVec(total + 8, MEMF_CLEAR)))
        return FALSE;
    CopyMem("FORM", data, 4);
    Put32(data + 4, total);
    CopyMem("PREF", data + 8, 4);
    pos = 12;
    CopyMem("PRHD", data + pos, 4);
    Put32(data + pos + 4, 6);
    pos += 8 + 6;
    ForeachNode(&Exports, e)
    {
        ULONG i, clen = 132 + 4 * e->NumAccess;
        UBYTE *c = data + pos + 8;

        CopyMem("VOLM", data + pos, 4);
        Put32(data + pos + 4, clen);
        strncpy((char *)c, e->Path, EXP_NAMELEN - 1);
        strncpy((char *)c + 64, e->Name, EXP_NAMELEN - 1);
        Put32(c + 128, e->Flags);
        for (i = 0; i < e->NumAccess; i++)
        {
            c[132 + 4 * i] = e->AccessID[i] >> 8;
            c[133 + 4 * i] = e->AccessID[i];
            c[134 + 4 * i] = e->AccessGroup[i];
        }
        pos += 8 + clen;
    }
    ok = Write(fh, data, pos) == (LONG)pos;
    FreeVec(data);
    return ok;
}

static BOOL LoadFile(CONST_STRPTR name)
{
    BPTR fh;
    BOOL ok = FALSE;

    if ((fh = Open(name, MODE_OLDFILE)))
    {
        ok = EfsPrefs_ImportFH(fh);
        Close(fh);
    }
    return ok;
}

static BOOL SaveFile(CONST_STRPTR name)
{
    char dir[64];
    BPTR fh, lock;
    BOOL ok = FALSE;

    /* ENV:Envoy may not exist yet */
    strncpy(dir, name, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    *PathPart(dir) = '\0';
    if ((lock = Lock(dir, SHARED_LOCK)))
        UnLock(lock);
    else if ((lock = CreateDir(dir)))
        UnLock(lock);
    if ((fh = Open(name, MODE_NEWFILE)))
    {
        ok = EfsPrefs_ExportFH(fh);
        Close(fh);
    }
    return ok;
}

BOOL EfsPrefs_HandleArgs(CONST_STRPTR from, BOOL use, BOOL save)
{
    if (from)
    {
        if (!LoadFile(from))
        {
            ShowMessage("Cannot read the input file.");
            return FALSE;
        }
    }
    else if (!LoadFile(EFS_PREFS_ENV))
        LoadFile(EFS_PREFS_ENVARC);             /* none at all: no exports */

    if ((use || save) && !SaveFile(EFS_PREFS_ENV))
        ShowMessage("Cannot write " EFS_PREFS_ENV ".");
    if (save && !SaveFile(EFS_PREFS_ENVARC))
        ShowMessage("Cannot write " EFS_PREFS_ENVARC ".");
    return TRUE;
}
