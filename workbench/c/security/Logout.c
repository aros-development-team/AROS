/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Logout - restore the previous user (security.library)
*/

/*****************************************************************************

    NAME
        Logout

    SYNOPSIS
        ALL/S,QUIET/S,GLOBAL/S,GRAPHICAL/S

    LOCATION
        C:

    FUNCTION
        Logs out and restores the user that was logged in before. If there
        is no previous user a login prompt appears, unless QUIET is given.

    INPUTS
        ALL       -- log out all previous users.
        QUIET     -- never prompt for a new login, just log out.
        GLOBAL    -- also apply to all tasks started from this shell.
        GRAPHICAL -- use a requester for the login prompt.

    SEE ALSO
        Login, Who

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>
#include <utility/tagitem.h>

const TEXT version[] = "$VER: Logout 45.1 (28.08.2026)";

#define TEMPLATE "ALL/S,QUIET/S,GLOBAL/S,GRAPHICAL/S"

enum { ARG_ALL, ARG_QUIET, ARG_GLOBAL, ARG_GRAPHICAL, ARG_COUNT };

struct Library *secBase;

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("Logout: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        struct TagItem tags[] =
        {
            { secT_All,       args[ARG_ALL] ? TRUE : FALSE       },
            { secT_Quiet,     args[ARG_QUIET] ? TRUE : FALSE     },
            { secT_Global,    args[ARG_GLOBAL] ? TRUE : FALSE    },
            { secT_Graphical, args[ARG_GRAPHICAL] ? TRUE : FALSE },
            { TAG_DONE,       0                                  }
        };
        ULONG owner = secLogoutA(tags);

        if (owner == secOWNER_NOBODY)
            PutStr("You are now nobody\n");
        else
            Printf("You are now uid %lu gid %lu\n", owner >> 16, owner & secMASK_GID);
        rc = RETURN_OK;
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "Logout");

    CloseLibrary(secBase);
    return rc;
}
