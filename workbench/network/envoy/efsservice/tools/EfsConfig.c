/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EfsConfig - the Envoy filesystem exports as a text file,
          written as ENVARC:Envoy/EFS.prefs and ENV:Envoy/EFS.prefs in
          the format of re/spec/efs-protocol.md §6 (FORM PREF, PRHD, one
          VOLM chunk per export).

    EfsConfig CFGFILE/K,SAVE/S,LIST/S

    CFGFILE lines:
        CLEARPREFS
        EXPORT "name" "path" [SNAPSHOT] [LEFTOUT] [MOUNTSFILES|NOSECURITY]
               [EMULATEEXALL] [READONLY] [REMOVABLE] [USER id ...] [GROUP id ...]
    Access entries are numeric Envoy IDs (what accounts.library reports).
    LIST prints the exports in the CFGFILE syntax; without arguments the
    command lists. A CFGFILE with bad lines is reported (return code 5) and
    not saved; one that cannot be read gives return code 10.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <string.h>

const char version[] = "$VER: EfsConfig 50.1 (7.10.2026)";

#define TEMPLATE        "CFGFILE/K,SAVE/S,LIST/S"
#define PREFS_ENV       "ENV:Envoy/EFS.prefs"
#define PREFS_ENVARC    "ENVARC:Envoy/EFS.prefs"
#define MAXACCESS       64

struct Entry
{
    struct Node node;
    char   path[64];
    char   name[64];
    ULONG  flags;
    ULONG  naccess;
    UWORD  accid[MAXACCESS];
    UBYTE  acckind[MAXACCESS];
};

static ULONG get32(const UBYTE *p) { return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3]; }
static void put32(UBYTE *p, ULONG v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void FreeEntries(struct List *l)
{
    struct Node *n;
    while ((n = RemHead(l)))
        FreeVec(n);
}

static BOOL ReadPrefs(CONST_STRPTR path, struct List *entries)
{
    BPTR fh;
    UBYTE *data;
    LONG size;
    ULONG pos = 12, formlen;

    if (!(fh = Open(path, MODE_OLDFILE)))
        return FALSE;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size < 12 || !(data = AllocVec(size, MEMF_ANY)))
    {
        Close(fh);
        return FALSE;
    }
    Read(fh, data, size);
    Close(fh);
    if (memcmp(data, "FORM", 4) || (memcmp(data + 8, "PREF", 4) && memcmp(data + 8, "EFSC", 4)))
    {
        FreeVec(data);
        return FALSE;
    }
    formlen = get32(data + 4) + 8;
    if (formlen > (ULONG)size)
        formlen = size;
    while (pos + 8 <= formlen)
    {
        ULONG clen = get32(data + pos + 4);
        if (pos + 8 + clen > formlen)
            break;
        if (!memcmp(data + pos, "VOLM", 4) && clen >= 132)
        {
            struct Entry *e = AllocVec(sizeof(struct Entry), MEMF_CLEAR);
            const UBYTE *c = data + pos + 8;
            ULONG i, n = (clen - 132) / 4;
            if (e)
            {
                strncpy(e->path, (const char *)c, 63);
                strncpy(e->name, (const char *)c + 64, 63);
                e->flags = get32(c + 128);
                for (i = 0; i < n && i < MAXACCESS; i++)
                {
                    e->accid[i] = (c[132 + 4 * i] << 8) | c[133 + 4 * i];
                    e->acckind[i] = c[134 + 4 * i];
                    e->naccess++;
                }
                e->node.ln_Name = e->name;
                AddTail(entries, &e->node);
            }
        }
        pos += 8 + clen + (clen & 1);
    }
    FreeVec(data);
    return TRUE;
}

static BOOL WritePrefs(CONST_STRPTR path, struct List *entries)
{
    struct Entry *e;
    ULONG total = 4 + 8 + 6, pos = 12;
    UBYTE *data;
    BPTR fh, lock;
    BOOL ok;
    char dir[256];

    ForeachNode(entries, e)
        total += 8 + 132 + 4 * e->naccess;
    if (!(data = AllocVec(total + 8, MEMF_CLEAR)))
        return FALSE;
    CopyMem("FORM", data, 4);
    put32(data + 4, total);
    CopyMem("PREF", data + 8, 4);
    CopyMem("PRHD", data + pos, 4);
    put32(data + pos + 4, 6);
    pos += 8 + 6;
    ForeachNode(entries, e)
    {
        ULONG i, clen = 132 + 4 * e->naccess;
        UBYTE *c = data + pos + 8;
        CopyMem("VOLM", data + pos, 4);
        put32(data + pos + 4, clen);
        strncpy((char *)c, e->path, 63);
        strncpy((char *)c + 64, e->name, 63);
        put32(c + 128, e->flags);
        for (i = 0; i < e->naccess; i++)
        {
            c[132 + 4 * i] = e->accid[i] >> 8;
            c[133 + 4 * i] = e->accid[i];
            c[134 + 4 * i] = e->acckind[i];
        }
        pos += 8 + clen;
    }
    /* make sure the drawer exists */
    strncpy(dir, path, sizeof(dir) - 1);
    *PathPart(dir) = '\0';
    if ((lock = CreateDir(dir)))
        UnLock(lock);
    ok = (fh = Open(path, MODE_NEWFILE)) != BNULL;
    if (ok)
    {
        ok = Write(fh, data, pos) == (LONG)pos;
        Close(fh);
    }
    FreeVec(data);
    return ok;
}

