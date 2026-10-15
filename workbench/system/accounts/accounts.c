/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts - the account and group databases, whichever the
          system uses

          Accounts and groups are read with usergroup.library (getpwent(),
          getgrent()), which shows security.library's users and groups on
          a multi-user system and the netinfo.device databases on a
          single-user one, and changed with ug_UserAdminA(), which writes
          to the same place.
          security.library, when present, is used only for what has no
          usergroup.library equivalent: whether an account is locked, and
          logging in as root. A user changes his own password through
          pam.library's "passwd" service, so that the system's password
          policy applies.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/usergroup.h>
#include <proto/security.h>
#include <proto/pam.h>

#include <libraries/usergroup.h>
#include <libraries/security.h>
#include <libraries/pam.h>
#include <sys/errno.h>
#include <string.h>

#include "accounts.h"

struct Library *secBase;
struct Library *PamBase;

static void FreeGroups(struct AccountsState *st);

static void Copy(STRPTR to, CONST_STRPTR from, ULONG size)
{
    strncpy((char *)to, from ? (const char *)from : "", size - 1);
    to[size - 1] = '\0';
}

BOOL Acc_Init(struct AccountsState *st)
{
    struct passwd *pw;

    memset(st, 0, sizeof(*st));

    if (FindResident(SECURITYNAME) && (secBase = OpenLibrary(SECURITYNAME, 45)))
    {
        if (secBase->lib_Version == 45 && secBase->lib_Revision < 14)
        {
            CloseLibrary(secBase);
            secBase = NULL;
        }
    }
    st->Multi = secBase && secIsConfigured();
    PamBase = OpenLibrary(PAMNAME, PAMVERSION);

    if (st->Multi)
        st->Admin = ((secGetTaskOwner(NULL) >> 16) == secROOT_UID);
    else
        st->Admin = TRUE;   /* one user owns the machine */

    if ((pw = getpwuid(geteuid())))
        Copy(st->Me, pw->pw_name, sizeof(st->Me));

    Acc_LoadGroups(st);
    return Acc_Load(st);
}

