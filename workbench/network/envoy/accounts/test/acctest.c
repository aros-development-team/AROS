/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AccTest - exercise accounts.library against an Accounts Server.

    AccTest HOST/K,USER/K,PASSWORD/K,GROUP/K

    Without HOST the public functions are used (the library's own link to
    the local server, or to ENV:Envoy/AccountsServer); with HOST the
    Acc#?() functions are used on an entity found on that host. Lists users
    and groups with their members, looks USER up by name and ID, GROUP by
    name and ID, tests MemberOf, and verifies USER's PASSWORD in clear and
    as an ECrypt hash, each with a wrong value too. Returns 5 when a check
    gave an unexpected result.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/nipc.h>
#include <proto/accounts.h>
#include <envoy/nipc.h>
#include <envoy/accounts.h>
#include <envoy/errors.h>
#include <string.h>
#include <stdio.h>

/* nipc.library and accounts.library are opened by the link libraries' auto-open code */

static struct Entity *server;       /* NULL: public functions */
static int failures;

#define U(name) (server ? Acc##name : name)

static ULONG ucall_next(struct UserInfo *u) { return server ? AccNextUser(u, server) : NextUser(u); }
static ULONG gcall_next(struct GroupInfo *g) { return server ? AccNextGroup(g, server) : NextGroup(g); }
static ULONG call_nextmember(struct GroupInfo *g, struct UserInfo *u) { return server ? AccNextMember(g, u, server) : NextMember(g, u); }
static ULONG call_nametouser(STRPTR n, struct UserInfo *u) { return server ? AccNameToUser(n, u, server) : NameToUser(n, u); }
static ULONG call_idtouser(UWORD id, struct UserInfo *u) { return server ? AccIDToUser(id, u, server) : IDToUser(id, u); }
static ULONG call_nametogroup(STRPTR n, struct GroupInfo *g) { return server ? AccNameToGroup(n, g, server) : NameToGroup(n, g); }
static ULONG call_idtogroup(UWORD id, struct GroupInfo *g) { return server ? AccIDToGroup(id, g, server) : IDToGroup(id, g); }
static ULONG call_memberof(struct GroupInfo *g, struct UserInfo *u) { return server ? AccMemberOf(g, u, server) : MemberOf(g, u); }
static ULONG call_verify(STRPTR n, STRPTR p, struct UserInfo *u) { return server ? AccVerifyUser(n, p, u, server) : VerifyUser(n, p, u); }
static ULONG call_verifycrypt(STRPTR n, STRPTR p, struct UserInfo *u) { return server ? AccVerifyUserCrypt(n, p, u, server) : VerifyUserCrypt(n, p, u); }

static void Expect(const char *what, ULONG got, ULONG want)
{
    printf("  %-44s -> %3lu %s\n", what, (unsigned long)got, got == want ? "ok" : "UNEXPECTED");
    if (got != want)
        failures++;
}

static void ShowUser(const char *prefix, const struct UserInfo *u)
{
    printf("%s'%s' uid %u gid %u flags 0x%02lx\n", prefix, u->ui_UserName, u->ui_UserID, u->ui_PrimaryGroupID, (unsigned long)u->ui_Flags);
}

