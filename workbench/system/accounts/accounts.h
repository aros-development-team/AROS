/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts - the account and group databases, whichever the
          system uses
*/
#ifndef _ACCOUNTS_ACCOUNTS_H
#define _ACCOUNTS_ACCOUNTS_H

#include <exec/types.h>

#define ACC_NAMESIZE    64
#define ACC_TEXTSIZE    256

struct Account
{
    char    Name[ACC_NAMESIZE];
    char    RealName[ACC_TEXTSIZE];
    char    Home[ACC_TEXTSIZE];
    char    Shell[ACC_TEXTSIZE];
    LONG    uid, gid;
    BOOL    Locked;
    BOOL    NoPassword;
};

struct GroupEntry
{
    char    Name[ACC_NAMESIZE];
    char    Description[ACC_TEXTSIZE];  /* multi-user only */
    LONG    gid;
    STRPTR  *Members;                   /* NULL-terminated user names (one block) */
};

struct AccountsState
{
    BOOL                Multi;          /* security.library is resident and configured */
    BOOL                Admin;          /* may change other accounts */
    BOOL                LoggedIn;       /* we logged this task in as root */
    char                Me[ACC_NAMESIZE];
    struct Account      *Accounts;
    ULONG               NumAccounts;
    struct GroupEntry   *Groups;
    ULONG               NumGroups;
};

BOOL  Acc_Init(struct AccountsState *st);
void  Acc_Cleanup(struct AccountsState *st);
BOOL  Acc_Load(struct AccountsState *st);
void  Acc_LoadGroups(struct AccountsState *st);

/* errno-style results: 0 for success */
LONG  Acc_Add(struct AccountsState *st, CONST_STRPTR name, LONG uid, LONG gid, CONST_STRPTR real,
              CONST_STRPTR home, CONST_STRPTR shell, CONST_STRPTR pwd);
LONG  Acc_Modify(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR newname, LONG uid, LONG gid,
                 CONST_STRPTR real, CONST_STRPTR home, CONST_STRPTR shell, CONST_STRPTR pwd);
LONG  Acc_Delete(struct AccountsState *st, CONST_STRPTR name);
LONG  Acc_SetPassword(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR pwd);
LONG  Acc_Lock(struct AccountsState *st, CONST_STRPTR name, BOOL lock);

/* The caller's own password, through pam.library's "passwd" service when
 * it can be used. On failure, why (for a requester) in msg. */
BOOL  Acc_ChangeMyPassword(struct AccountsState *st, CONST_STRPTR oldpwd, CONST_STRPTR newpwd, STRPTR msg, ULONG msgsize);

/* Multi-user: log this task in as root (the graphical login asks) */
BOOL  Acc_LoginRoot(struct AccountsState *st);

CONST_STRPTR Acc_GroupName(struct AccountsState *st, LONG gid);
LONG  Acc_NextFreeUID(struct AccountsState *st);
LONG  Acc_NextFreeGID(struct AccountsState *st);

/* Is the account in the group: its primary group, or listed as a member? */
BOOL  Acc_InGroup(struct GroupEntry *g, struct Account *a);
/* Listed as a member (not just through the primary group) */
BOOL  Acc_Listed(struct GroupEntry *g, CONST_STRPTR name);

/* Groups; members: NULL-terminated user names besides those whose
 * primary group it is (replaces the list), NULL to keep it */
LONG  Acc_GroupAdd(struct AccountsState *st, CONST_STRPTR name, LONG gid, CONST_STRPTR desc, STRPTR *members);
LONG  Acc_GroupModify(struct AccountsState *st, CONST_STRPTR name, CONST_STRPTR newname, LONG gid,
                      CONST_STRPTR desc, STRPTR *members);
LONG  Acc_GroupDelete(struct AccountsState *st, CONST_STRPTR name);

/* Single-user: nothing defined yet? Then the standard accounts and groups
 * of a network environment (AROSTCP's db/passwd and db/group) can be
 * created; those that exist already are left alone. 0 or the first error. */
BOOL  Acc_NeedsDefaults(struct AccountsState *st);
LONG  Acc_AddDefaults(struct AccountsState *st);

#endif /* _ACCOUNTS_ACCOUNTS_H */
