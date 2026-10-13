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

/*****************************************************************************

    NAME */
        AROS_LH0(struct UserInfo *, AllocUserInfo,

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 5, Accounts)
/*  FUNCTION
        Allocate a zero-filled struct UserInfo for account queries.

    INPUTS
        None.

    RESULT
        A caller-owned pointer, or NULL if allocation fails. Release a
        successful allocation with FreeUserInfo().

    NOTES
        Storage is allocated with MEMF_PUBLIC | MEMF_CLEAR. This allocates
        only a local record; it does not create a server-side user, contact
        the Accounts Server or reserve an account ID. The zero-filled record
        can start NextUser() enumeration. Call from task context.

    SEE ALSO
        FreeUserInfo(), NextUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    D(bug("accounts.library: AllocUserInfo, base %p\n", AccountsBase));
    return AllocVec(sizeof(struct UserInfo), MEMF_CLEAR | MEMF_PUBLIC);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(struct GroupInfo *, AllocGroupInfo,

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 6, Accounts)
/*  FUNCTION
        Allocate a zero-filled struct GroupInfo for account queries.

    INPUTS
        None.

    RESULT
        A caller-owned pointer, or NULL if allocation fails. Release a
        successful allocation with FreeGroupInfo().

    NOTES
        Storage is allocated with MEMF_PUBLIC | MEMF_CLEAR. This allocates
        only a local record; it does not create a server-side group, contact
        the Accounts Server or reserve an account ID. The zero-filled record
        can start NextGroup() enumeration. Call from task context.

    SEE ALSO
        FreeGroupInfo(), NextGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return AllocVec(sizeof(struct GroupInfo), MEMF_CLEAR | MEMF_PUBLIC);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeUserInfo,

/*  SYNOPSIS */
            AROS_LHA(struct UserInfo *, user, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 7, Accounts)
/*  FUNCTION
        Free a record allocated by AllocUserInfo().

    INPUTS
        user - Pointer returned by AllocUserInfo(), or NULL.

    RESULT
        None. NULL has no effect.

    NOTES
        Finish all uses of the record before freeing it. Its pointer is
        invalid afterwards. This releases local storage only; it does not
        delete the corresponding server-side user or contact the server.
        Call from task context.

    SEE ALSO
        AllocUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    FreeVec(user);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeGroupInfo,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 8, Accounts)
/*  FUNCTION
        Free a record allocated by AllocGroupInfo().

    INPUTS
        group - Pointer returned by AllocGroupInfo(), or NULL.

    RESULT
        None. NULL has no effect.

    NOTES
        Finish all uses of the record before freeing it. Its pointer is
        invalid afterwards. This releases local storage only; it does not
        delete the corresponding server-side group or contact the server.
        Call from task context.

    SEE ALSO
        AllocGroupInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    FreeVec(group);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, VerifyUser,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(STRPTR, password, A1),
            AROS_LHA(struct UserInfo *, user, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 9, Accounts)
/*  FUNCTION
        Ask the Accounts Server to authenticate a user and, on success,
        return the user's record. This does not change the caller's local
        credentials or establish a login session.

    INPUTS
        userName - Non-NULL NUL-terminated username.
        password - NUL-terminated clear-text password,
                   or NULL for an empty value.
        user - Non-NULL writable struct UserInfo.

    RESULT
        Zero on success, with *user replaced by the returned record.
        On any error, *user is unchanged. The AROS server reports
        ENVOYERR_UNKNOWNUSER for an unknown/locked user or failed
        authentication; these cases are not distinguished.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The password is placed in the request as clear text by this API.
        The AROS server applies its configured authentication policy.
        An unlocked account without UFLAGF_NeedsPassword accepts any
        password token.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        ECrypt(), VerifyUserCrypt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSER, userName, password, user, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, MemberOf,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct UserInfo *, user, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 10, Accounts)
/*  FUNCTION
        Check whether a user is a member of a group on the Accounts Server.

    INPUTS
        group - Non-NULL readable current GroupInfo record.
        user - Non-NULL readable current UserInfo record.

    RESULT
        Zero means membership was found. With the AROS server,
        ENVOYERR_UNKNOWNMEMBER means no membership; ENVOYERR_UNKNOWNGROUP
        or ENVOYERR_UNKNOWNUSER means the supplied record did not match.
        Neither caller record is modified.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        Supply complete, unchanged records from successful lookups.
        The AROS server checks all record fields, not just the IDs. A
        stale record is not interchangeable with a current one. Membership
        is checked against the server's membership list.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        NextMember(), NameToUser(), NameToGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_MEMBEROF, group, user, FALSE, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, NameToUser,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(struct UserInfo *, user, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 11, Accounts)
/*  FUNCTION
        Look up a user record by name.

    INPUTS
        userName - Non-NULL NUL-terminated user name.
        user - Non-NULL writable struct UserInfo.

    RESULT
        Zero on success, with *user replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNUSER when no match exists.
        On any error, *user is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server compares names case-insensitively. This is a
        lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        IDToUser(), NextUser()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, NameToGroup,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, groupName, A0),
            AROS_LHA(struct GroupInfo *, group, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 12, Accounts)
/*  FUNCTION
        Look up a group record by name.

    INPUTS
        groupName - Non-NULL NUL-terminated group name.
        group - Non-NULL writable struct GroupInfo.

    RESULT
        Zero on success, with *group replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP when no match exists.
        On any error, *group is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server compares names case-insensitively. This is a
        lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        IDToGroup(), NextGroup()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, IDToUser,

/*  SYNOPSIS */
            AROS_LHA(UWORD, userID, D0),
            AROS_LHA(struct UserInfo *, user, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 13, Accounts)
