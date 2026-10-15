/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AuthTest - exercise security.library's authentication process:
          the per-user hold-back, the remote lock-out, login tickets,
          account locking and the root-only parts of secVerifyUserA() and
          secAuthControlA(). Results go to a report file and the debug
          output, like SecTest.

    Usage: AuthTest REPORT/A,USER/A,PHASE/A
        REPORT - file the results are written to (created)
        USER   - a user without a password to test with
        PHASE  - ROOT (run as root; Security.config LOGINDELAYMAX 1,
                 MAXTRIES 5), USER (run as that user afterwards) or LOG
                 (write the security.library and pam.library entries of
                 log.resource to the report)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <proto/log.h>
#include <resources/log.h>

#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/security.h>

#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

const TEXT version[] = "$VER: AuthTest 45.1 (15.10.2026)";

#define TEMPLATE "REPORT/A,USER/A,PHASE/A"

struct Library *secBase;
static BPTR report;
static CONST_STRPTR testuser;

static void Report(CONST_STRPTR what, BOOL ok, LONG detail)
{
    FPrintf(report, "%-56s : %s (%ld)\n", what, ok ? "OK" : "FAILED", detail);
    bug("[AuthTest] %-56s : %s (%ld)\n", what, ok ? "OK" : "FAILED", (long)detail);
}

static ULONG Verify(CONST_STRPTR user, CONST_STRPTR pwd, CONST_STRPTR host, struct secLoginTicket *ticket)
{
    struct TagItem tags[] =
    {
        { secT_UserID,                                  (IPTR)user      },
        { secT_Password,                                (IPTR)pwd       },
        { secT_Service,                                 (IPTR)"authtest" },
        { host ? secT_RemoteHost : TAG_IGNORE,          (IPTR)host      },
        { ticket ? secT_Ticket : TAG_IGNORE,            (IPTR)ticket    },
        { TAG_DONE,                                     0               }
    };
    return secVerifyUserA(tags);
}

/* Ticks a call takes */
static LONG Ticks(struct DateStamp *a, struct DateStamp *b)
{
    return (b->ds_Days - a->ds_Days) * 24 * 60 * 50 + (b->ds_Minute - a->ds_Minute) * 60 * 50 + (b->ds_Tick - a->ds_Tick);
}

static BOOL Clear(CONST_STRPTR user)
{
    struct TagItem tags[] = { { secT_AuthClear, TRUE }, { user ? secT_UserID : TAG_IGNORE, (IPTR)user }, { TAG_DONE, 0 } };
    return secAuthControlA(tags);
}

/* A child process tries the parent's ticket */
static struct secLoginTicket shared;
static volatile ULONG childresult;
static struct Task *parent;

static void ChildEntry(void)
{
    struct TagItem tags[] = { { secT_Ticket, (IPTR)&shared }, { TAG_DONE, 0 } };

    childresult = secLoginA(tags);
    Signal(parent, SIGBREAKF_CTRL_F);
}

