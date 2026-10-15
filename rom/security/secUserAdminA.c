/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_server.h"

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secUserAdminA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 64, Security)

/*  FUNCTION
        Add, change or remove a user account, or set its password. Only
        root may do this. The password file is rewritten by the library
        (a new file, renamed into place once complete; nothing changes if
        that fails).

    TAGS
        secT_UAOperation - (ULONG) secUA_ADD, secUA_MODIFY, secUA_DELETE or
                           secUA_PASSWORD. Required.
        secT_UserID      - (STRPTR) the account; for secUA_ADD the new user
                           ID. Required.
        secT_NewUserID   - (STRPTR) secUA_MODIFY: rename the account. An
                           account with a password must be given a new one
                           (secT_Password) when renamed, as the stored hash
                           depends on the user ID.
        secT_UID         - (ULONG) user number, 1..65535, unique.
                           Required for secUA_ADD.
        secT_GID         - (ULONG) primary group number; the group must
                           exist. Required for secUA_ADD.
        secT_UserName    - (STRPTR) full name.
        secT_HomeDir     - (STRPTR) home directory.
        secT_Shell       - (STRPTR) shell.
        secT_Password    - (STRPTR) secUA_ADD, secUA_PASSWORD (and a
                           rename): the clear-text password; "" for none.
                           secUA_PASSWORD also unlocks a locked account.

        For secUA_MODIFY only the fields given are changed. Fields may not
        contain '|' or control characters; user IDs neither ':' nor spaces.

    RESULT
        secUAERR_OK, or a secUAERR_#? code.

    NOTES
        root cannot be deleted, renamed or given another uid.

    SEE ALSO
        secAuthControlA(), secGetUserInfo(), SYS:System/Accounts

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secUARequest req;
    ULONG res;

    if (!secBase->sec_AfterDOSDone)
        return secUAERR_UNAVAILABLE;
    if (!CallerIsRoot(secBase))
        return secUAERR_NOTROOT;

    memset(&req, 0, sizeof(req));
    req.Operation = GetTagData(secT_UAOperation, 0, taglist);
    req.UserID = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    req.NewUserID = (CONST_STRPTR)GetTagData(secT_NewUserID, 0, taglist);
    req.uid = FindTagItem(secT_UID, taglist) ? (LONG)GetTagData(secT_UID, 0, taglist) : -1;
    req.gid = FindTagItem(secT_GID, taglist) ? (LONG)GetTagData(secT_GID, 0, taglist) : -1;
    req.UserName = (CONST_STRPTR)GetTagData(secT_UserName, 0, taglist);
    req.HomeDir = (CONST_STRPTR)GetTagData(secT_HomeDir, 0, taglist);
    req.Shell = (CONST_STRPTR)GetTagData(secT_Shell, 0, taglist);
    req.Password = (CONST_STRPTR)GetTagData(secT_Password, 0, taglist);

    res = (ULONG)SendServerPacket(secBase, secSAction_UserAdmin, (SIPTR)&req, 0, 0, 0);
    return secBase->ServerPort ? res : secUAERR_UNAVAILABLE;

    AROS_LIBFUNC_EXIT
} /* secUserAdminA */
