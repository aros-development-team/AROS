/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: accounts.library - the API functions.

          Public functions (FD bias 30-114) use the per-task cached link to
          the Accounts Server; the Acc#?() functions (bias 180 upwards) take
          the server entity as their last argument, as the Envoy editors
          expect. Request layouts: re/spec/services-accounts.md §6.3/§6.6.
          Requests are zero-filled and, where the original leaves the first
          record undefined (§9.1), both records carry the caller's data.
*/

#include <proto/exec.h>
#include <string.h>

#include "accounts_intern.h"

/*------------------------------------------------------------------------*/
/* Request builders                                                        */
/*------------------------------------------------------------------------*/

#define REQ_MAX     208

static ULONG Verify(struct AccountsBase *AccountsBase, UBYTE cmd, CONST_STRPTR name, CONST_STRPTR password,
                    struct UserInfo *user, struct Entity *server)
{
    UBYTE buf[ACC_USERSIZE];
    ULONG err;

    if (!name || !user)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    AccPutString(buf, name, ACC_NAMESIZE);
    AccPutString(buf + 40, password, ACC_NAMESIZE);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))))
        AccUnpackUser(user, buf);
    return err;
}

static ULONG UserQuery(struct AccountsBase *AccountsBase, UBYTE cmd, const UBYTE *req, struct UserInfo *user, struct Entity *server)
{
    UBYTE buf[ACC_USERSIZE];
    ULONG err;

    if (!user)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    if (req)
        memcpy(buf, req, 40);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))))
        AccUnpackUser(user, buf);
    return err;
}

static ULONG GroupQuery(struct AccountsBase *AccountsBase, UBYTE cmd, const UBYTE *req, struct GroupInfo *group, struct Entity *server)
{
    UBYTE buf[ACC_GROUPSIZE];
    ULONG err;

    if (!group)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    if (req)
        memcpy(buf, req, 40);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))))
        AccUnpackGroup(group, buf);
    return err;
}

/* G @0, U @40: 207 NextMember and 216 MemberOf */
static ULONG GroupUser(struct AccountsBase *AccountsBase, UBYTE cmd, struct GroupInfo *group, struct UserInfo *user,
                       BOOL copyback, struct Entity *server)
{
    UBYTE buf[ACC_GROUPSIZE + ACC_USERSIZE];
    ULONG err;

    if (!group || !user)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    AccPackGroup(buf, group);
    AccPackUser(buf + 40, user);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))) && copyback)
        AccUnpackUser(user, buf + 40);
    return err;
}

static void PutAuthority(UBYTE *buf, const struct AuthorityInfo *authority)
{
    AccPutString(buf, (CONST_STRPTR)authority->ai_UserName, ACC_NAMESIZE);
    AccPutString(buf + 32, (CONST_STRPTR)authority->ai_Password, ACC_NAMESIZE);
}

/* A, U1 @64, U2 @136 (208 bytes): 200, 201, 214, 218 */
static ULONG UserAdmin(struct AccountsBase *AccountsBase, UBYTE cmd, const struct AuthorityInfo *authority,
                       const struct UserInfo *u1, const struct UserInfo *u2, CONST_STRPTR pw1, CONST_STRPTR pw2,
                       struct UserInfo *result, struct Entity *server)
{
    UBYTE buf[REQ_MAX];
    ULONG err;

    if (!authority || !u1 || !u2)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    PutAuthority(buf, authority);
    AccPackUser(buf + 64, u1);
    AccPutString(buf + 104, pw1, ACC_NAMESIZE);
    AccPackUser(buf + 136, u2);
    AccPutString(buf + 176, pw2, ACC_NAMESIZE);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))) && result)
        AccUnpackUser(result, buf + 64);
    return err;
}

/* A, G1 @64, G2 @104 (144 bytes): 202, 203, 215 */
static ULONG GroupAdmin(struct AccountsBase *AccountsBase, UBYTE cmd, const struct AuthorityInfo *authority,
                        const struct GroupInfo *g1, const struct GroupInfo *g2, struct GroupInfo *result, struct Entity *server)
{
    UBYTE buf[ACC_AUTHSIZE + 2 * ACC_GROUPSIZE];
    ULONG err;

    if (!authority || !g1 || !g2)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    PutAuthority(buf, authority);
    AccPackGroup(buf + 64, g1);
    AccPackGroup(buf + 104, g2);
    if (!(err = AccTransact(AccountsBase, server, cmd, buf, sizeof(buf))) && result)
        AccUnpackGroup(result, buf + 64);
    return err;
}

/* A, G @64, U @104 (176 bytes): 204, 205 */
static ULONG Membership(struct AccountsBase *AccountsBase, UBYTE cmd, const struct AuthorityInfo *authority,
                        const struct GroupInfo *group, const struct UserInfo *user, struct Entity *server)
{
    UBYTE buf[ACC_AUTHSIZE + ACC_GROUPSIZE + ACC_USERSIZE];

    if (!authority || !group || !user)
        return ENVOYERR_NULLPTR;
    memset(buf, 0, sizeof(buf));
    PutAuthority(buf, authority);
    AccPackGroup(buf + 64, group);
    AccPackUser(buf + 104, user);
    return AccTransact(AccountsBase, server, cmd, buf, sizeof(buf));
}

