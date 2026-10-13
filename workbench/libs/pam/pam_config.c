/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - per-service configuration

          SYS:Security/PAM/<service>, one module per line:

              <group> <control> <module> [argument ...]
              include <service>

          group:   auth | account | session | password
          control: required | requisite | sufficient | optional
          module:  a built-in (permit, deny, security, unixpw) or the name
                   of SYS:Libs/PAM/<name>.pam

          Without a file for the service, "other" is used; without that, a
          built-in default that depends on the state of security.library.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "pam_intern.h"

#define MAXLINE         512
#define MAXINCLUDE      8

static const char * const GroupNames[PAM_NUM_GROUPS] = { "auth", "account", "session", "password" };
static const char * const ControlNames[4] = { "required", "requisite", "sufficient", "optional" };

static LONG LookupWord(struct PamBase *PamBase, const char * const *names, LONG n, CONST_STRPTR word)
{
    LONG i;

    for (i = 0; i < n; i++)
        if (!Stricmp(names[i], word))
            return i;
    return -1;
}

/* Split a line into words in place; returns the count */
static LONG SplitWords(STRPTR line, STRPTR *words, LONG max)
{
    LONG n = 0;

    while (*line && n < max)
    {
        while (*line == ' ' || *line == '\t')
            line++;
        if (!*line || *line == '\n' || *line == '\r')
            break;
        words[n++] = line;
        while (*line && *line != ' ' && *line != '\t' && *line != '\n' && *line != '\r')
            line++;
        if (*line)
            *line++ = '\0';
    }
    return n;
}

static BOOL AddEntry(struct PamBase *PamBase, struct PamHandle *h, LONG group, LONG control, CONST_STRPTR module, LONG argc, STRPTR *argv)
{
    struct PamEntry *e;
    ULONG size = 0;
    LONG i;
    STRPTR p;

    if (!(e = AllocVec(sizeof(struct PamEntry), MEMF_CLEAR)))
        return FALSE;
    e->Group = group;
    e->Control = control;

    if (!(e->Builtin = PamFindBuiltin(module)))
    {
        if (!(e->Module = PamLoadModule(PamBase, module)))
        {
            D(bug(DEBUG_NAME_STR " %s: no module '%s'\n", __func__, module);)
            FreeVec(e);
            return FALSE;
        }
    }

    for (i = 0; i < argc; i++)
        size += strlen(argv[i]) + 1;
    if (argc)
    {
        if (!(e->Line = AllocVec(size + argc * sizeof(STRPTR), MEMF_CLEAR)))
        {
            if (e->Module)
                PamReleaseModule(PamBase, e->Module);
            FreeVec(e);
            return FALSE;
        }
        e->Argv = (CONST_STRPTR *)e->Line;
        p = e->Line + argc * sizeof(STRPTR);
        for (i = 0; i < argc; i++)
        {
            e->Argv[i] = p;
            strcpy(p, argv[i]);
            p += strlen(argv[i]) + 1;
        }
        e->Argc = argc;
    }
    AddTail((struct List *)&h->Stack[group], (struct Node *)e);
    return TRUE;
}

static BOOL ParseLine(struct PamBase *PamBase, struct PamHandle *h, STRPTR line, LONG depth);

/* LoadFile() results */
#define LOAD_OK         1
#define LOAD_NOFILE     0
#define LOAD_ERROR      -1      /* the file exists but could not be used: deny, do not fall back */

static LONG LoadFile(struct PamBase *PamBase, struct PamHandle *h, CONST_STRPTR service, LONG depth)
{
    char path[256];
    BPTR file;
    STRPTR line;
    LONG res = LOAD_OK;

    if (depth > MAXINCLUDE || strchr(service, ':') || strchr(service, '/'))
        return LOAD_ERROR;
    strcpy(path, PAM_CONFIG_DIR "/");
    strncat(path, service, sizeof(path) - strlen(path) - 1);

    if (!(file = Open(path, MODE_OLDFILE)))
        return LOAD_NOFILE;
    if ((line = AllocVec(MAXLINE, MEMF_ANY)))
    {
        while (res == LOAD_OK && FGets(file, line, MAXLINE - 1))
        {
            if (!ParseLine(PamBase, h, line, depth))
            {
                D(bug(DEBUG_NAME_STR " %s: bad line in '%s': %s\n", __func__, path, line);)
                res = LOAD_ERROR;
            }
        }
        FreeVec(line);
    }
    else
        res = LOAD_ERROR;
    Close(file);
    return res;
}

static BOOL ParseLine(struct PamBase *PamBase, struct PamHandle *h, STRPTR line, LONG depth)
{
    STRPTR words[32];
    LONG n, group, control;

    if (line[0] == '#' || line[0] == ';')
        return TRUE;
    n = SplitWords(line, words, 32);
    if (n == 0)
        return TRUE;
    if (!Stricmp(words[0], "include"))
        return (n == 2) ? (LoadFile(PamBase, h, words[1], depth + 1) == LOAD_OK) : FALSE;
    if (n < 3)
        return FALSE;
    group = LookupWord(PamBase, GroupNames, PAM_NUM_GROUPS, words[0]);
    control = LookupWord(PamBase, ControlNames, 4, words[1]);
    if (group < 0 || control < 0)
        return FALSE;
    return AddEntry(PamBase, h, group, control, words[2], n - 3, &words[3]);
}

/* The built-in default: what a machine does with no files at all */
static BOOL DefaultStack(struct PamBase *PamBase, struct PamHandle *h)
{
    CONST_STRPTR module;
    LONG g;

    switch (PamSecurityState(PamBase))
    {
    case PAM_STATE_CONFIGURED:      module = "security"; break;
    case PAM_STATE_UNCONFIGURED:    module = "deny";     break;
    default:                        module = "unixpw";   break;
    }
    for (g = 0; g < PAM_NUM_GROUPS; g++)
    {
        if (!AddEntry(PamBase, h, g, PAM_CTL_REQUIRED, module, 0, NULL))
            return FALSE;
    }
    return TRUE;
}

BOOL PamLoadStack(struct PamBase *PamBase, struct PamHandle *h)
{
    LONG res;

    if (h->StackLoaded)
        return TRUE;

    res = LoadFile(PamBase, h, h->Service, 0);
    if (res == LOAD_NOFILE)
        res = LoadFile(PamBase, h, PAM_DEFAULT_SERVICE, 0);
    if (res == LOAD_NOFILE)
        res = DefaultStack(PamBase, h) ? LOAD_OK : LOAD_ERROR;
    if (res != LOAD_OK)
    {
        /* A damaged service file denies everything rather than falling
         * back to a weaker default */
        PamFreeStack(PamBase, h);
        return FALSE;
    }
    h->StackLoaded = TRUE;
    return TRUE;
}

void PamFreeStack(struct PamBase *PamBase, struct PamHandle *h)
{
    struct PamEntry *e;
    LONG g;

    for (g = 0; g < PAM_NUM_GROUPS; g++)
    {
        while ((e = (struct PamEntry *)RemHead((struct List *)&h->Stack[g])))
        {
            if (e->Module)
                PamReleaseModule(PamBase, e->Module);
            if (e->Line)
                FreeVec(e->Line);
            FreeVec(e);
        }
    }
    h->StackLoaded = FALSE;
}