int main(void)
{
    IPTR args[4] = { 0 };
    struct RDArgs *rda;
    struct Entity *me = NULL;
    struct UserInfo *u, *u2;
    struct GroupInfo *g;
    STRPTR user, pw, group;
    ULONG err;
    char hash[12];

    if (!(rda = ReadArgs("HOST/K,USER/K,PASSWORD/K,GROUP/K", args, NULL)))
    {
        PrintFault(IoErr(), "AccTest");
        return RETURN_ERROR;
    }
    user = args[1] ? (STRPTR)args[1] : (STRPTR)"alice";
    pw = args[2] ? (STRPTR)args[2] : (STRPTR)"secret";
    group = args[3] ? (STRPTR)args[3] : (STRPTR)"staff";

    if (args[0])
    {
        if (!(me = CreateEntity(ENT_Name, (IPTR)"AccTest", ENT_AllocSignal, 0, TAG_DONE)))
            return RETURN_FAIL;
        if (!(server = FindEntity((STRPTR)args[0], "Accounts Server", me, &err)))
        {
            printf("FindEntity(%s, Accounts Server) failed: %lu\n", (char *)args[0], (unsigned long)err);
            DeleteEntity(me);
            FreeArgs(rda);
            return RETURN_FAIL;
        }
        printf("using the Accounts Server on %s through the Acc#?() functions\n", (char *)args[0]);
    }
    else
        printf("using the public functions (library-managed link)\n");

    u = AllocUserInfo();
    u2 = AllocUserInfo();
    g = AllocGroupInfo();

    printf("--- users\n");
    memset(u, 0, sizeof(*u));
    while (!(err = ucall_next(u)))
        ShowUser("  ", u);
    Expect("NextUser end of list", err, ENVOYERR_LASTUSER);

    printf("--- groups and members\n");
    memset(g, 0, sizeof(*g));
    while (!(err = gcall_next(g)))
    {
        printf("  '%s' gid %u admin %u flags 0x%lx\n", g->gi_GroupName, g->gi_GroupID, g->gi_AdminID, (unsigned long)g->gi_Flags);
        memset(u, 0, sizeof(*u));
        while (!(err = call_nextmember(g, u)))
            ShowUser("      member ", u);
        if (err != ENVOYERR_LASTMEMBER)
            Expect("NextMember end of list", err, ENVOYERR_LASTMEMBER);
    }
    Expect("NextGroup end of list", err, ENVOYERR_LASTGROUP);

    printf("--- lookups\n");
    Expect("NameToUser(USER)", call_nametouser(user, u), 0);
    ShowUser("    ", u);
    Expect("IDToUser(its ID)", call_idtouser(u->ui_UserID, u2), 0);
    Expect("  same name back", strcmp((char *)u->ui_UserName, (char *)u2->ui_UserName) == 0 ? 0 : 1, 0);
    Expect("NameToUser(nosuchuser)", call_nametouser("nosuchuser", u2), ENVOYERR_UNKNOWNUSER);
    Expect("IDToUser(60000)", call_idtouser(60000, u2), ENVOYERR_UNKNOWNUSER);
    Expect("NameToGroup(GROUP)", call_nametogroup(group, g), 0);
    Expect("IDToGroup(its ID)", call_idtogroup(g->gi_GroupID, g), 0);
    Expect("NameToGroup(nosuchgroup)", call_nametogroup("nosuchgroup", g), ENVOYERR_UNKNOWNGROUP);

    printf("--- membership\n");
    call_nametogroup(group, g);
    call_nametouser(user, u);
    err = call_memberof(g, u);
    printf("  MemberOf(%s, %s)                         -> %3lu %s\n", group, user, (unsigned long)err,
           err == 0 ? "member" : err == ENVOYERR_UNKNOWNMEMBER ? "not a member" : "UNEXPECTED");
    if (err != 0 && err != ENVOYERR_UNKNOWNMEMBER)
        failures++;
    u->ui_Flags ^= 0x80;
    Expect("MemberOf with an altered user record", call_memberof(g, u), ENVOYERR_UNKNOWNUSER);

    printf("--- verification\n");
    Expect("VerifyUser(USER, PASSWORD)", call_verify(user, pw, u), 0);
    Expect("VerifyUser(USER, wrong)", call_verify(user, "wrong-password", u), ENVOYERR_UNKNOWNUSER);
    Expect("VerifyUser(nosuchuser, x)", call_verify("nosuchuser", "x", u), ENVOYERR_UNKNOWNUSER);
    ECrypt(hash, pw, user);
    printf("  ECrypt(PASSWORD, USER) = %s\n", hash);
    Expect("VerifyUserCrypt(USER, ECrypt)", call_verifycrypt(user, hash, u), 0);
    ECrypt(hash, "wrong-password", user);
    Expect("VerifyUserCrypt(USER, ECrypt of wrong)", call_verifycrypt(user, hash, u), ENVOYERR_UNKNOWNUSER);
    Expect("VerifyUserCrypt(USER, clear password)", call_verifycrypt(user, pw, u), ENVOYERR_UNKNOWNUSER);
    ECrypt(hash, "Admin", "Admin");
    Expect("ECrypt(Admin, Admin) == jutNGZGRdOV", strcmp(hash, "jutNGZGRdOV") == 0 ? 0 : 1, 0);

    printf("%d unexpected results\n", failures);
    FreeUserInfo(u);
    FreeUserInfo(u2);
    FreeGroupInfo(g);
    if (server)
        LoseEntity(server);
    if (me)
        DeleteEntity(me);
    FreeArgs(rda);
    return failures ? RETURN_WARN : RETURN_OK;
}