/*  FUNCTION
        Look up a user record by numeric ID.

    INPUTS
        userID - 16-bit numeric user ID.
        user - Non-NULL writable struct UserInfo.

    RESULT
        Zero on success, with *user replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNUSER when no match exists.
        On any error, *user is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server returns the first record with this ID in its
        name-ordered list. IDs can be shared by multiple records in the
        backend; an ID lookup is therefore not a unique-name lookup.
        This is a lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        NameToUser(), NextUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = userID >> 8;
    req[33] = userID;
    return UserQuery(AccountsBase, ACCCMD_IDTOUSER, req, user, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, IDToGroup,

/*  SYNOPSIS */
            AROS_LHA(UWORD, groupID, D0),
            AROS_LHA(struct GroupInfo *, group, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 14, Accounts)
/*  FUNCTION
        Look up a group record by numeric ID.

    INPUTS
        groupID - 16-bit numeric group ID.
        group - Non-NULL writable struct GroupInfo.

    RESULT
        Zero on success, with *group replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP when no match exists.
        On any error, *group is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server returns the first record with this ID in its
        name-ordered list. IDs can be shared by multiple records in the
        backend; an ID lookup is therefore not a unique-name lookup.
        This is a lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        NameToGroup(), NextGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = groupID >> 8;
    req[33] = groupID;
    return GroupQuery(AccountsBase, ACCCMD_IDTOGROUP, req, group, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, NextUser,

/*  SYNOPSIS */
            AROS_LHA(struct UserInfo *, user, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 15, Accounts)
