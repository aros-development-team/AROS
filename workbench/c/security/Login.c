/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Login - log a user into the system (security.library)
*/

/*****************************************************************************

    NAME
        Login

    SYNOPSIS
        USERID,PASSWORD/K,GLOBAL/S,PARENT/S,GRAPHICAL/S,QUIET/S,SYSTEM/S

    LOCATION
        C:

    FUNCTION
        Logs a user into the system. Without USERID the user is asked for a
        user id and password on the console (or in a requester with
        GRAPHICAL). The shell running Login becomes owned by the user; all
        commands started from it inherit that owner.

    INPUTS
        USERID    -- the user to log in as; asked for if omitted.
        PASSWORD  -- the password (only useful in scripts, requires USERID).
        GLOBAL    -- also apply the login to all tasks started from this
                     shell.
        PARENT    -- also apply the login to the parent process of this
                     shell. S:Security-Startup uses this so that the
                     Startup-Sequence, which the boot process runs afterwards,
                     inherits the login.
        GRAPHICAL -- use a requester instead of the console.
        QUIET     -- do not print who you are after logging in.

    RESULT
        Standard DOS return codes; RETURN_WARN if the login failed.

    NOTES
        On a system without a password file (SYS:Security/passwd) the login
        always succeeds as root.

    SEE ALSO
        Logout, Passwd, Who

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <exec/tasks.h>
#include <libraries/security.h>
#include <utility/tagitem.h>

#include <string.h>

const TEXT version[] = "$VER: Login 45.1 (28.08.2026)";

#define TEMPLATE "USERID,PASSWORD/K,GLOBAL/S,PARENT/S,GRAPHICAL/S,QUIET/S,SYSTEM/S"

enum { ARG_USERID, ARG_PASSWORD, ARG_GLOBAL, ARG_PARENT, ARG_GRAPHICAL, ARG_QUIET, ARG_SYSTEM, ARG_COUNT };

struct Library *secBase;

static void PrintWhoAmI(ULONG owner)
{
    struct secUserInfo *info;

    if (owner == secOWNER_NOBODY)
    {
        PutStr("Logged in as nobody\n");
        return;
    }
    if ((info = secAllocUserInfo()))
    {
        info->uid = owner >> 16;
        if (secGetUserInfo(info, secKeyType_uid))
            Printf("Logged in as %s (%s), uid %lu gid %lu\n", info->UserID, info->UserName, (ULONG)info->uid, (ULONG)info->gid);
        else
            Printf("Logged in with uid %lu gid %lu\n", owner >> 16, owner & secMASK_GID);
        secFreeUserInfo(info);
    }
}

/* Point HOME: at the user's home directory from the user database */
static void AssignHome(ULONG owner)
{
    struct secUserInfo *ui = secAllocUserInfo();

    if (ui)
    {
        ui->uid = owner >> 16;
        if (secGetUserInfo(ui, secKeyType_uid) && ui->HomeDir[0])
        {
            BPTR lock = Lock(ui->HomeDir, ACCESS_READ);

            if (lock && !AssignLock("HOME", lock))
                UnLock(lock);
        }
        secFreeUserInfo(ui);
    }
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;
    ULONG owner;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("Login: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        struct TagItem tags[10];
        int n = 0;

        if (args[ARG_USERID])
        {
            tags[n].ti_Tag = secT_UserID; tags[n++].ti_Data = args[ARG_USERID];
        }
        if (args[ARG_PASSWORD])
        {
            tags[n].ti_Tag = secT_Password; tags[n++].ti_Data = args[ARG_PASSWORD];
        }
        if (args[ARG_GLOBAL])
        {
            tags[n].ti_Tag = secT_Global; tags[n++].ti_Data = TRUE;
        }
        if (args[ARG_GRAPHICAL])
        {
            tags[n].ti_Tag = secT_Graphical; tags[n++].ti_Data = TRUE;
        }
        if (args[ARG_SYSTEM])
        {
            tags[n].ti_Tag = secT_System; tags[n++].ti_Data = TRUE;
        }
        if (args[ARG_QUIET])
        {
            tags[n].ti_Tag = secT_Quiet; tags[n++].ti_Data = TRUE;
        }
        if (args[ARG_QUIET])
        {
            tags[n].ti_Tag = secT_Quiet; tags[n++].ti_Data = TRUE;
        }
        tags[n].ti_Tag = TAG_DONE;

        owner = secLoginA(tags);
        if (owner != secOWNER_NOBODY)
        {
            rc = RETURN_OK;
            if (args[ARG_PARENT])
            {
                struct Task *me = FindTask(NULL);
                struct Task *parent = GetETask(me) ? (struct Task *)GetETask(me)->et_Parent : NULL;

                if (parent)
                {
                    struct TagItem ptags[] =
                    {
                        { secT_Task,    (IPTR)parent    },
                        { secT_Own,     TRUE            },
                        { secT_Global,  args[ARG_GLOBAL] ? TRUE : FALSE },
                        { TAG_DONE,     0               }
                    };
                    if (secLoginA(ptags) == secOWNER_NOBODY)
                    {
                        PutStr("Login: could not apply the login to the parent process\n");
                        rc = RETURN_WARN;
                    }
                }
            }
            AssignHome(owner);
            if (!args[ARG_QUIET])
                PrintWhoAmI(owner);
        }
        else
        {
            PutStr("Login failed\n");
            rc = RETURN_WARN;
        }
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "Login");

    CloseLibrary(secBase);
    return rc;
}
