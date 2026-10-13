/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library built-in module "security": accounts kept by
          security.library.

          auth:     secVerifyUserA() - the comparison is made by the
                    library's authentication process; this module never
                    sees a hash. Argument "hashed" allows a PAMTOK_ENVOY
                    token (which the library still accepts only with
                    HASHEDLOGIN=1 in Security.config). A login ticket is
                    kept for setcred.
          setcred:  secLoginA() with the ticket on PAM_AROS_TASK
                    (PAM_ESTABLISH_CRED); secLogoutA() for PAM_DELETE_CRED.
          account:  the user must exist.
          password: secPasswd() for the caller's own account.
          session:  nothing yet.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <proto/pam.h>
#include <string.h>

#include "pam_intern.h"

#define DATA_TICKET     "security.ticket"
#define DATA_LOGGEDIN   "security.loggedin"

static struct Library *OpenSec(void)
{
    struct Library *base = OpenLibrary(SECURITYNAME, 45);

    if (base && base->lib_Version == 45 && base->lib_Revision < 13)
    {
        CloseLibrary(base);
        base = NULL;
    }
    return base;
}

static LONG MapVerify(ULONG r)
{
    switch (r)
    {
    case secVERIFY_OK:           return PAM_SUCCESS;
    case secVERIFY_LOCKED:       return PAM_MAXTRIES;
    case secVERIFY_BUSY:         return PAM_TRY_AGAIN;
    case secVERIFY_NOTSUPPORTED: return PAM_AUTHINFO_UNAVAIL;
    case secVERIFY_UNAVAILABLE:  return PAM_AUTHINFO_UNAVAIL;
    case secVERIFY_BADARGS:      return PAM_AUTH_ERR;
    default:                     return PAM_AUTH_ERR;
    }
}

static LONG SecAuthenticate(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user, tok;
    struct secUserInfo *info;
    ULONG ticket = 0, r;
    LONG res;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &tok, NULL)) != PAM_SUCCESS)
        return res;
    if ((flags & PAM_DISALLOW_NULL_AUTHTOK) && !tok[0])
        return PAM_AUTH_ERR;
    if (h->AuthTokFormat == PAMTOK_ENVOY && !PamArgPresent(argc, argv, "hashed"))
        return PAM_AUTHINFO_UNAVAIL;

    if (!(secBase = OpenSec()))
        return PAM_AUTHINFO_UNAVAIL;
    if (!(info = secAllocUserInfo()))
    {
        CloseLibrary(secBase);
        return PAM_BUF_ERR;
    }
    {
        struct TagItem tags[] =
        {
            { secT_UserID,      (IPTR)user      },
            { h->AuthTokFormat == PAMTOK_ENVOY ? secT_PasswordHash : secT_Password, (IPTR)tok },
            { secT_HashType,    secHASH_ACRYPT_LC },
            { secT_Service,     (IPTR)h->Service },
            { secT_RemoteHost,  (IPTR)h->RHost  },
            { secT_UserInfo,    (IPTR)info      },
            { secT_Ticket,      (IPTR)&ticket   },
            { TAG_DONE,         0               }
        };
        r = secVerifyUserA(tags);
    }
    res = MapVerify(r);
    if (res == PAM_SUCCESS)
    {
        /* the stored spelling of the user ID */
        PamSetItem(h, PAM_USER, info->UserID);
        PamSetData(h, DATA_TICKET, (APTR)(IPTR)ticket, NULL);
    }
    secFreeUserInfo(info);
    CloseLibrary(secBase);
    return res;
}

static LONG SecSetCred(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    APTR data;
    LONG res = PAM_SUCCESS;

    if (!(secBase = OpenSec()))
        return PAM_CRED_UNAVAIL;

    if (flags & PAM_DELETE_CRED)
    {
        if (PamGetData(h, DATA_LOGGEDIN, &data) == PAM_SUCCESS && data)
        {
            struct TagItem tags[] = { { secT_Task, (IPTR)h->Task }, { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
            secLogoutA(tags);
            PamSetData(h, DATA_LOGGEDIN, NULL, NULL);
        }
    }
    else
    {
        if (PamGetData(h, DATA_TICKET, &data) != PAM_SUCCESS || !data)
            res = PAM_CRED_UNAVAIL;
        else
        {
            struct TagItem tags[] =
            {
                { secT_Ticket, (IPTR)data   },
                { secT_Task,   (IPTR)h->Task },
                { TAG_DONE,    0            }
            };
            if (secLoginA(tags) == secOWNER_NOBODY)
                res = PAM_CRED_ERR;
            else
                PamSetData(h, DATA_LOGGEDIN, (APTR)1, NULL);
            PamSetData(h, DATA_TICKET, NULL, NULL);     /* one use */
        }
    }
    CloseLibrary(secBase);
    return res;
}

static LONG SecAcctMgmt(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user;
    LONG res;
    BOOL found;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(secBase = OpenSec()))
        return PAM_AUTHINFO_UNAVAIL;
    if (!secIsConfigured())
    {
        CloseLibrary(secBase);
        return PAM_AUTHINFO_UNAVAIL;
    }
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { TAG_DONE, 0 } };
        found = secQueryUserA(tags);
    }
    CloseLibrary(secBase);
    return found ? PAM_SUCCESS : PAM_USER_UNKNOWN;
}

static LONG SecChAuthTok(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user, oldtok, newtok;
    struct secUserInfo *info;
    LONG res;
    BOOL maychange = FALSE, ok;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(secBase = OpenSec()))
        return PAM_AUTHTOK_ERR;

    /* Only the caller's own account: secPasswd() works on the task's owner */
    res = PAM_PERM_DENIED;
    if ((info = secAllocUserInfo()))
    {
        info->uid = (UWORD)(secGetTaskOwner(NULL) >> 16);
        if (secGetUserInfo(info, secKeyType_uid) && !strcmp(info->UserID, user))
            res = PAM_SUCCESS;
        secFreeUserInfo(info);
    }
    if (res == PAM_SUCCESS)
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_MayChangePassword, (IPTR)&maychange }, { TAG_DONE, 0 } };
        if (!secQueryUserA(tags) || !maychange)
            res = PAM_PERM_DENIED;
    }
    if (res == PAM_SUCCESS)
    {
        if ((res = PamGetAuthTok(h, PAM_OLDAUTHTOK, &oldtok, "Old password: ")) == PAM_SUCCESS)
        {
            if (flags & PAM_PRELIM_CHECK)
            {
                struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_Password, (IPTR)oldtok }, { secT_Service, (IPTR)h->Service }, { TAG_DONE, 0 } };
                res = MapVerify(secVerifyUserA(tags));
                if (res != PAM_SUCCESS && res != PAM_MAXTRIES)
                    res = PAM_AUTHTOK_RECOVERY_ERR;
            }
            else if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &newtok, "New password: ")) == PAM_SUCCESS)
            {
                ok = secPasswd((STRPTR)oldtok, (STRPTR)newtok);
                res = ok ? PAM_SUCCESS : PAM_AUTHTOK_ERR;
            }
        }
    }
    CloseLibrary(secBase);
    return res;
}

const struct PamBuiltin PamBuiltinSecurity =
{
    "security", { SecAuthenticate, SecSetCred, SecAcctMgmt, NULL, NULL, SecChAuthTok }
};