/*  FUNCTION
        Enumerate user records maintained by the Accounts Server.

    INPUTS
        user - Non-NULL read/write struct UserInfo. Zero the whole record
                before the first call. For each subsequent call, pass the
                complete record returned by the previous successful call.

    RESULT
        Zero on success, replacing *user with the next record.
        ENVOYERR_LASTUSER at the end of the AROS server's list, or if the
        previous record no longer matches. On any error, including end
        of list, the caller's record remains unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server starts when the name is empty and ID is zero.
        Otherwise it matches name, ID and all remaining fields before
        advancing in case-insensitive name order. Editing the cursor record
        or changing the server database can end enumeration early. There
        is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        NameToUser(), AllocUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!user)
        return ENVOYERR_NULLPTR;
    AccPackUser(req, user);
    return UserQuery(AccountsBase, ACCCMD_NEXTUSER, req, user, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, NextGroup,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 16, Accounts)
/*  FUNCTION
        Enumerate group records maintained by the Accounts Server.

    INPUTS
        group - Non-NULL read/write struct GroupInfo. Zero the whole record
                before the first call. For each subsequent call, pass the
                complete record returned by the previous successful call.

    RESULT
        Zero on success, replacing *group with the next record.
        ENVOYERR_LASTGROUP at the end of the AROS server's list, or if the
        previous record no longer matches. On any error, including end
        of list, the caller's record remains unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server starts when the name is empty and ID is zero.
        Otherwise it matches name, ID and all remaining fields before
        advancing in case-insensitive name order. Editing the cursor record
        or changing the server database can end enumeration early. There
        is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        NameToGroup(), AllocGroupInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!group)
        return ENVOYERR_NULLPTR;
    AccPackGroup(req, group);
    return GroupQuery(AccountsBase, ACCCMD_NEXTGROUP, req, group, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, NextMember,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct UserInfo *, user, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 17, Accounts)
/*  FUNCTION
        Enumerate the users belonging to a group on the Accounts Server.

    INPUTS
        group - Non-NULL readable current GroupInfo record.
        user - Non-NULL read/write UserInfo cursor. Zero the whole record
               to start; subsequently use the previous successful result.

    RESULT
        Zero on success, replacing *user with the next member's record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP for a mismatched
        group or ENVOYERR_LASTMEMBER at the end, for a mismatched cursor,
        or when the cursor no longer belongs to the group. On any error,
        *user is unchanged. The group record is never modified.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server checks complete records and enumerates the group's
        stored member IDs in ascending order, skipping IDs without a user
        record. If an ID is shared by users, its first name-ordered record
        is returned. There is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        MemberOf(), NameToGroup(), AllocUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_NEXTMEMBER, group, user, TRUE, NULL);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(STRPTR, ECrypt,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, buffer, A0),
            AROS_LHA(STRPTR, password, A1),
            AROS_LHA(STRPTR, username, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 18, Accounts)
/*  FUNCTION
        Compute the legacy, username-dependent Envoy password token. The
        first 12 password and username bytes participate; ASCII uppercase
        letters in the username are folded to lowercase. The result has
        11 characters followed by a NUL byte.

    INPUTS
        buffer - Writable storage for at least 12 bytes, or NULL.
        password - NUL-terminated clear-text password, or NULL for empty.
        username - NUL-terminated username, or NULL for empty.

    RESULT
        buffer after writing the token, or NULL when buffer is NULL.
        No output-size check is performed.

    NOTES
        No allocation, server request or wait is performed. All buffers
        remain caller-owned. This is the Envoy compatibility token format,
        not reversible encryption. Bytes after the first 12 do not change
        the token, but the input strings must still be NUL-terminated.

    SEE ALSO
        VerifyUserCrypt(), AccVerifyUserCrypt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    if (!buffer)
        return NULL;
    return AccECrypt(buffer, password, username);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, VerifyUserCrypt,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(STRPTR, password, A1),
            AROS_LHA(struct UserInfo *, user, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 19, Accounts)
/*  FUNCTION
        Ask the Accounts Server to authenticate a user and, on success,
        return the user's record. This does not change the caller's local
        credentials or establish a login session.

    INPUTS
        userName - Non-NULL NUL-terminated username.
        password - NUL-terminated 11-character token produced by ECrypt() for this username,
                   or NULL for an empty value.
        user - Non-NULL writable struct UserInfo.

    RESULT
        Zero on success, with *user replaced by the returned record.
        On any error, *user is unchanged. The AROS server reports
        ENVOYERR_UNKNOWNUSER for an unknown/locked user or failed
        authentication; these cases are not distinguished.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server applies its configured authentication policy.
        An unlocked account without UFLAGF_NeedsPassword accepts any
        password token. A crypt token is sent as given, not computed by
        this call.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

        The server is the per-task cached "Accounts Server" entity, on the
        host selected by Envoy/AccountsServer through GetVar(), or local
        when no host is selected. The library owns this cached link.

    SEE ALSO
        ECrypt(), VerifyUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSERCRYPT, userName, password, user, NULL);
    AROS_LIBFUNC_EXIT
}

/*------------------------------------------------------------------------*/
/* Functions with an explicit server entity (FD bias 180-234)              */
/*------------------------------------------------------------------------*/

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccVerifyUser,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(STRPTR, password, A1),
            AROS_LHA(struct UserInfo *, user, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 30, Accounts)
