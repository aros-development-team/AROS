/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Passwd - change your password (security.library)
*/

/*****************************************************************************

    NAME
        Passwd

    SYNOPSIS
        OLD/K,NEW/K,LOCK/K,UNLOCK/K

    LOCATION
        C:

    FUNCTION
        Changes the password of the user logged into the current task.
        Missing passwords are requested without echoing their characters.
        An interactively entered new password must be confirmed.

        LOCK and UNLOCK (root only) lock a user's account - no password
        is accepted for it any more, the stored one is kept - or unlock it.

    INPUTS
        OLD    -- the current password.
        NEW    -- the new password.
        LOCK   -- user ID of the account to lock.
        UNLOCK -- user ID of the account to unlock.

    RESULT
        RETURN_OK if a password change, lock, or unlock succeeds.
        RETURN_WARN if no user is logged in or the requested operation fails.
        RETURN_FAIL if the security library is unavailable or arguments
        cannot be parsed.

    NOTES
        Password changes use the configured PAM password service when
        available, or security.library otherwise.

    SEE ALSO
        Login

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>
#include <libraries/pam.h>
#include <proto/pam.h>

#include <string.h>

const TEXT version[] = "$VER: Passwd 45.3 (15.10.2026)";

#define TEMPLATE "OLD/K,NEW/K,LOCK/K,UNLOCK/K"

enum { ARG_OLD, ARG_NEW, ARG_LOCK, ARG_UNLOCK, ARG_COUNT };

struct Library *secBase;

/* Read a password without echo */
static BOOL ReadPassword(CONST_STRPTR prompt, STRPTR buf, ULONG size)
{
    BPTR in = Input(), out = Output();
    ULONG len = 0;
    BOOL done = FALSE, ok = TRUE;
    LONG c;

    FPuts(out, prompt);
    Flush(out);
    SetMode(in, 1);
    do
    {
        c = FGetC(in);
        switch (c)
        {
        case -1:
        case 3:
            ok = FALSE;
            done = TRUE;
            break;
        case '\b':
        case 127:
            if (len)
                len--;
            break;
        case '\r':
        case '\n':
            done = TRUE;
            break;
        default:
            if ((len < size - 1) && (c & 0x7f) > 31)
                buf[len++] = c;
            break;
        }
    } while (!done);
    buf[len] = '\0';
    SetMode(in, 0);
    FPuts(out, "\n");
    Flush(out);
    return ok;
}

struct Library *PamBase;

/*
 * Change the password through pam.library's "passwd" service when the
 * library is available (the stack decides what a password change means on
 * this system), otherwise directly through security.library.
 */
static BOOL ChangePassword(STRPTR oldpwd, STRPTR newpwd)
{
    struct PamHandle *h;
    struct secUserInfo *info;
    LONG r = PAM_SYSTEM_ERR;
    BOOL done = FALSE;

    if ((PamBase = OpenLibrary(PAMNAME, PAMVERSION)))
    {
        if ((info = secAllocUserInfo()))
        {
            info->uid = (UWORD)(secGetTaskOwner(NULL) >> 16);
            if (secGetUserInfo(info, secKeyType_uid) &&
                (h = PamStartA("passwd", info->UserID, NULL, NULL)))
            {
                PamSetItem(h, PAM_OLDAUTHTOK, oldpwd);
                PamSetItem(h, PAM_AUTHTOK, newpwd);
                r = PamChAuthTok(h, 0);
                if (r != PAM_SUCCESS)
                    Printf("%s\n", PamStrError(h, r));
                PamEnd(h, r);
                done = TRUE;
            }
            secFreeUserInfo(info);
        }
        CloseLibrary(PamBase);
        if (done)
            return r == PAM_SUCCESS;
    }
    return secPasswd(oldpwd, newpwd);
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;
    char oldpwd[secPASSWORDSIZE], newpwd[secPASSWORDSIZE], retype[secPASSWORDSIZE];
    STRPTR o, n;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("Passwd: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        rc = RETURN_WARN;
        if (args[ARG_LOCK] || args[ARG_UNLOCK])
        {
            STRPTR user = (STRPTR)(args[ARG_LOCK] ? args[ARG_LOCK] : args[ARG_UNLOCK]);
            struct TagItem tags[] =
            {
                { args[ARG_LOCK] ? secT_LockAccount : secT_UnlockAccount, (IPTR)user },
                { TAG_DONE, 0 }
            };

            if (secBase->lib_Version == 45 && secBase->lib_Revision < 14)
                PutStr("Passwd: LOCK/UNLOCK needs security.library 45.14\n");
            else if (secAuthControlA(tags))
            {
                Printf("Account %s %s\n", user, args[ARG_LOCK] ? "locked" : "unlocked");
                rc = RETURN_OK;
            }
            else
                Printf("Passwd: cannot %s %s (root only; does the user exist?)\n", args[ARG_LOCK] ? "lock" : "unlock", user);
        }
        else if (secGetTaskOwner(NULL) == secOWNER_NOBODY)
            PutStr("You are not logged in\n");
        else
        {
            o = (STRPTR)args[ARG_OLD];
            n = (STRPTR)args[ARG_NEW];
            if (!o)
            {
                if (!ReadPassword("Old password        : ", oldpwd, sizeof(oldpwd)))
                    goto done;
                o = oldpwd;
            }
            if (!n)
            {
                if (!ReadPassword("New password        : ", newpwd, sizeof(newpwd)))
                    goto done;
                if (!ReadPassword("Retype new password : ", retype, sizeof(retype)))
                    goto done;
                if (strcmp(newpwd, retype))
                {
                    PutStr("Error retyping new password\n");
                    goto done;
                }
                n = newpwd;
            }
            if (ChangePassword(o, n))
            {
                PutStr("Password successfully changed\n");
                rc = RETURN_OK;
            }
            else
                PutStr("Changing password failed\n");
        }
done:
        memset(oldpwd, 0, sizeof(oldpwd));
        memset(newpwd, 0, sizeof(newpwd));
        memset(retype, 0, sizeof(retype));
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "Passwd");

    CloseLibrary(secBase);
    return rc;
}
