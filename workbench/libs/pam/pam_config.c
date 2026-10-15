/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - per-service configuration

          SYS:Security/PAM/<service>, one module per line:

              <group> <control> <module> [argument ...]
              include <service>

          group:   auth | account | session | password
          control: required | requisite | sufficient | optional
          module:  a built-in (permit, deny, security, unixpw, access) or the name
                   of SYS:Libs/PAM/<name>.pam

          '#' or ';' starts a comment anywhere a word could start.

          Without a file for the service, "other" is used; without that, a
          built-in default that depends on the state of security.library.
          A file that exists but cannot be used - a bad line, a line too
          long, a read error, a module that cannot be loaded, or on a
          configured multi-user system a file or directory not owned by
          root or writable by others - denies everything instead of
          falling back to a weaker default.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/exall.h>
#include <libraries/security.h>
#include <resources/log.h>
#include <string.h>

#include "pam_intern.h"

#define MAXLINE         512
#define MAXINCLUDE      8
#define MAXWORDS        33      /* group, control, module and 30 arguments */

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

/* Split a line into words in place; returns the count, or -1 if there
 * are more than max. A word starting with '#' or ';' ends the line. */
static LONG SplitWords(STRPTR line, STRPTR *words, LONG max)
{
    LONG n = 0;

    for (;;)
    {
        while (*line == ' ' || *line == '\t')
            line++;
        if (!*line || *line == '\n' || *line == '\r' || *line == '#' || *line == ';')
            break;
        if (n == max)
            return -1;
        words[n++] = line;
        while (*line && *line != ' ' && *line != '\t' && *line != '\n' && *line != '\r')
            line++;
        if (*line)
            *line++ = '\0';
    }
    return n;
}

/*
 * May a configuration file or module be trusted? On a configured
 * multi-user system it must belong to root, as must its directory, and
 * neither may be writable by group or others; otherwise anybody could
 * add "auth sufficient permit". Without security.library, or before it
 * is configured, there is no ownership to check.
 */
static BOOL TrustedObject(struct PamBase *PamBase, BPTR lock, BOOL file)
{
    struct FileInfoBlock *fib;
    BOOL ok = FALSE;

    if (!(fib = AllocDosObject(DOS_FIB, NULL)))
        return FALSE;
    if (Examine(lock, fib))
    {
        ok = (fib->fib_OwnerUID == secROOT_UID) &&
             !(fib->fib_Protection & (FIBF_GRP_WRITE | FIBF_OTR_WRITE)) &&
             (file ? fib->fib_DirEntryType < 0 : fib->fib_DirEntryType > 0);
    }
    FreeDosObject(DOS_FIB, fib);
    return ok;
}