/*  FUNCTION
        Ask the Accounts Server to authenticate a user and, on success,
        return the user's record. This does not change the caller's local
        credentials or establish a login session.

    INPUTS
        userName - Non-NULL NUL-terminated username.
        password - NUL-terminated clear-text password,
                   or NULL for an empty value.
        user - Non-NULL writable struct UserInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *user replaced by the returned record.
        On any error, *user is unchanged. The AROS server reports
        ENVOYERR_UNKNOWNUSER for an unknown/locked user or failed
        authentication; these cases are not distinguished.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The password is placed in the request as clear text by this API.
        The AROS server applies its configured authentication policy.
        An unlocked account without UFLAGF_NeedsPassword accepts any
        password token.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        ECrypt(), AccVerifyUserCrypt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSER, userName, password, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccVerifyUserCrypt,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(STRPTR, password, A1),
            AROS_LHA(struct UserInfo *, user, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 31, Accounts)
/*  FUNCTION
        Ask the Accounts Server to authenticate a user and, on success,
        return the user's record. This does not change the caller's local
        credentials or establish a login session.

    INPUTS
        userName - Non-NULL NUL-terminated username.
        password - NUL-terminated 11-character token produced by ECrypt() for this username,
                   or NULL for an empty value.
        user - Non-NULL writable struct UserInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *user replaced by the returned record.
        On any error, *user is unchanged. The AROS server reports
        ENVOYERR_UNKNOWNUSER for an unknown/locked user or failed
        authentication; these cases are not distinguished.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server applies its configured authentication policy.
        An unlocked account without UFLAGF_NeedsPassword accepts any
        password token. A crypt token is sent as given, not computed by
        this call.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        ECrypt(), AccVerifyUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Verify(AccountsBase, ACCCMD_VERIFYUSERCRYPT, userName, password, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccMemberOf,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 32, Accounts)
/*  FUNCTION
        Check whether a user is a member of a group on the Accounts Server.

    INPUTS
        group - Non-NULL readable current GroupInfo record.
        user - Non-NULL readable current UserInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero means membership was found. With the AROS server,
        ENVOYERR_UNKNOWNMEMBER means no membership; ENVOYERR_UNKNOWNGROUP
        or ENVOYERR_UNKNOWNUSER means the supplied record did not match.
        Neither caller record is modified.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        Supply complete, unchanged records from successful lookups.
        The AROS server checks all record fields, not just the IDs. A
        stale record is not interchangeable with a current one. Membership
        is checked against the server's membership list.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNextMember(), AccNameToUser(), AccNameToGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_MEMBEROF, group, user, FALSE, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccNameToUser,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, userName, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 33, Accounts)
/*  FUNCTION
        Look up a user record by name.

    INPUTS
        userName - Non-NULL NUL-terminated user name.
        user - Non-NULL writable struct UserInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *user replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNUSER when no match exists.
        On any error, *user is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server compares names case-insensitively. This is a
        lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccIDToUser(), AccNextUser()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccNameToGroup,

/*  SYNOPSIS */
            AROS_LHA(STRPTR, groupName, A0),
            AROS_LHA(struct GroupInfo *, group, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 34, Accounts)
/*  FUNCTION
        Look up a group record by name.

    INPUTS
        groupName - Non-NULL NUL-terminated group name.
        group - Non-NULL writable struct GroupInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *group replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP when no match exists.
        On any error, *group is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server compares names case-insensitively. This is a
        lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccIDToGroup(), AccNextGroup()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccIDToUser,

/*  SYNOPSIS */
            AROS_LHA(UWORD, userID, D0),
            AROS_LHA(struct UserInfo *, user, A0),
            AROS_LHA(struct Entity *, server, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 35, Accounts)
/*  FUNCTION
        Look up a user record by numeric ID.

    INPUTS
        userID - 16-bit numeric user ID.
        user - Non-NULL writable struct UserInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *user replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNUSER when no match exists.
        On any error, *user is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server returns the first record with this ID in its
        name-ordered list. IDs can be shared by multiple records in the
        backend; an ID lookup is therefore not a unique-name lookup.
        This is a lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToUser(), AccNextUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = userID >> 8;
    req[33] = userID;
    return UserQuery(AccountsBase, ACCCMD_IDTOUSER, req, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccIDToGroup,

/*  SYNOPSIS */
            AROS_LHA(UWORD, groupID, D0),
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct Entity *, server, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 36, Accounts)
/*  FUNCTION
        Look up a group record by numeric ID.

    INPUTS
        groupID - 16-bit numeric group ID.
        group - Non-NULL writable struct GroupInfo.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, with *group replaced by the returned record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP when no match exists.
        On any error, *group is unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server returns the first record with this ID in its
        name-ordered list. IDs can be shared by multiple records in the
        backend; an ID lookup is therefore not a unique-name lookup.
        This is a lookup, not authentication.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToGroup(), AccNextGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    memset(req, 0, sizeof(req));
    req[32] = groupID >> 8;
    req[33] = groupID;
    return GroupQuery(AccountsBase, ACCCMD_IDTOGROUP, req, group, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, AccNextUser,

/*  SYNOPSIS */
            AROS_LHA(struct UserInfo *, user, A0),
            AROS_LHA(struct Entity *, server, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 37, Accounts)
