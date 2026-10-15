/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: UATest - exercise usergroup.library's ug_UserAdminA() (the back
          end of SYS:System/Accounts) on whatever database the system uses:
          security.library's, or the netinfo.device password and group
          files - accounts and groups.

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

const TEXT version[] = "$VER: UATest 45.2 (15.10.2026)";

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

/* Does the group (by name) exist with this gid, and list the user? */
static BOOL GroupHas(CONST_STRPTR group, LONG gid, CONST_STRPTR user)
{
    struct group *gr = getgrnam((char *)group);
    char **m;

    if (!gr || (gid >= 0 && gr->gr_gid != gid))
        return FALSE;
    for (m = gr->gr_mem; m && *m; m++)
        if (!strcmp(*m, user))
            return TRUE;
    return FALSE;
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
            struct TagItem g[] = { { UGA_Operation, UGOP_GROUPADD }, { UGA_Name, (IPTR)"uagroup" }, { UGA_GID, 4400 }, { TAG_DONE, 0 } };

            Report("add refused for a non-root caller", e == EPERM, e);
            e = Admin(g);
            Report("group add refused for a non-root caller", e == EPERM, e);
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

    /* groups */
    {
        char *mem[] = { "uatest3", NULL };
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPADD }, { UGA_Name, (IPTR)"uagroup" }, { UGA_GID, 4400 },
                               { UGA_RealName, (IPTR)"UA test group" }, { UGA_Members, (IPTR)mem }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("add group 'uagroup' (gid 4400) with 'uatest3'", e == 0 && GroupHas("uagroup", 4400, "uatest3"), e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPADD }, { UGA_Name, (IPTR)"uagroup2" }, { UGA_GID, 4400 }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("a second group with the same gid refused", e == EEXIST, e);
    }
    {
        char *mem[] = { "nosuchuser", NULL };
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPADD }, { UGA_Name, (IPTR)"uagroup3" }, { UGA_GID, 4401 },
                               { UGA_Members, (IPTR)mem }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("a group with an unknown member refused", e == ENOENT && !getgrnam("uagroup3"), e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPMODIFY }, { UGA_Name, (IPTR)"uagroup" }, { UGA_NewName, (IPTR)"uagroup2" },
                               { UGA_GID, 4402 }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("rename to 'uagroup2', renumber to 4402", e == 0 && !getgrnam("uagroup") && GroupHas("uagroup2", 4402, "uatest3"), e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)"uatest3" }, { UGA_NewName, (IPTR)"uatest4" },
                               { UGA_Password, (IPTR)"secret4" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("renamed member 'uatest4' stays in the group", e == 0 && GroupHas("uagroup2", 4402, "uatest4") &&
               !GroupHas("uagroup2", 4402, "uatest3"), e);
    }
    {
        char *none[] = { NULL }, *mem[] = { "uatest4", NULL };
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPMODIFY }, { UGA_Name, (IPTR)"uagroup2" }, { UGA_Members, (IPTR)none }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("empty the member list", e == 0 && getgrnam("uagroup2") && !GroupHas("uagroup2", -1, "uatest4"), e);
        t[2].ti_Data = (IPTR)mem;
        e = Admin(t);
        Report("set it again", e == 0 && GroupHas("uagroup2", -1, "uatest4"), e);
    }
    {
        struct TagItem u[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)"uatest4" }, { UGA_GID, 4402 }, { TAG_DONE, 0 } };
        struct TagItem d[] = { { UGA_Operation, UGOP_GROUPDELETE }, { UGA_Name, (IPTR)"uagroup2" }, { TAG_DONE, 0 } };
        struct TagItem r[] = { { UGA_Operation, UGOP_GROUPMODIFY }, { UGA_Name, (IPTR)"uagroup2" }, { UGA_GID, 4403 }, { TAG_DONE, 0 } };

        e = Admin(u);
        Report("make it the primary group of 'uatest4'", e == 0, e);
        e = Admin(d);
        Report("deleting a primary group refused", e == EBUSY, e);
        e = Admin(r);
        Report("renumbering a primary group refused", e == EBUSY, e);
        u[2].ti_Data = gid;
        e = Admin(u);
        Report("primary group back", e == 0, e);
    }
    {
        struct passwd *root = getpwnam("root");
        struct group *rg = root ? getgrgid(root->pw_gid) : NULL;
        char name[64];

        name[0] = '\0';
        if (rg)
            strncpy(name, rg->gr_name, sizeof(name) - 1), name[sizeof(name) - 1] = '\0';
        {
            struct TagItem t[] = { { UGA_Operation, UGOP_GROUPDELETE }, { UGA_Name, (IPTR)name }, { TAG_DONE, 0 } };
            e = rg ? Admin(t) : -1;
            Report("deleting root's group refused", e == EPERM, e);
        }
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_DELETE }, { UGA_Name, (IPTR)"uatest4" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("delete 'uatest4': gone from the group", e == 0 && !getpwnam("uatest4") && getgrnam("uagroup2") &&
               !GroupHas("uagroup2", -1, "uatest4"), e);
        e = Admin(t);
        Report("deleting it again: no such account", e == ENOENT, e);
    }
    {
        struct TagItem t[] = { { UGA_Operation, UGOP_GROUPDELETE }, { UGA_Name, (IPTR)"uagroup2" }, { TAG_DONE, 0 } };
        e = Admin(t);
        Report("delete group 'uagroup2'", e == 0 && !getgrnam("uagroup2"), e);
        e = Admin(t);
        Report("deleting it again: no such group", e == ENOENT, e);
    }
done:
    Close(report);
    FreeArgs(rda);
    return RETURN_OK;
}