BOOL PamTrusted(struct PamBase *PamBase, BPTR lock, BOOL file)
{
    BPTR dir;
    BOOL ok;

    if (PamSecurityState(PamBase) != PAM_STATE_CONFIGURED)
        return TRUE;
    if (!TrustedObject(PamBase, lock, file))
        return FALSE;
    if (!(dir = ParentDir(lock)))
        return FALSE;
    ok = TrustedObject(PamBase, dir, FALSE);
    UnLock(dir);
    return ok;
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

static BOOL ParseLine(struct PamBase *PamBase, struct PamHandle *h, STRPTR line, CONST_STRPTR service, LONG depth);

/* LoadFile() results */
#define LOAD_OK         1
#define LOAD_NOFILE     0
#define LOAD_ERROR      -1      /* the file exists but could not be used: deny, do not fall back */

static LONG LoadFile(struct PamBase *PamBase, struct PamHandle *h, CONST_STRPTR service, LONG depth)
{
    char path[256];
    BPTR file, lock;
    STRPTR line;
    LONG res = LOAD_OK, len;

    if (depth > MAXINCLUDE || !service || !service[0] || strchr(service, ':') || strchr(service, '/'))
        return LOAD_ERROR;
    strcpy(path, PAM_CONFIG_DIR "/");
    strncat(path, service, sizeof(path) - strlen(path) - 1);

    if (!(lock = Lock(path, SHARED_LOCK)))
        return (IoErr() == ERROR_OBJECT_NOT_FOUND) ? LOAD_NOFILE : LOAD_ERROR;
    if (!PamTrusted(PamBase, lock, TRUE))
    {
        UnLock(lock);
        PamLogA(PamBase, h, LOGF_Flag_Type_Error, "service file '%s' refused: not owned by root or writable by others", (IPTR []){ (IPTR)path });
        return LOAD_ERROR;
    }
    file = OpenFromLock(lock);
    if (!file)
    {
        UnLock(lock);
        return LOAD_ERROR;
    }
    if ((line = AllocVec(MAXLINE, MEMF_ANY)))
    {
        SetIoErr(0);
        while (res == LOAD_OK && FGets(file, line, MAXLINE - 1))
        {
            len = strlen(line);
            if (len >= MAXLINE - 2 && line[len - 1] != '\n')
            {
                PamLogA(PamBase, h, LOGF_Flag_Type_Error, "line too long in '%s'", (IPTR []){ (IPTR)path });
                res = LOAD_ERROR;
            }
            else if (!ParseLine(PamBase, h, line, service, depth))
            {
                PamLogA(PamBase, h, LOGF_Flag_Type_Error, "bad line in '%s': %s", (IPTR []){ (IPTR)path, (IPTR)line });
                res = LOAD_ERROR;
            }
        }
        /* FGets() also ends on a read error: a truncated stack is no stack */
        if (res == LOAD_OK && IoErr())
            res = LOAD_ERROR;
        FreeVec(line);
    }
    else
        res = LOAD_ERROR;
    Close(file);
    return res;
}

static BOOL ParseLine(struct PamBase *PamBase, struct PamHandle *h, STRPTR line, CONST_STRPTR service, LONG depth)
{
    STRPTR words[MAXWORDS];
    LONG n, group, control;

    n = SplitWords(line, words, MAXWORDS);
    if (n < 0)
        return FALSE;
    if (n == 0)
        return TRUE;
    if (!Stricmp(words[0], "include"))
    {
        if (n != 2 || !Stricmp(words[1], service))
            return FALSE;
        return LoadFile(PamBase, h, words[1], depth + 1) == LOAD_OK;
    }
    if (n < 3)
        return FALSE;
    group = LookupWord(PamBase, GroupNames, PAM_NUM_GROUPS, words[0]);
    control = LookupWord(PamBase, ControlNames, 4, words[1]);
    if (group < 0 || control < 0)
        return FALSE;
    return AddEntry(PamBase, h, group, control, words[2], n - 3, &words[3]);
}

/*
 * The built-in default: what a machine does with no files at all.
 *
 *   state          auth      account   session   password
 *   configured     security  security  security  security
 *   unconfigured   deny      deny      permit    deny
 *   absent         unixpw    unixpw    unixpw    unixpw
 */
static BOOL DefaultStack(struct PamBase *PamBase, struct PamHandle *h)
{
    static CONST_STRPTR const table[3][PAM_NUM_GROUPS] =
    {
        { "unixpw",   "unixpw",   "unixpw",   "unixpw"   },     /* PAM_STATE_ABSENT       */
        { "deny",     "deny",     "permit",   "deny"     },     /* PAM_STATE_UNCONFIGURED */
        { "security", "security", "security", "security" }      /* PAM_STATE_CONFIGURED   */
    };
    LONG state = PamSecurityState(PamBase), g;

    for (g = 0; g < PAM_NUM_GROUPS; g++)
    {
        if (!AddEntry(PamBase, h, g, PAM_CTL_REQUIRED, table[state][g], 0, NULL))
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
        PamLogA(PamBase, h, LOGF_Flag_Type_Error, "service '%s' cannot be used; denying", (IPTR []){ (IPTR)h->Service });
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
