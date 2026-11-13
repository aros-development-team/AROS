/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library built-in module "unixpw": the AROSTCP password file
          (netinfo.device through usergroup.library), for machines without
          security.library. Passwords are DES crypt() hashes; the shipped
          file has "*" for every user, which never matches.

          auth:     getpwnam() + crypt() comparison.
          account:  the user must exist.
          setcred:  success: without security.library there are no
                    credentials to establish.
          session:  success, nothing to do.
          password: the old password is checked like for auth, the new
                    one is set with ug_UserAdminA() (usergroup.library 4.11),
                    which saves it through netinfo.device.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <libraries/usergroup.h>
#include <aros/libcall.h>
#include <proto/pam.h>
#include <string.h>

#include "pam_intern.h"

/* usergroup.library has a per-task base and its stub header expects the
 * link library's base accessor; call the two functions by LVO instead. */
static struct passwd *ug_getpwnam(struct Library *UserGroupBase, const char *name)
{
    return AROS_LC1(struct passwd *, getpwnam, AROS_LCA(const char *, name, A1), struct Library *, UserGroupBase, 19, Usergroup);
}

static char *ug_crypt(struct Library *UserGroupBase, const char *key, const char *setting)
{
    return AROS_LC2(char *, crypt, AROS_LCA(const char *, key, A0), AROS_LCA(const char *, setting, A1), struct Library *, UserGroupBase, 29, Usergroup);
}

static int ug_UserAdmin(struct Library *UserGroupBase, struct TagItem *tags)
{
    return AROS_LC1(int, ug_UserAdminA, AROS_LCA(struct TagItem *, tags, A0), struct Library *, UserGroupBase, 44, Usergroup);
}

/* Is tok the password of the entry? */
static BOOL CheckPw(struct Library *UserGroupBase, struct passwd *pw, CONST_STRPTR tok, BOOL nullok)
{
    CONST_STRPTR stored = pw->pw_passwd ? (CONST_STRPTR)pw->pw_passwd : (CONST_STRPTR)"";
    STRPTR c;

    if (!stored[0])
        return !tok[0] && nullok;
    if (stored[0] == '*' || stored[0] == '!')
        return FALSE;
    c = (STRPTR)ug_crypt(UserGroupBase, (const char *)tok, (const char *)stored);
    return c && !strcmp(c, stored);
}

static LONG UgAuthenticate(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *UserGroupBase;
    CONST_STRPTR user, tok;
    struct passwd *pw;
    LONG res;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &tok, NULL)) != PAM_SUCCESS)
        return res;
    if (h->AuthTokFormat != PAMTOK_CLEAR)
        return PAM_AUTHINFO_UNAVAIL;        /* cannot compare a pre-hashed token with crypt() */

    if (!(UserGroupBase = OpenLibrary(USERGROUPNAME, 4)))
        return PAM_AUTHINFO_UNAVAIL;

    res = PAM_AUTH_ERR;
    if ((pw = ug_getpwnam(UserGroupBase, (const char *)user)))
    {
        CONST_STRPTR stored = pw->pw_passwd ? (CONST_STRPTR)pw->pw_passwd : (CONST_STRPTR)"";

        if (!stored[0])
            res = (!tok[0] && !(flags & PAM_DISALLOW_NULL_AUTHTOK) && PamArgPresent(argc, argv, "nullok")) ? PAM_SUCCESS : PAM_AUTH_ERR;
        else if (stored[0] != '*' && stored[0] != '!')
        {
            STRPTR c = (STRPTR)ug_crypt(UserGroupBase, (const char *)tok, (const char *)stored);
            if (c && !strcmp(c, stored))
                res = PAM_SUCCESS;
        }
        if (res == PAM_SUCCESS)
            PamSetItem(h, PAM_USER, (APTR)pw->pw_name);
    }
    CloseLibrary(UserGroupBase);
    return res;
}

static LONG UgAcctMgmt(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *UserGroupBase;
    CONST_STRPTR user;
    LONG res;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(UserGroupBase = OpenLibrary(USERGROUPNAME, 4)))
        return PAM_AUTHINFO_UNAVAIL;
    res = ug_getpwnam(UserGroupBase, (const char *)user) ? PAM_SUCCESS : PAM_USER_UNKNOWN;
    CloseLibrary(UserGroupBase);
    return res;
}

static LONG UgNothing(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    return PAM_SUCCESS;
}

static LONG UgChAuthTok(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *UserGroupBase;
    CONST_STRPTR user, oldtok, newtok;
    struct passwd *pw;
    LONG res;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(UserGroupBase = OpenLibrary(USERGROUPNAME, 4)))
        return PAM_AUTHTOK_ERR;
    if (UserGroupBase->lib_Version == 4 && UserGroupBase->lib_Revision < 11)
    {
        CloseLibrary(UserGroupBase);
        return PAM_AUTHTOK_ERR;
    }

    res = PAM_USER_UNKNOWN;
    if ((pw = ug_getpwnam(UserGroupBase, (const char *)user)))
    {
        if ((res = PamGetAuthTok(h, PAM_OLDAUTHTOK, &oldtok, "Old password: ")) == PAM_SUCCESS)
        {
            if (flags & PAM_PRELIM_CHECK)
                res = CheckPw(UserGroupBase, pw, oldtok, TRUE) ? PAM_SUCCESS : PAM_AUTHTOK_RECOVERY_ERR;
            else if (!CheckPw(UserGroupBase, pw, oldtok, TRUE))
                res = PAM_AUTHTOK_RECOVERY_ERR;
            else if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &newtok, "New password: ")) == PAM_SUCCESS)
            {
                struct TagItem tags[] =
                {
                    { UGA_Operation,    UGOP_PASSWORD   },
                    { UGA_Name,         (IPTR)user      },
                    { UGA_Password,     (IPTR)newtok    },
                    { TAG_DONE,         0               }
                };
                res = (ug_UserAdmin(UserGroupBase, tags) == 0) ? PAM_SUCCESS : PAM_AUTHTOK_ERR;
            }
        }
    }
    CloseLibrary(UserGroupBase);
    return res;
}

const struct PamBuiltin PamBuiltinUnixPw =
{
    "unixpw", { UgAuthenticate, UgNothing, UgAcctMgmt, UgNothing, UgNothing, UgChAuthTok }
};
