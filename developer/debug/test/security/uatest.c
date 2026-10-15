/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: UATest - exercise usergroup.library's ug_UserAdminA() (the back
          end of SYS:System/Accounts) on whatever database the system uses:
          security.library's, or the netinfo.device password file.

    Usage: UATest REPORT/A,EXPECT/K
        EXPECT=DENIED: the caller may not change accounts (a user on a
        multi-user system); every change must fail with EPERM.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/usergroup.h>

#include <libraries/usergroup.h>
#include <sys/errno.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

const TEXT version[] = "$VER: UATest 45.1 (15.10.2026)";

static BPTR report;

static void Report(CONST_STRPTR what, BOOL ok, LONG detail)
{
    FPrintf(report, "%-52s : %s (%ld)\n", what, ok ? "OK" : "FAILED", detail);
    bug("[UATest] %-52s : %s (%ld)\n", what, ok ? "OK" : "FAILED", (long)detail);
}

static LONG Admin(struct TagItem *tags)
{
    return ug_UserAdminA(tags) ? (LONG)ug_GetErr() : 0;
}

int main(void)
{
    IPTR args[2] = { 0 };
    struct RDArgs *rda;
    struct passwd *pw;
    BOOL denied;
    LONG e, gid = 100;
    struct group *gr;

    if (!(rda = ReadArgs("REPORT/A,EXPECT/K", args, NULL)))
        return RETURN_FAIL;
    if (!(report = Open((STRPTR)args[0], MODE_NEWFILE)))
    {
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    denied = args[1] && !strcmp((char *)args[1], "DENIED");

    /* a group that exists */
    setgrent();
    if ((gr = getgrent()))
        gid = gr->gr_gid;
    while ((gr = getgrent()))
        if (!strcmp(gr->gr_name, "users"))
            gid = gr->gr_gid;
    endgrent();
    FPrintf(report, "--- UATest: uid %ld, using gid %ld ---\n", (long)getuid(), gid);
    {
        struct group *g = getgrgid(gid);
        char *c = crypt("password", "ab");
        Report("getgrgid() finds that group", g && g->gr_gid == gid, g ? (LONG)g->gr_gid : -1);
        Report("crypt() works", c && c[0], c ? (LONG)strlen(c) : -1);
    }

    {
        struct TagItem t[] = { { UGA_Operation, UGOP_ADD }, { UGA_Name, (IPTR)"uatest" }, { UGA_UID, 4321 },
                               { UGA_GID, gid }, { UGA_RealName, (IPTR)"UA Test" }, { UGA_HomeDir, (IPTR)"SYS:" },
                               { UGA_Shell, (IPTR)"" }, { UGA_Password, (IPTR)"secret1" }, { TAG_DONE, 0 } };
        e = Admin(t);
        if (denied)
        {
            Report("add refused for a non-root caller", e == EPERM, e);
            goto done;
        }
        Report("add 'uatest' (uid 4321)", e == 0, e);
    }
    pw = getpwnam("uatest");
    Report("getpwnam finds it", pw && pw->pw_uid == 4321, pw ? pw->pw_uid : -1);
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_ADD }, { UGA_Name, (IPTR)"uatest2" }, { UGA_UID, 4321 },
                               { UGA_GID, gid }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("a second account with the same uid refused", e == EEXIST, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_ADD }, { UGA_Name, (IPTR)"bad|name" }, { UGA_UID, 4322 },
                               { UGA_GID, gid }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("a name with '|' refused", e == EINVAL, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)"uatest" }, { UGA_RealName, (IPTR)"Changed Name" }, { TAG_DONE, 0 } };
        e = Admin(t);
        pw = getpwnam("uatest");
        Report("modify the full name", e == 0 && pw && !strcmp(pw->pw_gecos, "Changed Name"), e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)"uatest" }, { UGA_Lock, TRUE }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("lock", e == 0, e);
        t[2].ti_Data = FALSE;
        e = Admin(t);
        Report("unlock", e == 0, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_PASSWORD }, { UGA_Name, (IPTR)"uatest" }, { UGA_Password, (IPTR)"secret2" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("set the password", e == 0, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)"uatest" }, { UGA_NewName, (IPTR)"uatest3" },
                               { UGA_Password, (IPTR)"secret3" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("rename to 'uatest3' (with a new password)", e == 0 && getpwnam("uatest3") && !getpwnam("uatest"), e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_DELETE }, { UGA_Name, (IPTR)"root" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("deleting root refused", e == EPERM, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_DELETE }, { UGA_Name, (IPTR)"uatest3" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("delete 'uatest3'", e == 0 && !getpwnam("uatest3"), e);
        e = Admin(t);
        Report("deleting it again: no such account", e == ENOENT, e);
    }
done:
    Close(report);
    FreeArgs(rda);
    return RETURN_OK;
}