/* one word of a line: quoted or blank-delimited; returns the position after it */
static char *Word(char *p, char *out, ULONG size)
{
    ULONG n = 0;

    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '"')
    {
        p++;
        while (*p && *p != '"' && *p != '\n' && *p != '\r')
        {
            if (n < size - 1)
                out[n++] = *p;
            p++;
        }
        if (*p == '"')
            p++;
    }
    else
    {
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
        {
            if (n < size - 1)
                out[n++] = *p;
            p++;
        }
    }
    out[n] = '\0';
    return p;
}

/* RETURN_OK, RETURN_WARN (bad lines, reported) or RETURN_ERROR (file cannot be read) */
static int ReadConfig(CONST_STRPTR filename, struct List *entries)
{
    BPTR fh;
    char line[512], word[256];
    ULONG lineno = 0;
    int rc = RETURN_OK;

    if (!(fh = Open(filename, MODE_OLDFILE)))
    {
        Printf("EfsConfig: cannot open %s\n", filename);
        return RETURN_ERROR;
    }
    while (FGets(fh, line, sizeof(line)))
    {
        char *p = Word(line, word, sizeof(word));
        lineno++;
        if (!word[0] || word[0] == ';' || word[0] == '#')
            continue;
        if (!Stricmp(word, "CLEARPREFS"))
            FreeEntries(entries);
        else if (!Stricmp(word, "EXPORT"))
        {
            struct Entry *e = AllocVec(sizeof(struct Entry), MEMF_CLEAR), *old;
            int mode = 0;
            if (!e)
                break;
            p = Word(p, e->name, sizeof(e->name));
            p = Word(p, e->path, sizeof(e->path));
            if (!e->path[0])
            {
                Printf("EfsConfig: line %lu: EXPORT needs a name and a path\n", (ULONG)lineno);
                rc = RETURN_WARN;
                FreeVec(e);
                continue;
            }
            for (;;)
            {
                p = Word(p, word, sizeof(word));
                if (!word[0])
                    break;
                if (!Stricmp(word, "SNAPSHOT"))          e->flags |= 0x01;
                else if (!Stricmp(word, "LEFTOUT"))      e->flags |= 0x02;
                else if (!Stricmp(word, "MOUNTSFILES"))  e->flags |= 0x04;
                else if (!Stricmp(word, "NOSECURITY"))   e->flags |= 0x08;
                else if (!Stricmp(word, "EMULATEEXALL")) e->flags |= 0x10;
                else if (!Stricmp(word, "READONLY"))     e->flags |= 0x20;
                else if (!Stricmp(word, "REMOVABLE"))    e->flags |= 0x40;
                else if (!Stricmp(word, "USER"))         mode = 1;
                else if (!Stricmp(word, "GROUP"))        mode = 2;
                else
                {
                    LONG id;
                    if (mode && StrToLong(word, &id) > 0 && e->naccess < MAXACCESS)
                    {
                        e->accid[e->naccess] = id;
                        e->acckind[e->naccess] = mode == 2;
                        e->naccess++;
                    }
                    else
                    {
                        Printf("EfsConfig: line %lu: unknown word '%s'\n", (ULONG)lineno, word);
                        rc = RETURN_WARN;
                    }
                }
            }
            if (!e->name[0] || e->name[0] == ':')
                e->flags |= 0x40;                   /* the editor forces Removable for a nameless export */
            e->node.ln_Name = e->name;
            if (e->name[0] && (old = (struct Entry *)FindName(entries, e->name)))
            {
                Remove(&old->node);
                FreeVec(old);
            }
            AddTail(entries, &e->node);
        }
        else
        {
            Printf("EfsConfig: line %lu: unknown directive '%s'\n", (ULONG)lineno, word);
            rc = RETURN_WARN;
        }
    }
    Close(fh);
    return rc;
}

int main(void)
{
    IPTR args[3] = { 0 };
    struct RDArgs *rda;
    struct List entries;
    struct Entry *e;
    int rc = RETURN_OK;

    NEWLIST(&entries);
    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "EfsConfig");
        return RETURN_FAIL;
    }
    if (!ReadPrefs(PREFS_ENV, &entries))
        ReadPrefs(PREFS_ENVARC, &entries);
    if (args[0])
        rc = ReadConfig((CONST_STRPTR)args[0], &entries);
    if (args[1] && rc != RETURN_OK)
        PutStr("EfsConfig: not saved because of the errors above\n");
    else if (args[1])
    {
        if (!WritePrefs(PREFS_ENVARC, &entries) || !WritePrefs(PREFS_ENV, &entries))
        {
            PutStr("EfsConfig: cannot write the preferences\n");
            rc = RETURN_ERROR;
        }
    }
    if (args[2] || (!args[0] && !args[1]))           /* no arguments: list */
    {
        static const struct { ULONG bit; CONST_STRPTR word; } fw[] =
        {
            { 0x01, "SNAPSHOT" }, { 0x02, "LEFTOUT" }, { 0x04, "MOUNTSFILES" }, { 0x08, "NOSECURITY" },
            { 0x10, "EMULATEEXALL" }, { 0x20, "READONLY" }, { 0x40, "REMOVABLE" }, { 0, NULL }
        };
        ForeachNode(&entries, e)
        {
            ULONG i;
            /* in the input syntax, so the output can be fed back with CFGFILE */
            Printf("EXPORT \"%s\" \"%s\"", e->name, e->path);
            for (i = 0; fw[i].word; i++)
                if (e->flags & fw[i].bit)
                    Printf(" %s", fw[i].word);
            for (i = 0; i < e->naccess; i++)
                Printf(" %s %lu", e->acckind[i] ? (CONST_STRPTR)"GROUP" : (CONST_STRPTR)"USER", (ULONG)e->accid[i]);
            PutStr("\n");
        }
    }
    FreeEntries(&entries);
    FreeArgs(rda);
    return rc;
}