/*------------------------------------------------------------------------*/
/* Public functions                                                        */
/*------------------------------------------------------------------------*/

AROS_LH0(struct UserInfo *, AllocUserInfo,
    struct AccountsBase *, AccountsBase, 5, Accounts)
{
    AROS_LIBFUNC_INIT
    D(bug("accounts.library: AllocUserInfo, base %p\n", AccountsBase));
    return AllocVec(sizeof(struct UserInfo), MEMF_CLEAR | MEMF_PUBLIC);
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct GroupInfo *, AllocGroupInfo,
    struct AccountsBase *, AccountsBase, 6, Accounts)
{
    AROS_LIBFUNC_INIT
    return AllocVec(sizeof(struct GroupInfo), MEMF_CLEAR | MEMF_PUBLIC);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeUserInfo,
    AROS_LHA(struct UserInfo *, user, A0),
    struct AccountsBase *, AccountsBase, 7, Accounts)
{
    AROS_LIBFUNC_INIT
    FreeVec(user);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, FreeGroupInfo,
    AROS_LHA(struct GroupInfo *, group, A0),
    struct AccountsBase *, AccountsBase, 8, Accounts)
{
    AROS_LIBFUNC_INIT
    FreeVec(group);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, VerifyUser,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(STRPTR, password, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    struct AccountsBase *, AccountsBase, 9, Accounts)
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSER, userName, password, user, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, MemberOf,
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    struct AccountsBase *, AccountsBase, 10, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_MEMBEROF, group, user, FALSE, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, NameToUser,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    struct AccountsBase *, AccountsBase, 11, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!userName)
        return ENVOYERR_NULLPTR;
    memset(req, 0, sizeof(req));
    AccPutString(req, userName, ACC_NAMESIZE);
    return UserQuery(AccountsBase, ACCCMD_NAMETOUSER, req, user, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, NameToGroup,
    AROS_LHA(STRPTR, groupName, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    struct AccountsBase *, AccountsBase, 12, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!groupName)
        return ENVOYERR_NULLPTR;
    memset(req, 0, sizeof(req));
    AccPutString(req, groupName, ACC_NAMESIZE);
    return GroupQuery(AccountsBase, ACCCMD_NAMETOGROUP, req, group, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, IDToUser,
    AROS_LHA(UWORD, userID, D0),
    AROS_LHA(struct UserInfo *, user, A0),
    struct AccountsBase *, AccountsBase, 13, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = userID >> 8;
    req[33] = userID;
    return UserQuery(AccountsBase, ACCCMD_IDTOUSER, req, user, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, IDToGroup,
    AROS_LHA(UWORD, groupID, D0),
    AROS_LHA(struct GroupInfo *, group, A0),
    struct AccountsBase *, AccountsBase, 14, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = groupID >> 8;
    req[33] = groupID;
    return GroupQuery(AccountsBase, ACCCMD_IDTOGROUP, req, group, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, NextUser,
    AROS_LHA(struct UserInfo *, user, A0),
    struct AccountsBase *, AccountsBase, 15, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!user)
        return ENVOYERR_NULLPTR;
    AccPackUser(req, user);
    return UserQuery(AccountsBase, ACCCMD_NEXTUSER, req, user, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, NextGroup,
    AROS_LHA(struct GroupInfo *, group, A0),
    struct AccountsBase *, AccountsBase, 16, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!group)
        return ENVOYERR_NULLPTR;
    AccPackGroup(req, group);
    return GroupQuery(AccountsBase, ACCCMD_NEXTGROUP, req, group, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, NextMember,
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    struct AccountsBase *, AccountsBase, 17, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_NEXTMEMBER, group, user, TRUE, NULL);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(STRPTR, ECrypt,
    AROS_LHA(STRPTR, buffer, A0),
    AROS_LHA(STRPTR, password, A1),
    AROS_LHA(STRPTR, username, A2),
    struct AccountsBase *, AccountsBase, 18, Accounts)
{
    AROS_LIBFUNC_INIT
    if (!buffer)
        return NULL;
    return AccECrypt(buffer, password, username);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, VerifyUserCrypt,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(STRPTR, password, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    struct AccountsBase *, AccountsBase, 19, Accounts)
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSERCRYPT, userName, password, user, NULL);
    AROS_LIBFUNC_EXIT
}

/*------------------------------------------------------------------------*/
/* Functions with an explicit server entity (FD bias 180-234)              */
/*------------------------------------------------------------------------*/

AROS_LH4(ULONG, AccVerifyUser,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(STRPTR, password, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 30, Accounts)
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSER, userName, password, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccVerifyUserCrypt,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(STRPTR, password, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 31, Accounts)
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSERCRYPT, userName, password, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccMemberOf,
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 32, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_MEMBEROF, group, user, FALSE, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccNameToUser,
    AROS_LHA(STRPTR, userName, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 33, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!userName)
        return ENVOYERR_NULLPTR;
    memset(req, 0, sizeof(req));
    AccPutString(req, userName, ACC_NAMESIZE);
    return UserQuery(AccountsBase, ACCCMD_NAMETOUSER, req, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccNameToGroup,
    AROS_LHA(STRPTR, groupName, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 34, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!groupName)
        return ENVOYERR_NULLPTR;
    memset(req, 0, sizeof(req));
    AccPutString(req, groupName, ACC_NAMESIZE);
    return GroupQuery(AccountsBase, ACCCMD_NAMETOGROUP, req, group, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccIDToUser,
    AROS_LHA(UWORD, userID, D0),
    AROS_LHA(struct UserInfo *, user, A0),
    AROS_LHA(struct Entity *, server, A1),
    struct AccountsBase *, AccountsBase, 35, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = userID >> 8;
    req[33] = userID;
    return UserQuery(AccountsBase, ACCCMD_IDTOUSER, req, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccIDToGroup,
    AROS_LHA(UWORD, groupID, D0),
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct Entity *, server, A1),
    struct AccountsBase *, AccountsBase, 36, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = groupID >> 8;
    req[33] = groupID;
    return GroupQuery(AccountsBase, ACCCMD_IDTOGROUP, req, group, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, AccNextUser,
    AROS_LHA(struct UserInfo *, user, A0),
    AROS_LHA(struct Entity *, server, A1),
    struct AccountsBase *, AccountsBase, 37, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!user)
        return ENVOYERR_NULLPTR;
    AccPackUser(req, user);
    return UserQuery(AccountsBase, ACCCMD_NEXTUSER, req, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, AccNextGroup,
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct Entity *, server, A1),
    struct AccountsBase *, AccountsBase, 38, Accounts)
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!group)
        return ENVOYERR_NULLPTR;
    AccPackGroup(req, group);
    return GroupQuery(AccountsBase, ACCCMD_NEXTGROUP, req, group, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccNextMember,
    AROS_LHA(struct GroupInfo *, group, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 39, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_NEXTMEMBER, group, user, TRUE, server);
    AROS_LIBFUNC_EXIT
}

/*------------------------------------------------------------------------*/
/* Administration (FD bias 300-348)                                        */
/*------------------------------------------------------------------------*/

AROS_LH4(ULONG, AccModifyUser,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct UserInfo *, newUser, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 50, Accounts)
{
    AROS_LIBFUNC_INIT
    /* U1 = new values, U2 = the record as it is now; U2 := returned U1 */
    return UserAdmin(AccountsBase, ACCCMD_MODIFYUSER, authority, newUser, user, NULL, NULL, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccSetPassword,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(STRPTR, password, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 51, Accounts)
{
    AROS_LIBFUNC_INIT
    /* the target twice, the new password in U2's password field */
    return UserAdmin(AccountsBase, ACCCMD_SETPASSWORD, authority, user, user, NULL, password, NULL, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccModifyGroup,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct GroupInfo *, newGroup, A1),
    AROS_LHA(struct GroupInfo *, group, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 52, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_MODIFYGROUP, authority, newGroup, group, group, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccAddUser,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(STRPTR, password, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 53, Accounts)
{
    AROS_LIBFUNC_INIT
    /* the new record and password in both halves (§9.1); result from U1 */
    return UserAdmin(AccountsBase, ACCCMD_ADDUSER, authority, user, user, password, password, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccDeleteUser,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct UserInfo *, user, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 54, Accounts)
{
    AROS_LIBFUNC_INIT
    return UserAdmin(AccountsBase, ACCCMD_DELETEUSER, authority, user, user, NULL, NULL, NULL, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccAddGroup,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 55, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_ADDGROUP, authority, group, group, group, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, AccDeleteGroup,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    AROS_LHA(struct Entity *, server, A2),
    struct AccountsBase *, AccountsBase, 56, Accounts)
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_DELETEGROUP, authority, group, group, NULL, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccAddMember,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 57, Accounts)
{
    AROS_LIBFUNC_INIT
    return Membership(AccountsBase, ACCCMD_ADDMEMBER, authority, group, user, server);
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, AccRemoveMember,
    AROS_LHA(struct AuthorityInfo *, authority, A0),
    AROS_LHA(struct GroupInfo *, group, A1),
    AROS_LHA(struct UserInfo *, user, A2),
    AROS_LHA(struct Entity *, server, A3),
    struct AccountsBase *, AccountsBase, 58, Accounts)
{
    AROS_LIBFUNC_INIT
    return Membership(AccountsBase, ACCCMD_REMOVEMEMBER, authority, group, user, server);
    AROS_LIBFUNC_EXIT
}
