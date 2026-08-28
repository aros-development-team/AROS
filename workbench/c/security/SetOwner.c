/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SetOwner - change the owner of a file or directory
*/

/*****************************************************************************

    NAME
        SetOwner

    SYNOPSIS
        FILE/A,USER,GROUP,UID/K/N,GID/K/N

    LOCATION
        C:

    FUNCTION
        Changes the owner (and group) of a file or directory. The user and
        group may be given by name or by number. Only root or the current
        owner may change the owner; only root may give an object to
        somebody else.

    INPUTS
        FILE  -- the file or directory.
        USER  -- the new owner's user id (name).
        GROUP -- the new group id (name); defaults to the user's primary group.
        UID   -- the new owner's uid (number).
        GID   -- the new gid (number).

    SEE ALSO
        Protect, Who

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>

#include <string.h>

const TEXT version[] = "$VER: SetOwner 45.1 (28.08.2026)";

#define TEMPLATE "FILE/A,USER,GROUP,UID/K/N,GID/K/N"

enum { ARG_FILE, ARG_USER, ARG_GROUP, ARG_UID, ARG_GID, ARG_COUNT };

struct Library *secBase;

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;
    LONG uid = -1, gid = -1;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("SetOwner: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        if (args[ARG_UID])
            uid = *(LONG *)args[ARG_UID];
        if (args[ARG_GID])
            gid = *(LONG *)args[ARG_GID];

        if (args[ARG_USER])
        {
            struct secUserInfo *info = secAllocUserInfo();

            if (info)
            {
                strncpy(info->UserID, (STRPTR)args[ARG_USER], secUSERIDSIZE - 1);
                if (secGetUserInfo(info, secKeyType_UserID))
                {
                    uid = info->uid;
                    if (gid == -1 && !args[ARG_GROUP])
                        gid = info->gid;
                }
                else
                    Printf("SetOwner: unknown user '%s'\n", (STRPTR)args[ARG_USER]);
                secFreeUserInfo(info);
            }
        }
        if (args[ARG_GROUP])
        {
            struct secGroupInfo *info = secAllocGroupInfo();

            if (info)
            {
                strncpy(info->GroupID, (STRPTR)args[ARG_GROUP], secGROUPIDSIZE - 1);
                if (secGetGroupInfo(info, secKeyType_GroupID))
                    gid = info->gid;
                else
                    Printf("SetOwner: unknown group '%s'\n", (STRPTR)args[ARG_GROUP]);
                secFreeGroupInfo(info);
            }
        }

        if (uid < 0 || uid > 65535 || gid < 0 || gid > 65535)
            PutStr("SetOwner: a valid user and group are required\n");
        else if (SetOwner((STRPTR)args[ARG_FILE], ((ULONG)uid << 16) | (ULONG)gid))
        {
            Printf("%s: owner set to %ld:%ld\n", (STRPTR)args[ARG_FILE], uid, gid);
            rc = RETURN_OK;
        }
        else
        {
            PrintFault(IoErr(), (STRPTR)args[ARG_FILE]);
            rc = RETURN_ERROR;
        }
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "SetOwner");

    CloseLibrary(secBase);
    return rc;
}
