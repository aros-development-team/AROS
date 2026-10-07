/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ServicesConfig - edits ENV:Envoy/services.prefs from a text file,
          in the batch form of the original Services Configuration editor:

          ServicesConfig CFGFILE/K,SAVE/S,LIST/S

          The file holds one directive per line:
              CLEARPREFS                      forget the existing services
              SERVICE "name" "path" [Active]  add or replace a service;
                                              without Active it is inactive
          SAVE writes ENVARC:Envoy/services.prefs and ENV:Envoy/services.prefs.
          LIST prints the configuration (after applying CFGFILE, if given).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "../services_private.h"
#include "prefsfile.h"

const char version[] = "$VER: ServicesConfig 50.0 (6.10.2026)";

#define TEMPLATE "CFGFILE/K,SAVE/S,LIST/S"

/* next token: a quoted string or a run of non-blanks; returns NULL at the end */
static char *Token(char **cursor)
{
    char *p = *cursor, *start;

    while (*p == ' ' || *p == '\t')
        p++;
    if (!*p || *p == '\n' || *p == '\r')
        return NULL;
    if (*p == '"')
    {
        start = ++p;
        while (*p && *p != '"' && *p != '\n' && *p != '\r')
            p++;
    }
    else
    {
        start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            p++;
    }
    if (*p)
        *p++ = '\0';
    *cursor = p;
    return start;
}

static BOOL ApplyFile(CONST_STRPTR filename, struct List *entries)
{
    BPTR fh;
    char line[512];
    LONG lineno = 0;
    BOOL ok = TRUE;

    if (!(fh = Open(filename, MODE_OLDFILE)))
    {
        Printf("ServicesConfig: cannot open %s\n", filename);
        return FALSE;
    }
    while (FGets(fh, line, sizeof(line)))
    {
        char *cursor = line, *word;

        lineno++;
        if (!(word = Token(&cursor)) || word[0] == ';' || word[0] == '#')
            continue;
        if (!Stricmp(word, "CLEARPREFS"))
            FreePrefsEntries(entries);
        else if (!Stricmp(word, "SERVICE"))
        {
            char *name = Token(&cursor), *path = Token(&cursor), *flag;
            BOOL active = FALSE;
            struct PrefsEntry *e;

            if (!name || !path || !name[0] || !path[0])
            {
                Printf("ServicesConfig: line %lu: SERVICE needs a name and a path\n", (ULONG)lineno);
                ok = FALSE;
                continue;
            }
            while ((flag = Token(&cursor)))
            {
                if (!Stricmp(flag, "ACTIVE"))
                    active = TRUE;
                else if (!Stricmp(flag, "INACTIVE"))
                    active = FALSE;
                else
                {
                    Printf("ServicesConfig: line %lu: unknown word '%s'\n", (ULONG)lineno, flag);
                    ok = FALSE;
                }
            }
            if ((e = FindPrefsEntry(entries, name)))
            {
                Remove(&e->pe_Node);
                FreeVec(e);
            }
            if (!AddPrefsEntry(entries, name, path, active))
                ok = FALSE;
        }
        else
        {
            Printf("ServicesConfig: line %lu: unknown directive '%s'\n", (ULONG)lineno, word);
            ok = FALSE;
        }
    }
    Close(fh);
    return ok;
}

int main(void)
{
    IPTR args[3] = { 0 };
    struct RDArgs *rda;
    struct List entries;
    int rc = RETURN_OK;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "ServicesConfig");
        return RETURN_FAIL;
    }
    NEWLIST(&entries);
    if (ReadServicesPrefs(SERVICES_PREFS_ENV, &entries) < 0)
        ReadServicesPrefs(SERVICES_PREFS_ENVARC, &entries);

    if (args[0] && !ApplyFile((CONST_STRPTR)args[0], &entries))
        rc = RETURN_WARN;
    if (args[1])
    {
        if (!WriteServicesPrefs(SERVICES_PREFS_ENVARC, &entries) || !WriteServicesPrefs(SERVICES_PREFS_ENV, &entries))
        {
            PutStr("ServicesConfig: cannot write the preferences\n");
            rc = RETURN_FAIL;
        }
    }
    if (args[2])
    {
        struct PrefsEntry *e;

        ForeachNode(&entries, e)
            Printf("SERVICE \"%s\" \"%s\"%s\n", e->pe_Name, e->pe_Path, e->pe_Active ? (CONST_STRPTR)" Active" : (CONST_STRPTR)"");
    }
    FreePrefsEntries(&entries);
    FreeArgs(rda);
    return rc;
}