static void RootPhase(void)
{
    struct DateStamp t0, t1;
    struct secLoginTicket ticket, forged;
    ULONG r, owner, count = 0;
    LONG tk, i;
    char longhost[120];

    r = Verify(testuser, "", NULL, NULL);
    Report("verify, right (empty) password", r == secVERIFY_OK, r);

    /* hold-back after failures (LOGINDELAYMAX 1) */
    Clear(testuser);
    DateStamp(&t0);
    r = Verify(testuser, "wrong", NULL, NULL);
    DateStamp(&t1);
    Report("verify, wrong password: failed at once", r == secVERIFY_FAILED && Ticks(&t0, &t1) < 40, Ticks(&t0, &t1));
    DateStamp(&t0);
    r = Verify(testuser, "wrong", NULL, NULL);
    DateStamp(&t1);
    tk = Ticks(&t0, &t1);
    Report("verify again: held back about a second", r == secVERIFY_FAILED && tk >= 35 && tk < 150, tk);
    DateStamp(&t0);
    r = Verify(testuser, "", NULL, NULL);
    DateStamp(&t1);
    tk = Ticks(&t0, &t1);
    Report("right password after failures: held back, then OK", r == secVERIFY_OK && tk >= 35, tk);
    DateStamp(&t0);
    r = Verify(testuser, "", NULL, NULL);
    DateStamp(&t1);
    Report("success cleared the hold-back", r == secVERIFY_OK && Ticks(&t0, &t1) < 40, Ticks(&t0, &t1));

    /* local failures never lock out */
    for (i = 0; i < 6; i++)
        Verify(testuser, "wrong", NULL, NULL);
    r = Verify(testuser, "", NULL, NULL);
    Report("6 local failures: no lock-out", r == secVERIFY_OK, r);

    /* remote lock-out (MAXTRIES 5). The test user has no password and an
     * empty password is never accepted from a remote host, so these
     * compare LOCKED with FAILED */
    Clear(NULL);
    r = Verify(testuser, "", "host-b", NULL);
    Report("empty password from a remote host refused", r == secVERIFY_FAILED, r);
    Clear(NULL);
    for (i = 0; i < 5; i++)
        Verify(testuser, "wrong", "host-a", NULL);
    r = Verify(testuser, "wrong", "host-a", NULL);
    Report("5 failures from host-a: host-a locked out", r == secVERIFY_LOCKED, r);
    r = Verify(testuser, "wrong", "host-b", NULL);
    Report("host-b not locked out", r == secVERIFY_FAILED, r);
    r = Verify(testuser, "", NULL, NULL);
    Report("local not locked out", r == secVERIFY_OK, r);

    memset(longhost, 'x', sizeof(longhost) - 1);
    longhost[sizeof(longhost) - 1] = '\0';
    for (i = 0; i < 5; i++)
        Verify(testuser, "wrong", longhost, NULL);
    r = Verify(testuser, "wrong", longhost, NULL);
    Report("119-character host name locked out too", r == secVERIFY_LOCKED, r);

    {
        struct secAuthRecord recs[16];
        struct TagItem tags[] = { { secT_AuthList, (IPTR)recs }, { secT_AuthListSize, 16 }, { secT_AuthCount, (IPTR)&count }, { TAG_DONE, 0 } };
        BOOL ok = secAuthControlA(tags);
        Report("secAuthControlA() lists the records", ok && count >= 2, count);
    }
    Report("secAuthControlA() clears them", Clear(NULL), 0);
    r = Verify(testuser, "wrong", "host-a", NULL);
    Report("host-a no longer locked out", r == secVERIFY_FAILED, r);

    /* tickets */
    Clear(NULL);
    r = Verify(testuser, "", NULL, &ticket);
    Report("verify with a ticket", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_Ticket, (IPTR)&ticket }, { TAG_DONE, 0 } };
        owner = secLoginA(tags);
        Report("secLoginA() with the ticket", owner != secOWNER_NOBODY && (owner >> 16) != secROOT_UID, owner);
        {
            struct TagItem out[] = { { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
            owner = secLogoutA(out);
            Report("logged out again: root", (owner >> 16) == secROOT_UID, owner);
        }
        owner = secLoginA(tags);
        Report("the same ticket a second time refused", owner == secOWNER_NOBODY, owner);
    }
    memset(&forged, 0x5a, sizeof(forged));
    {
        struct TagItem tags[] = { { secT_Ticket, (IPTR)&forged }, { TAG_DONE, 0 } };
        owner = secLoginA(tags);
        Report("a made-up ticket refused", owner == secOWNER_NOBODY, owner);
    }
    r = Verify(testuser, "", NULL, &shared);
    {
        struct TagItem ptags[] = { { NP_Entry, (IPTR)ChildEntry }, { NP_Name, (IPTR)"AuthTest child" }, { TAG_DONE, 0 } };

        parent = FindTask(NULL);
        childresult = 0xdead;
        SetSignal(0, SIGBREAKF_CTRL_F);
        if (r == secVERIFY_OK && CreateNewProc(ptags))
        {
            Wait(SIGBREAKF_CTRL_F);
            Report("another task cannot use this task's ticket", childresult == secOWNER_NOBODY, childresult);
        }
        else
            Report("another task cannot use this task's ticket", FALSE, r);
    }
    {
        struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)&shared }, { TAG_DONE, 0 } };
        Report("discard a ticket", secVerifyUserA(tags) == secVERIFY_OK, 0);
    }

    /* account locking */
    {
        struct TagItem lock[] = { { secT_LockAccount, (IPTR)testuser }, { TAG_DONE, 0 } };
        struct TagItem unlock[] = { { secT_UnlockAccount, (IPTR)testuser }, { TAG_DONE, 0 } };
        BOOL locked = FALSE;
        struct TagItem q[] = { { secT_UserID, (IPTR)testuser }, { secT_AccountLocked, (IPTR)&locked }, { TAG_DONE, 0 } };

        Report("lock the account", secAuthControlA(lock), 0);
        secQueryUserA(q);
        Report("secQueryUserA() reports it locked", locked, locked);
        Clear(NULL);
        r = Verify(testuser, "", NULL, NULL);
        Report("a locked account does not verify", r == secVERIFY_FAILED, r);
        Report("unlock the account", secAuthControlA(unlock), 0);
        Clear(NULL);
        r = Verify(testuser, "", NULL, NULL);
        Report("unlocked again: verifies", r == secVERIFY_OK, r);
    }

    /* hashed tokens need HASHEDLOGIN, even for root */
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)testuser }, { secT_PasswordHash, (IPTR)"abcdefghijk" }, { TAG_DONE, 0 } };
        r = secVerifyUserA(tags);
        Report("hashed token without HASHEDLOGIN: not supported", r == secVERIFY_NOTSUPPORTED, r);
    }
    Clear(NULL);
}

