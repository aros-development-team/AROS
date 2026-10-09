/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_userinfo.h"

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, secQueryUserA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 62, Security)

/*  FUNCTION
        Answer questions about a user's account that secGetUserInfo() does
        not: whether a password is set, and whether the user may change it.

    TAGS
        secT_UserID            - (STRPTR) the user. Required.
        secT_HasPassword       - (BOOL *) set to TRUE if a password is set.
        secT_MayChangePassword - (BOOL *) set to TRUE if secPasswd() would
                                 be allowed for this user (PASSWDUIDLEVEL /
                                 PASSWDGIDLEVEL in Security.config).

    RESULT
        TRUE if the user exists, FALSE otherwise (or on an unconfigured
        system).

    SEE ALSO
        secGetUserInfo(), secVerifyUserA(), secPasswd()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR userid = (CONST_STRPTR)GetTagData(secT_UserID, 0, taglist);
    BOOL *haspw = (BOOL *)GetTagData(secT_HasPassword, 0, taglist);
    BOOL *maychange = (BOOL *)GetTagData(secT_MayChangePassword, 0, taglist);
    struct secPrivUserInfo *info;
    BOOL found = FALSE;

    if (haspw)
        *haspw = FALSE;
    if (maychange)
        *maychange = FALSE;
    if (!userid || !userid[0] || !secBase->sec_AfterDOSDone || !secBase->Configured)
        return FALSE;

    if ((info = (struct secPrivUserInfo *)secAllocUserInfo()))
    {
        strncpy(info->Pub.UserID, userid, secUSERIDSIZE - 1);
        if (secGetUserInfo(&info->Pub, secKeyType_UserID))
        {
            found = TRUE;
            if (haspw)
                *haspw = info->Password;
            if (maychange)
                *maychange = (info->Pub.uid <= secBase->Config.PasswduidLevel) ||
                             (info->Pub.gid <= secBase->Config.PasswdgidLevel);
        }
        secFreeUserInfo(&info->Pub);
    }
    return found;

    AROS_LIBFUNC_EXIT
} /* secQueryUserA */
