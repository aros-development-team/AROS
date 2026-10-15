/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Lockouts - list or clear failed-login records (security.library)
*/

/*****************************************************************************

    NAME
        Lockouts

    SYNOPSIS
        USER/K,HOST/K,CLEAR/S

    LOCATION
        C:

    FUNCTION
        Shows the failed-login records of security.library's authentication
        process, or clears them. After failed logins for a user name,
        further attempts for that name are held back for up to
        LOGINDELAYMAX seconds (Security.config); after MAXTRIES failures
        from a remote host, that host is locked out for the user for
        LOCKTIME seconds. Only root may use this command.

    INPUTS
        USER  -- only this user.
        HOST  -- only this remote host.
        CLEAR -- clear the records (of USER and/or HOST, else all of them)
                 instead of listing them.

    RESULT
        Standard DOS return codes.

    EXAMPLE
        Lockouts
        Lockouts USER=root CLEAR

    SEE ALSO
        Login, Who

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>
#include <utility/tagitem.h>

const TEXT version[] = "$VER: Lockouts 45.1 (15.10.2026)";

#define TEMPLATE "USER/K,HOST/K,CLEAR/S"

enum { ARG_USER, ARG_HOST, ARG_CLEAR, ARG_COUNT };

#define MAXRECORDS  (64)

struct Library *secBase;

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;

    if (!(secBase = OpenLibrary(SECURITYNAME, 45)) ||
        (secBase->lib_Version == 45 && secBase->lib_Revision < 14))
    {
        PutStr("Lockouts: needs security.library 45.14\n");
        if (secBase)
            CloseLibrary(secBase);
        return RETURN_FAIL;
    }

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "Lockouts");
        CloseLibrary(secBase);
        return RETURN_FAIL;
    }

    if (args[ARG_CLEAR])
    {
        struct TagItem tags[] =
        {
            { secT_AuthClear,                                   TRUE                 },
            { args[ARG_USER] ? secT_UserID : TAG_IGNORE,        args[ARG_USER]       },
            { args[ARG_HOST] ? secT_RemoteHost : TAG_IGNORE,    args[ARG_HOST]       },
            { TAG_DONE,                                         0                    }
        };

        if (secAuthControlA(tags))
        {
            PutStr("Lockouts: records cleared\n");
            rc = RETURN_OK;
        }
        else
            PutStr("Lockouts: only root may do this (or the authentication process is not running)\n");
    }
    else
    {
        struct secAuthRecord *recs;
        ULONG count = 0, i, shown = 0;

        if ((recs = AllocVec(MAXRECORDS * sizeof(struct secAuthRecord), MEMF_ANY | MEMF_CLEAR)))
        {
            struct TagItem tags[] =
            {
                { secT_AuthList,        (IPTR)recs      },
                { secT_AuthListSize,    MAXRECORDS      },
                { secT_AuthCount,       (IPTR)&count    },
                { TAG_DONE,             0               }
            };

            if (secAuthControlA(tags))
            {
                rc = RETURN_OK;
                for (i = 0; i < count && i < MAXRECORDS; i++)
                {
                    struct secAuthRecord *r = &recs[i];

                    if (args[ARG_USER] && Stricmp(r->UserID, (STRPTR)args[ARG_USER]))
                        continue;
                    if (args[ARG_HOST] && (r->Kind != secAUTHREC_LOCKOUT || Stricmp(r->Source, (STRPTR)args[ARG_HOST])))
                        continue;
                    if (!shown++)
                        PutStr("User                             Failures  Waiting  Host\n");
                    Printf("%-32s %8lu %7lus  %s\n", r->UserID, (ULONG)r->Failures, (ULONG)r->Seconds,
                           (r->Kind == secAUTHREC_LOCKOUT) ? r->Source : "(any)");
                }
                if (!shown)
                    PutStr("Lockouts: no failed logins recorded\n");
                else if (count > MAXRECORDS)
                    Printf("Lockouts: %lu more not shown\n", (unsigned long)(count - MAXRECORDS));
            }
            else
                PutStr("Lockouts: only root may do this (or the authentication process is not running)\n");
            FreeVec(recs);
        }
    }

    FreeArgs(rda);
    CloseLibrary(secBase);
    return rc;
}
