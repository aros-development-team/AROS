/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts Server - the account store.

          Two backends behind one interface:
          - the system's own password and group files (SYS:Security, found
            through security.library), read and written here; the security
            server watches them and reloads on every change;
          - usergroup.library, read-only, when security.library is absent
            or unconfigured (its store is then the stack's files).
          IDs are MuFS/Envoy IDs throughout (root 0xFFFF, nobody 0).
*/
#ifndef ACC_STORE_H
#define ACC_STORE_H

#include <exec/lists.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <envoy/accounts.h>
#include <envoy/errors.h>

#define ACC_NAMESIZE    32
#define ACC_ROOT_UID    0xFFFF
#define ACC_NOBODY_UID  0

struct AccUser
{
    struct MinNode  Node;
    char            Name[ACC_NAMESIZE];
    char            Hash[64];
    UWORD           Uid, Gid;
    ULONG           Flags;                  /* UFLAGF_#?, derived from the store */
    char            Gecos[128], Home[128], Shell[64];
};

struct AccGroup
{
    struct MinNode  Node;
    char            Name[ACC_NAMESIZE];
    UWORD           Gid, Admin;
    ULONG           Flags;
    char            Desc[128];
    UWORD          *Members;                /* user IDs, in user-name order */
    ULONG           NumMembers, MaxMembers;
};

struct AccStore
{
    struct MinList  Users, Groups;
    BOOL            Writable;               /* file backend in use */
    BOOL            Loaded;
    char            PasswdPath[256], GroupPath[256], ConfigPath[256];
    struct DateStamp PasswdDate, GroupDate;
    BOOL            UsersDirty, GroupsDirty;
    UWORD           PasswdUidLevel, PasswdGidLevel;
    struct Library *SecLib;
    struct Library *UserGroupBase;
    struct Library *PamLib;
    BOOL            Verbose;
};

BOOL  StoreInit(struct AccStore *s, BOOL verbose);
void  StoreCleanup(struct AccStore *s);
void  StoreRefresh(struct AccStore *s);
void  StoreCommit(struct AccStore *s);

struct AccUser  *StoreUserByName(struct AccStore *s, CONST_STRPTR name);
struct AccUser  *StoreUserById(struct AccStore *s, UWORD uid);
struct AccGroup *StoreGroupByName(struct AccStore *s, CONST_STRPTR name);
struct AccGroup *StoreGroupById(struct AccStore *s, UWORD gid);
BOOL  StoreIsMember(struct AccGroup *g, UWORD uid);
struct AccUser  *StoreNextMember(struct AccStore *s, struct AccGroup *g, struct AccUser *after);

ULONG StoreVerify(struct AccStore *s, struct AccUser *u, CONST_STRPTR token, BOOL hashed, CONST_STRPTR rhost);

ULONG StoreAddUser(struct AccStore *s, CONST_STRPTR name, UWORD gid, ULONG flags, CONST_STRPTR password, struct AccUser **result);
ULONG StoreDeleteUser(struct AccStore *s, struct AccUser *u);
ULONG StoreModifyUser(struct AccStore *s, struct AccUser *u, CONST_STRPTR name, UWORD gid, ULONG flags);
ULONG StoreSetPassword(struct AccStore *s, struct AccUser *u, CONST_STRPTR password);
ULONG StoreAddGroup(struct AccStore *s, CONST_STRPTR name, ULONG flags, UWORD admin, struct AccGroup **result);
ULONG StoreDeleteGroup(struct AccStore *s, struct AccGroup *g);
ULONG StoreModifyGroup(struct AccStore *s, struct AccGroup *g, CONST_STRPTR name, UWORD gid, UWORD admin, ULONG flags);
ULONG StoreAddMember(struct AccStore *s, struct AccGroup *g, struct AccUser *u);
ULONG StoreRemoveMember(struct AccStore *s, struct AccGroup *g, struct AccUser *u);

int   AccStricmp(CONST_STRPTR a, CONST_STRPTR b);
void  AccCopyName(STRPTR dst, CONST_STRPTR src, ULONG size);

#endif /* ACC_STORE_H */