/*  FUNCTION
        Enumerate user records maintained by the Accounts Server.

    INPUTS
        user - Non-NULL read/write struct UserInfo. Zero the whole record
                before the first call. For each subsequent call, pass the
                complete record returned by the previous successful call.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *user with the next record.
        ENVOYERR_LASTUSER at the end of the AROS server's list, or if the
        previous record no longer matches. On any error, including end
        of list, the caller's record remains unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server starts when the name is empty and ID is zero.
        Otherwise it matches name, ID and all remaining fields before
        advancing in case-insensitive name order. Editing the cursor record
        or changing the server database can end enumeration early. There
        is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToUser(), AllocUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!user)
        return ENVOYERR_NULLPTR;
    AccPackUser(req, user);
    return UserQuery(AccountsBase, ACCCMD_NEXTUSER, req, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(ULONG, AccNextGroup,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct Entity *, server, A1),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 38, Accounts)
/*  FUNCTION
        Enumerate group records maintained by the Accounts Server.

    INPUTS
        group - Non-NULL read/write struct GroupInfo. Zero the whole record
                before the first call. For each subsequent call, pass the
                complete record returned by the previous successful call.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *group with the next record.
        ENVOYERR_LASTGROUP at the end of the AROS server's list, or if the
        previous record no longer matches. On any error, including end
        of list, the caller's record remains unchanged.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server starts when the name is empty and ID is zero.
        Otherwise it matches name, ID and all remaining fields before
        advancing in case-insensitive name order. Editing the cursor record
        or changing the server database can end enumeration early. There
        is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToGroup(), AllocGroupInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE req[40];
    if (!group)
        return ENVOYERR_NULLPTR;
    AccPackGroup(req, group);
    return GroupQuery(AccountsBase, ACCCMD_NEXTGROUP, req, group, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccNextMember,

/*  SYNOPSIS */
            AROS_LHA(struct GroupInfo *, group, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 39, Accounts)
/*  FUNCTION
        Enumerate the users belonging to a group on the Accounts Server.

    INPUTS
        group - Non-NULL readable current GroupInfo record.
        user - Non-NULL read/write UserInfo cursor. Zero the whole record
               to start; subsequently use the previous successful result.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *user with the next member's record.
        The AROS server returns ENVOYERR_UNKNOWNGROUP for a mismatched
        group or ENVOYERR_LASTMEMBER at the end, for a mismatched cursor,
        or when the cursor no longer belongs to the group. On any error,
        *user is unchanged. The group record is never modified.

        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server checks complete records and enumerates the group's
        stored member IDs in ascending order, skipping IDs without a user
        record. If an ID is shared by users, its first name-ordered record
        is returned. There is no snapshot across calls.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccMemberOf(), AccNameToGroup(), AllocUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupUser(AccountsBase, ACCCMD_NEXTMEMBER, group, user, TRUE, server);
    AROS_LIBFUNC_EXIT
}

/*------------------------------------------------------------------------*/
/* Administration (FD bias 300-348)                                        */
/*------------------------------------------------------------------------*/

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccModifyUser,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct UserInfo *, newUser, A1),
            AROS_LHA(struct UserInfo *, user, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 50, Accounts)