static void UserPhase(void)
{
    ULONG r;

    /* host-a was locked out for the user in the root phase and cleared;
     * lock it again as root would - a user's host name is ignored anyway */
    r = Verify(testuser, "", "host-a", NULL);
    Report("a user's secT_RemoteHost is ignored", r == secVERIFY_OK, r);
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)testuser }, { secT_PasswordHash, (IPTR)"abcdefghijk" }, { TAG_DONE, 0 } };
        r = secVerifyUserA(tags);
        Report("a user may not verify a hashed token", r == secVERIFY_NOTSUPPORTED, r);
    }
    Report("a user may not list the records", !secAuthControlA(NULL), 0);
    {
        struct TagItem lock[] = { { secT_LockAccount, (IPTR)"root" }, { TAG_DONE, 0 } };
        Report("a user may not lock accounts", !secAuthControlA(lock), 0);
    }
}

static void LogPhase(void)
{
    APTR LogResBase = OpenResource("log.resource");
    APTR entry = (APTR)LOGEntry_First;
    ULONG n = 0;

    if (!LogResBase)
    {
        Report("log.resource present", FALSE, 0);
        return;
    }
    logLockEntries(LLF_READ);
    while ((entry = logNextEntry(&entry)) && entry != (APTR)LOGEntry_Last)
    {
        STRPTR comp = NULL, sub = NULL, text = NULL;
        ULONG flags = 0;
        struct TagItem tags[] =
        {
            { LOGMA_Component,      (IPTR)&comp     },
            { LOGMA_SubComponent,   (IPTR)&sub      },
            { LOGMA_Entry,          (IPTR)&text     },
            { LOGMA_Flags,          (IPTR)&flags    },
            { TAG_DONE,             0               }
        };

        logGetEntryAttrs(entry, tags);
        if (comp && (!strcmp(comp, "security.library") || !strcmp(comp, "pam.library")))
        {
            FPrintf(report, "%s [%s] %08lx: %s\n", comp, sub ? sub : (STRPTR)"", (ULONG)flags, text ? text : (STRPTR)"");
            n++;
        }
    }
    logUnlockEntries(LLF_READ);
    Report("security/pam entries in log.resource", n > 0, n);
}

int main(void)
{
    IPTR args[3] = { 0 };
    struct RDArgs *rda;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "AuthTest");
        return RETURN_FAIL;
    }
    if (!(secBase = OpenLibrary(SECURITYNAME, 45)) || secBase->lib_Revision < 14)
    {
        PutStr("AuthTest: needs security.library 45.14\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (!(report = Open((STRPTR)args[0], MODE_NEWFILE)))
    {
        CloseLibrary(secBase);
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    testuser = (CONST_STRPTR)args[1];
    if (!Stricmp((STRPTR)args[2], "ROOT"))
        RootPhase();
    else if (!Stricmp((STRPTR)args[2], "LOG"))
        LogPhase();
    else
        UserPhase();

    Close(report);
    CloseLibrary(secBase);
    FreeArgs(rda);
    return RETURN_OK;
}
