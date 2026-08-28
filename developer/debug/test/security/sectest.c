/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SecTest - exercise multi-user filesystem enforcement without
          touching the console (results go to a report file).

    Usage: SecTest REPORT/A,DIR/A
        REPORT - file the results are written to (created)
        DIR    - volume/directory to test in (e.g. Work:)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>

#include <string.h>
#include <stdio.h>

#define DEBUG 1
#include <aros/debug.h>

const TEXT version[] = "$VER: SecTest 45.1 (28.08.2026)";

#define TEMPLATE "REPORT/A,DIR/A"

struct Library *secBase;
static BPTR report;
static char path[512];

static void Report(CONST_STRPTR what, BOOL ok, LONG err)
{
    FPrintf(report, "%-48s : %s%s%ld\n", what, ok ? "OK" : "FAILED", ok ? "" : " err=", ok ? 0 : err);
    /* also to the serial debug output: the report file may be lost in a cache */
    bug("[SecTest] %-48s : %s%s%ld\n", what, ok ? "OK" : "FAILED", ok ? "" : " err=", ok ? 0 : (long)err);
}

static CONST_STRPTR Join(CONST_STRPTR dir, CONST_STRPTR name)
{
    strncpy(path, dir, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    AddPart(path, name, sizeof(path));
    return path;
}

static BOOL TryWrite(CONST_STRPTR file, CONST_STRPTR text)
{
    BPTR fh = Open(file, MODE_NEWFILE);
    if (!fh)
        return FALSE;
    FPuts(fh, text);
    Close(fh);
    return TRUE;
}

static BOOL TryRead(CONST_STRPTR file)
{
    char buf[64];
    BPTR fh = Open(file, MODE_OLDFILE);
    LONG n;
    if (!fh)
        return FALSE;
    n = Read(fh, buf, sizeof(buf));
    Close(fh);
    return n >= 0;
}

static BOOL TryLock(CONST_STRPTR file)
{
    BPTR l = Lock(file, SHARED_LOCK);
    if (!l)
        return FALSE;
    UnLock(l);
    return TRUE;
}

int main(void)
{
    IPTR args[2] = { 0 };
    struct RDArgs *rda;
    CONST_STRPTR dir;
    ULONG owner;
    BOOL ok;
    char who[64], mine[128];

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
        return RETURN_FAIL;
    if (!(report = Open((STRPTR)args[0], MODE_NEWFILE)))
    {
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    dir = (CONST_STRPTR)args[1];

    secBase = OpenLibrary(SECURITYNAME, 0);
    owner = secBase ? secGetTaskOwner(NULL) : secOWNER_NOBODY;
    if (owner == secOWNER_NOBODY)
        strcpy(who, "nobody");
    else
        snprintf(who, sizeof(who), "%lu-%lu", (unsigned long)(owner >> 16), (unsigned long)(owner & 0xffff));
    FPrintf(report, "--- SecTest as %s in %s (secfs: %s) ---\n", who, dir,
            secBase ? (secIsConfigured() ? "configured" : "unconfigured") : "no library");
    bug("[SecTest] --- as %s in %s (secfs: %s) ---\n", who, dir,
        secBase ? (secIsConfigured() ? "configured" : "unconfigured") : "no library");

    /* per-user names so that a previous run as another user does not interfere */
    snprintf(mine, sizeof(mine), "sectest-%s-rootdir.txt", who);
    ok = TryWrite(Join(dir, mine), "written by sectest\n");
    Report("create file in volume root", ok, IoErr());
    ok = TryRead(Join(dir, "rootfile.txt"));
    Report("read root's rootfile.txt", ok, IoErr());
    ok = TryLock(Join(dir, "rootfile.txt"));
    Report("lock root's rootfile.txt", ok, IoErr());
    ok = DeleteFile(Join(dir, "rootfile.txt"));
    Report("delete root's rootfile.txt", ok, IoErr());
    if (ok)     /* put it back for the runs as other users */
        TryWrite(Join(dir, "rootfile.txt"), "hello from root\n");
    ok = TryLock(Join(dir, "Home"));
    Report("lock Home (root, no other bits)", ok, IoErr());
    snprintf(mine, sizeof(mine), "Home/user/sectest-%s.txt", who);
    ok = TryWrite(Join(dir, mine), "mine\n");
    Report("create file in Home/user (owned by user)", ok, IoErr());
    ok = TryRead(Join(dir, "Home/user/mine.txt"));
    Report("read Home/user/mine.txt", ok, IoErr());
    ok = SetProtection(Join(dir, "Home/user/mine.txt"), FIBF_OTR_READ | FIBF_GRP_READ);
    Report("set protection of Home/user/mine.txt", ok, IoErr());
    ok = SetProtection(Join(dir, "Shared/s.txt"), FIBF_OTR_READ);
    Report("set protection of Shared/s.txt (owner 0:100)", ok, IoErr());
    snprintf(mine, sizeof(mine), "Shared/sectest-%s.txt", who);
    ok = TryWrite(Join(dir, mine), "shared\n");
    Report("create file in Shared (owner 0:100, grp bits?)", ok, IoErr());
    ok = SetOwner(Join(dir, "Home/user/mine.txt"), (0 << 16) | 0);
    Report("give Home/user/mine.txt to root", ok, IoErr());
    snprintf(mine, sizeof(mine), "Home/user/sub-%s", who);
    {
        BPTR l = CreateDir(Join(dir, mine));
        ok = (l != BNULL);
        if (l) UnLock(l);
    }
    Report("create dir Home/user/sub-<uid>", ok, IoErr());
    snprintf(mine, sizeof(mine), "sectest-%s-rootsub", who);
    {
        BPTR l = CreateDir(Join(dir, mine));
        ok = (l != BNULL);
        if (l) UnLock(l);
    }
    Report("create dir in volume root", ok, IoErr());

    if (secBase)
        CloseLibrary(secBase);
    Close(report);
    FreeArgs(rda);
    return RETURN_OK;
}