/*  FUNCTION
        Request changes to an existing user record.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        newUser - Non-NULL readable proposed UserInfo record.
        user - Non-NULL read/write complete current UserInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *user with the returned record.
        On error, *user is unchanged. Separate newUser storage is not
        modified.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNUSER means a stale/missing target;
        ACCERROR_USEREXISTS means a conflicting name.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server matches the current record in full. AdminAll may
        change the name, primary group and password-required state; a user
        with AdminName may rename themselves. The numeric user ID is not
        changed. An empty proposed name leaves the existing name unchanged.
        Clearing NeedsPassword removes the stored password and unlocks the
        account. Other flags are derived from server policy, not copied as
        arbitrary privilege bits; setting NeedsPassword does not create a
        password. Use the returned record, not the proposal, afterwards.
        Renaming retains any existing password token. Since ECrypt()
        includes the username, reset the password after a rename when
        using the stored Envoy-token authentication backend.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToUser(), AccSetPassword()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* U1 = new values, U2 = the record as it is now; U2 := returned U1 */
    return UserAdmin(AccountsBase, ACCCMD_MODIFYUSER, authority, newUser, user, NULL, NULL, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccSetPassword,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(STRPTR, password, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 51, Accounts)
/*  FUNCTION
        Set or remove a user password on the Accounts Server.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        user - Non-NULL readable complete current UserInfo record.
        password - NUL-terminated new clear-text password, or NULL/empty
                   to remove the password.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success. The caller's user record is not modified.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNUSER means the complete target record did not match.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server permits AdminAll, or a user changing their own
        password with AdminPassword. Setting a password also unlocks the
        account; NULL or an empty string removes its password. Requery the
        user record afterwards because flags may have changed. The new
        password and authority password are sent as clear text by this API.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccVerifyUser(), AccNameToUser(), ECrypt()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* the target twice, the new password in U2's password field */
    return UserAdmin(AccountsBase, ACCCMD_SETPASSWORD, authority, user, user, NULL, password, NULL, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccModifyGroup,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct GroupInfo *, newGroup, A1),
            AROS_LHA(struct GroupInfo *, group, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 52, Accounts)
/*  FUNCTION
        Request changes to an existing group record.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        newGroup - Non-NULL readable proposed GroupInfo record.
        group - Non-NULL read/write complete current GroupInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *group with the returned record.
        On error, *group is unchanged. Separate newGroup storage is not
        modified.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNGROUP means a stale/missing target;
        ACCERROR_GROUPEXISTS means a conflicting name.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server permits AdminAll or the group's recorded admin.
        It updates name, group ID, admin ID and flags; an empty proposed
        name leaves the existing name unchanged. The full current record
        must match. Changing the group ID does not rewrite users' primary
        group IDs, and the backend does not check for a conflicting numeric
        group ID. Coordinate such changes explicitly.
        Group flags are not represented in the AROS backend's group file
        and are not preserved by saving and reloading that file.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToGroup(), AccModifyUser()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_MODIFYGROUP, authority, newGroup, group, group, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccAddUser,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(STRPTR, password, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 53, Accounts)
/*  FUNCTION
        Create a user on the Accounts Server.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        user - Non-NULL read/write proposed UserInfo record.
        password - NUL-terminated initial clear-text password, or NULL
                   for no initial password.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *user with the assigned record.
        On error, the caller's record is unchanged.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ACCERROR_USEREXISTS means an unavailable name;
        ACCERROR_NOFREEUSERS means the server cannot assign another ID.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server requires AdminAll. It assigns the user ID rather
        than using the proposed ID. An empty name requests an available
        NewUser, NewUser2, ... name. The proposed primary group is retained.
        A password is stored only when NeedsPassword is set and password
        is nonempty. Other privilege flags are derived from server policy;
        the returned record is authoritative. No local UserInfo storage is
        allocated for the caller.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AllocUserInfo(), AccDeleteUser(), AccSetPassword()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* the new record and password in both halves (§9.1); result from U1 */
    return UserAdmin(AccountsBase, ACCCMD_ADDUSER, authority, user, user, password, password, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccDeleteUser,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct UserInfo *, user, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 54, Accounts)
/*  FUNCTION
        Delete a user account and its stored group memberships.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        user - Non-NULL readable complete current UserInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success. The caller's record is not modified or freed.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNUSER means a stale/missing target.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server requires AdminAll, matches all fields of the target
        and refuses deletion of protected root IDs. The backend removes
        memberships and the user record; it does not remove home-directory
        contents or update a group's administrator ID. Any surviving local
        copy of the record is stale after success.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToUser(), AccAddUser(), FreeUserInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return UserAdmin(AccountsBase, ACCCMD_DELETEUSER, authority, user, user, NULL, NULL, NULL, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccAddGroup,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct GroupInfo *, group, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 55, Accounts)
/*  FUNCTION
        Create a group on the Accounts Server.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        group - Non-NULL read/write proposed GroupInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success, replacing *group with the assigned record.
        On error, the caller's record is unchanged.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ACCERROR_GROUPEXISTS means an unavailable name;
        ACCERROR_NOFREEGROUPS means the server cannot assign another ID.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server requires AdminGroups or AdminAll. It assigns a
        new group ID and uses the authorizer's user ID as administrator,
        ignoring those proposed fields. Flags are copied. Group flags are
        not stored in the AROS backend's group file. An empty name
        requests an available NewGroup, NewGroup2, ... name. The new group
        starts with no explicit members. No local GroupInfo is allocated
        for the caller.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AllocGroupInfo(), AccAddMember(), AccDeleteGroup()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_ADDGROUP, authority, group, group, group, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, AccDeleteGroup,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct GroupInfo *, group, A1),
            AROS_LHA(struct Entity *, server, A2),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 56, Accounts)
/*  FUNCTION
        Delete a group and its explicit membership list.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        group - Non-NULL readable complete current GroupInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success. The caller's record is not modified or freed.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNGROUP means a stale/missing target.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server permits AdminAll or the group's recorded admin and
        requires the complete target record to match. Deleting the group
        does not delete its users or rewrite their primary group IDs.
        Any surviving local copy of the group record is stale after success.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccNameToGroup(), AccAddGroup(), FreeGroupInfo()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return GroupAdmin(AccountsBase, ACCCMD_DELETEGROUP, authority, group, group, NULL, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccAddMember,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct GroupInfo *, group, A1),
            AROS_LHA(struct UserInfo *, user, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 57, Accounts)
/*  FUNCTION
        Add a user to a group's explicit membership list.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        group - Non-NULL readable complete current GroupInfo record.
        user - Non-NULL readable complete current UserInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success. Neither caller record is modified.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNGROUP or ENVOYERR_UNKNOWNUSER means a stale/missing
        record. ACCERROR_USEREXISTS means membership already existed.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server permits AdminAll or the group's recorded admin
        and matches all fields of both records. Membership is keyed by
        user ID; users sharing an ID share this membership. This operation
        does not change the user's primary group field.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccMemberOf(), AccRemoveMember()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Membership(AccountsBase, ACCCMD_ADDMEMBER, authority, group, user, server);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(ULONG, AccRemoveMember,

/*  SYNOPSIS */
            AROS_LHA(struct AuthorityInfo *, authority, A0),
            AROS_LHA(struct GroupInfo *, group, A1),
            AROS_LHA(struct UserInfo *, user, A2),
            AROS_LHA(struct Entity *, server, A3),

/*  LOCATION */
            struct AccountsBase *, AccountsBase, 58, Accounts)
