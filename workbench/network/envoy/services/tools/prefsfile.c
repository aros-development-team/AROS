/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ENV:Envoy/services.prefs reading and writing
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <string.h>

#include "prefsfile.h"

#define ISVC_SIZE               328
#define ISVC_ACTIVE             0
#define ISVC_PATH               8
#define ISVC_SHORTPATH          229
#define ISVC_NAME               264
#define PRHD_SIZE               6

static ULONG Get32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

static void Put32(UBYTE *p, ULONG v)
{
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static void CopyField(char *dst, ULONG dstsize, const UBYTE *src, ULONG srcsize)
{
    ULONG n = 0;

    while (n < srcsize && n < dstsize - 1 && src[n])
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
}

struct PrefsEntry *FindPrefsEntry(struct List *entries, CONST_STRPTR name)
{
    struct PrefsEntry *e;

    ForeachNode(entries, e)
        if (!Stricmp(e->pe_Name, name))
            return e;
    return NULL;
}

struct PrefsEntry *AddPrefsEntry(struct List *entries, CONST_STRPTR name, CONST_STRPTR path, BOOL active)
{
    struct PrefsEntry *e;

    if (!(e = AllocVec(sizeof(struct PrefsEntry), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    strncpy(e->pe_Name, name, PREFS_NAMESIZE - 1);
    strncpy(e->pe_Path, path, PREFS_PATHSIZE - 1);
    e->pe_Active = active;
    e->pe_Node.ln_Name = e->pe_Name;
    AddTail(entries, &e->pe_Node);
    return e;
}

void FreePrefsEntries(struct List *entries)
{
    struct Node *n;

    while ((n = RemHead(entries)))
        FreeVec(n);
}

LONG ReadServicesPrefs(CONST_STRPTR filename, struct List *entries)
{
    BPTR fh;
    LONG size, got, count = 0;
    UBYTE *data;
    ULONG pos, formlen;

    if (!(fh = Open(filename, MODE_OLDFILE)))
        return -1;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size < 12 || !(data = AllocVec(size, MEMF_PUBLIC)))
    {
        Close(fh);
        return -1;
    }
    got = Read(fh, data, size);
    Close(fh);
    if (got != size || memcmp(data, "FORM", 4) || memcmp(data + 8, "PREF", 4))
    {
        FreeVec(data);
        return -1;
    }
    formlen = Get32(data + 4) + 8;
    if (formlen > (ULONG)size)
        formlen = size;
    for (pos = 12; pos + 8 <= formlen; )
    {
        ULONG len = Get32(data + pos + 4);
        const UBYTE *c = data + pos + 8;

        if (pos + 8 + len > formlen)
            break;
        if (!memcmp(data + pos, "ISVC", 4) && len >= ISVC_SIZE)
        {
            char name[PREFS_NAMESIZE], path[PREFS_PATHSIZE];

            CopyField(name, sizeof(name), c + ISVC_NAME, PREFS_NAMESIZE);
            CopyField(path, sizeof(path), c + ISVC_PATH, PREFS_PATHSIZE);
            if (name[0] && AddPrefsEntry(entries, name, path, Get32(c + ISVC_ACTIVE) ? TRUE : FALSE))
                count++;
        }
        pos += 8 + ((len + 1) & ~1UL);
    }
    FreeVec(data);
    return count;
}

static BOOL EnsureDirectory(CONST_STRPTR filename)
{
    char dir[256];
    STRPTR end;
    BPTR lock;

    strncpy(dir, filename, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    end = (STRPTR)PathPart(dir);
    if (end == (STRPTR)dir || *(end - 1) == ':')
        return TRUE;
    *end = '\0';
    if ((lock = Lock(dir, SHARED_LOCK)) || (lock = CreateDir(dir)))
    {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
}

BOOL WriteServicesPrefs(CONST_STRPTR filename, struct List *entries)
{
    struct PrefsEntry *e;
    ULONG count = 0, size, pos;
    UBYTE *data;
    BPTR fh;
    BOOL ok;

    ForeachNode(entries, e)
        count++;
    size = 12 + 8 + PRHD_SIZE + count * (8 + ISVC_SIZE);
    if (!(data = AllocVec(size, MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    CopyMem("FORM", data, 4);
    Put32(data + 4, size - 8);
    CopyMem("PREF", data + 8, 4);
    CopyMem("PRHD", data + 12, 4);
    Put32(data + 16, PRHD_SIZE);
    pos = 20 + PRHD_SIZE;
    ForeachNode(entries, e)
    {
        UBYTE *c = data + pos + 8;
        const char *colon = strchr(e->pe_Path, ':');

        CopyMem("ISVC", data + pos, 4);
        Put32(data + pos + 4, ISVC_SIZE);
        Put32(c + ISVC_ACTIVE, e->pe_Active ? 1 : 0);
        strncpy((char *)c + ISVC_PATH, e->pe_Path, PREFS_PATHSIZE - 1);
        strncpy((char *)c + ISVC_SHORTPATH, colon ? colon : e->pe_Path, PREFS_SHORTPATHSIZE - 1);
        strncpy((char *)c + ISVC_NAME, e->pe_Name, PREFS_NAMESIZE - 1);
        pos += 8 + ISVC_SIZE;
    }
    if (!EnsureDirectory(filename) || !(fh = Open(filename, MODE_NEWFILE)))
    {
        FreeVec(data);
        return FALSE;
    }
    ok = Write(fh, data, size) == (LONG)size;
    Close(fh);
    FreeVec(data);
    return ok;
}