void Acc_Cleanup(struct AccountsState *st)
{
    if (st->LoggedIn && secBase)
    {
        struct TagItem tags[] = { { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
        secLogoutA(tags);
    }
    if (st->Accounts)
        FreeVec(st->Accounts);
    FreeGroups(st);
    st->Accounts = NULL;
    if (PamBase)
        CloseLibrary(PamBase);
    if (secBase)
        CloseLibrary(secBase);
    PamBase = secBase = NULL;
}

/* A copy of a member list in one block */
static STRPTR *CopyMembers(char **from)
{
    ULONG n = 0, size = 0;
    char **m;
    STRPTR *to, s;

    for (m = from; m && *m; m++)
    {
        n++;
        size += strlen(*m) + 1;
    }
    if (!(to = AllocVec((n + 1) * sizeof(STRPTR) + size, MEMF_CLEAR)))
        return NULL;
    s = (STRPTR)(to + n + 1);
    for (n = 0, m = from; m && *m; m++)
    {
        strcpy(s, *m);
        to[n++] = s;
        s += strlen(s) + 1;
    }
    to[n] = NULL;
    return to;
}

static void FreeGroups(struct AccountsState *st)
{
    ULONG i;

    for (i = 0; i < st->NumGroups; i++)
        if (st->Groups[i].Members)
            FreeVec(st->Groups[i].Members);
    if (st->Groups)
        FreeVec(st->Groups);
    st->Groups = NULL;
    st->NumGroups = 0;
}

void Acc_LoadGroups(struct AccountsState *st)
{
    struct group *gr;
    struct secGroupInfo *gi = NULL;
    ULONG n = 0, max = 0;

    FreeGroups(st);

    setgrent();
    while ((gr = getgrent()))
        max++;
    endgrent();
    if (!max || !(st->Groups = AllocVec(max * sizeof(struct GroupEntry), MEMF_CLEAR)))
        return;
    if (st->Multi)
        gi = secAllocGroupInfo();
    setgrent();
    while (n < max && (gr = getgrent()))
    {
        struct GroupEntry *g = &st->Groups[n++];

        /* gid -1: AROSTCP's group file carries its version string that way */
        if (gr->gr_gid == (gid_t)-1)
        {
            n--;
            continue;
        }
        Copy(g->Name, gr->gr_name, ACC_NAMESIZE);
        g->gid = gr->gr_gid;
        g->Members = CopyMembers(gr->gr_mem);
        if (gi)
        {
            Copy(gi->GroupID, g->Name, secGROUPIDSIZE);
            if (secGetGroupInfo(gi, secKeyType_GroupID))
                Copy(g->Description, gi->GroupName, ACC_TEXTSIZE);
        }
    }
    endgrent();
    if (gi)
        secFreeGroupInfo(gi);
    st->NumGroups = n;
}

BOOL Acc_Listed(struct GroupEntry *g, CONST_STRPTR name)
{
    STRPTR *m;

    for (m = g->Members; m && *m; m++)
        if (!strcmp(*m, name))
            return TRUE;
    return FALSE;
}

BOOL Acc_InGroup(struct GroupEntry *g, struct Account *a)
{
    return a->gid == g->gid || Acc_Listed(g, a->Name);
}

BOOL Acc_Load(struct AccountsState *st)
{
    struct passwd *pw;
    ULONG n = 0, max = 0;

    if (st->Accounts)
        FreeVec(st->Accounts);
    st->Accounts = NULL;
    st->NumAccounts = 0;

    setpwent();
    while ((pw = getpwent()))
        max++;
    endpwent();
    if (!max)
        return TRUE;
    if (!(st->Accounts = AllocVec(max * sizeof(struct Account), MEMF_CLEAR)))
        return FALSE;

    setpwent();
    while (n < max && (pw = getpwent()))
    {
        struct Account *a;

        /* uid -1 is not an account: AROSTCP's passwd carries its version string that way */
        if (pw->pw_uid == (uid_t)-1)
            continue;
        a = &st->Accounts[n++];

        Copy(a->Name, pw->pw_name, sizeof(a->Name));
        Copy(a->RealName, pw->pw_gecos, sizeof(a->RealName));
        Copy(a->Home, pw->pw_dir, sizeof(a->Home));
        Copy(a->Shell, pw->pw_shell, sizeof(a->Shell));
        a->uid = pw->pw_uid;
        a->gid = pw->pw_gid;
        if (!st->Multi)
        {
            a->Locked = (pw->pw_passwd && pw->pw_passwd[0] == '!');
            a->NoPassword = (!pw->pw_passwd || !pw->pw_passwd[0]);
        }
    }
    endpwent();
    st->NumAccounts = n;

    /* security.library knows about locks and passwords */
    if (st->Multi)
    {
        for (n = 0; n < st->NumAccounts; n++)
        {
            struct Account *a = &st->Accounts[n];
            BOOL locked = FALSE, haspw = TRUE;
            struct TagItem tags[] =
            {
                { secT_UserID,          (IPTR)a->Name   },
                { secT_AccountLocked,   (IPTR)&locked   },
                { secT_HasPassword,     (IPTR)&haspw    },
                { TAG_DONE,             0               }
            };
            secQueryUserA(tags);
            a->Locked = locked;
            a->NoPassword = !haspw;
        }
    }
    return TRUE;
}

CONST_STRPTR Acc_GroupName(struct AccountsState *st, LONG gid)
{
    ULONG i;

    for (i = 0; i < st->NumGroups; i++)
        if (st->Groups[i].gid == gid)
            return st->Groups[i].Name;
    return NULL;
}

LONG Acc_NextFreeUID(struct AccountsState *st)
{
    LONG uid;
    ULONG i;

    for (uid = 100; uid < 65535; uid++)
    {
        for (i = 0; i < st->NumAccounts; i++)
            if (st->Accounts[i].uid == uid)
                break;
        if (i == st->NumAccounts)
            return uid;
    }
    return 0;
}

LONG Acc_NextFreeGID(struct AccountsState *st)
{
    LONG gid;
    ULONG i;

    for (gid = 100; gid < 65535; gid++)
    {
        for (i = 0; i < st->NumGroups; i++)
            if (st->Groups[i].gid == gid)
                break;
        if (i == st->NumGroups)
            return gid;
    }
    return 0;
}

static LONG Result(int r)
{
    return r ? (LONG)ug_GetErr() : 0;
}

LONG Acc_Add(struct AccountsState *st, CONST_STRPTR name, LONG uid, LONG gid, CONST_STRPTR real,
             CONST_STRPTR home, CONST_STRPTR shell, CONST_STRPTR pwd)
{
    struct TagItem tags[] =
    {
        { UGA_Operation,    UGOP_ADD        },
        { UGA_Name,         (IPTR)name      },
        { UGA_UID,          uid             },
        { UGA_GID,          gid             },
        { UGA_RealName,     (IPTR)real      },
        { UGA_HomeDir,      (IPTR)home      },
        { UGA_Shell,        (IPTR)shell     },
        { UGA_Password,     (IPTR)(pwd ? pwd : (CONST_STRPTR)"") },
        { TAG_DONE,         0               }
    };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_Modify(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR newname, LONG uid, LONG gid,
                CONST_STRPTR real, CONST_STRPTR home, CONST_STRPTR shell, CONST_STRPTR pwd)
{
    struct TagItem tags[] =
    {
        { UGA_Operation,                        UGOP_MODIFY     },
        { UGA_Name,                             (IPTR)name      },
        { newname ? UGA_NewName : TAG_IGNORE,   (IPTR)newname   },
        { UGA_UID,                              uid             },
        { UGA_GID,                              gid             },
        { UGA_RealName,                         (IPTR)real      },
        { UGA_HomeDir,                          (IPTR)home      },
        { UGA_Shell,                            (IPTR)shell     },
        { pwd ? UGA_Password : TAG_IGNORE,      (IPTR)pwd       },
        { TAG_DONE,                             0               }
    };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_Delete(struct AccountsState *st, CONST_STRPTR name)
{
    struct TagItem tags[] = { { UGA_Operation, UGOP_DELETE }, { UGA_Name, (IPTR)name }, { TAG_DONE, 0 } };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_SetPassword(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR pwd)
{
    struct TagItem tags[] = { { UGA_Operation, UGOP_PASSWORD }, { UGA_Name, (IPTR)name }, { UGA_Password, (IPTR)pwd }, { TAG_DONE, 0 } };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_Lock(struct AccountsState *st, CONST_STRPTR name, BOOL lock)
{
    struct TagItem tags[] = { { UGA_Operation, UGOP_MODIFY }, { UGA_Name, (IPTR)name }, { UGA_Lock, lock }, { TAG_DONE, 0 } };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_GroupAdd(struct AccountsState *st, CONST_STRPTR name, LONG gid, CONST_STRPTR desc, STRPTR *members)
{
    STRPTR none[1] = { NULL };
    struct TagItem tags[] =
    {
        { UGA_Operation,                        UGOP_GROUPADD   },
        { UGA_Name,                             (IPTR)name      },
        { UGA_GID,                              gid             },
        { st->Multi ? UGA_RealName : TAG_IGNORE, (IPTR)desc     },
        { UGA_Members,                          (IPTR)(members ? members : none) },
        { TAG_DONE,                             0               }
    };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_GroupModify(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR newname, LONG gid,
                     CONST_STRPTR desc, STRPTR *members)
{
    struct TagItem tags[] =
    {
        { UGA_Operation,                                UGOP_GROUPMODIFY },
        { UGA_Name,                                     (IPTR)name      },
        { newname ? UGA_NewName : TAG_IGNORE,           (IPTR)newname   },
        { UGA_GID,                                      gid             },
        { (st->Multi && desc) ? UGA_RealName : TAG_IGNORE, (IPTR)desc   },
        { members ? UGA_Members : TAG_IGNORE,           (IPTR)members   },
        { TAG_DONE,                                     0               }
    };
    return Result(ug_UserAdminA(tags));
}

LONG Acc_GroupDelete(struct AccountsState *st, CONST_STRPTR name)
{
    struct TagItem tags[] = { { UGA_Operation, UGOP_GROUPDELETE }, { UGA_Name, (IPTR)name }, { TAG_DONE, 0 } };
    return Result(ug_UserAdminA(tags));
}

BOOL Acc_ChangeMyPassword(struct AccountsState *st, CONST_STRPTR oldpwd, CONST_STRPTR newpwd, STRPTR msg, ULONG msgsize)
{
    struct PamHandle *h;
    LONG r;

    msg[0] = '\0';
    if (!st->Me[0])
        return FALSE;

    if (PamBase && (h = PamStartA("passwd", st->Me, NULL, NULL)))
    {
        PamSetItem(h, PAM_OLDAUTHTOK, (APTR)oldpwd);
        PamSetItem(h, PAM_AUTHTOK, (APTR)newpwd);
        r = PamChAuthTok(h, 0);
        if (r != PAM_SUCCESS)
            Copy(msg, PamStrError(h, r), msgsize);
        PamEnd(h, r);
        return r == PAM_SUCCESS;
    }

    /* Without pam.library */
    if (st->Multi)
        return secPasswd((STRPTR)oldpwd, (STRPTR)newpwd);
    {
        struct passwd *pw = getpwnam(st->Me);
        STRPTR c;

        if (!pw)
            return FALSE;
        if (pw->pw_passwd && pw->pw_passwd[0])
        {
            if (!(c = crypt(oldpwd, pw->pw_passwd)) || strcmp(c, pw->pw_passwd))
                return FALSE;
        }
        else if (oldpwd[0])
            return FALSE;
        return Acc_SetPassword(st, st->Me, newpwd) == 0;
    }
}

BOOL Acc_LoginRoot(struct AccountsState *st)
{
    struct TagItem tags[] =
    {
        { secT_UserID,      (IPTR)"root"    },
        { secT_Graphical,   TRUE            },
        { TAG_DONE,         0               }
    };

    if (!st->Multi)
        return FALSE;
    if ((secLoginA(tags) >> 16) == secROOT_UID)
    {
        st->LoggedIn = TRUE;
        st->Admin = TRUE;
        return TRUE;
    }
    return FALSE;
}