/*  FUNCTION
        Remove a user from a group's explicit membership list.

    INPUTS
        authority - Non-NULL AuthorityInfo containing NUL-terminated
                    ai_UserName and clear-text ai_Password of the authorizer.
        group - Non-NULL readable complete current GroupInfo record.
        user - Non-NULL readable complete current UserInfo record.
        server - Borrowed Accounts Server entity obtained through NIPC,
                 valid until return, or NULL to use the per-task cached
                 server selected by Envoy/AccountsServer (local by default).

    RESULT
        Zero on success. Neither caller record is modified.

        ACCERROR_NOAUTHORITY means failed authorizer authentication;
        ACCERROR_NOPRIVS means insufficient rights or a read-only backend.
        ENVOYERR_UNKNOWNGROUP or ENVOYERR_UNKNOWNUSER means a stale/missing
        record. ENVOYERR_UNKNOWNMEMBER means membership was absent.
        ENVOYERR_NULLPTR denotes a missing required pointer;
        ENVOYERR_NORESOURCES denotes an allocation or context failure.
        Discovery, NIPC and server errors propagate unchanged; see
        <envoy/errors.h> and <envoy/accounts.h>.

    NOTES
        The AROS server permits AdminAll or the group's recorded admin
        and matches all fields of both records. Membership is keyed by
        user ID; users sharing an ID share this membership. This operation
        does not change the user's primary group field.

        Requires an authenticated authorizer and writable backend;
        a read-only usergroup.library backend reports ACCERROR_NOPRIVS.
        Success confirms the server's in-memory change, not durable storage:
        later file-save errors are not reported to the caller. A transport
        error does not prove that no change occurred. Query the server
        before retrying a destructive request.

        Synchronous task-context call; may block. The requested five-
        second NIPC transaction timeout does not bound discovery, local
        server waits or total execution time. Caller-owned data must remain
        valid until return. Strings must be NUL-terminated; wire string
        fields retain at most 31 characters. Do not close accounts.library
        from this task while the request is pending.

    SEE ALSO
        AccMemberOf(), AccAddMember()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return Membership(AccountsBase, ACCCMD_REMOVEMEMBER, authority, group, user, server);
    AROS_LIBFUNC_EXIT
}
