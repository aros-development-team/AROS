/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Who - show the owner of the current shell or of all tasks
*/

/*****************************************************************************

    NAME
        Who

    SYNOPSIS
        ALL/S,USERS/S,GROUPS/S,RESCAN/S

    LOCATION
        C:

    FUNCTION
        Shows who you are. With ALL, lists every task with its owner. With
        USERS or GROUPS, lists the user or group database. RESCAN asks the
        security server to rescan the volumes for key files.

    SEE ALSO
        Login, Logout

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>
#include <proto/task.h>

#include <dos/dos.h>
#include <exec/tasks.h>
#include <libraries/security.h>
#include <resources/task.h>

#include <stdio.h>
#include <string.h>

const TEXT version[] = "$VER: Who 45.1 (28.08.2026)";

#define TEMPLATE "ALL/S,USERS/S,GROUPS/S,RESCAN/S"

enum { ARG_ALL, ARG_USERS, ARG_GROUPS, ARG_RESCAN, ARG_COUNT };

struct Library *secBase;
APTR TaskResBase;

static void DescribeOwner(ULONG owner, STRPTR buf, ULONG size)
{
    struct secUserInfo *info;

    if (owner == secOWNER_NOBODY)
    {
        strncpy(buf, "nobody", size);
        return;
    }
    if ((info = secAllocUserInfo()))
    {
        info->uid = owner >> 16;
        if (secGetUserInfo(info, secKeyType_uid))
            snprintf(buf, size, "%s (%lu:%lu)", info->UserID, owner >> 16, owner & secMASK_GID);
        else
            snprintf(buf, size, "uid %lu gid %lu", owner >> 16, owner & secMASK_GID);
        secFreeUserInfo(info);
    }
    else
        buf[0] = '\0';
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    char buf[300];
    int rc = RETURN_FAIL;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("Who: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        rc = RETURN_OK;
        if (args[ARG_RESCAN])
        {
            secFSRendezVous();
            PutStr("Volume rescan requested\n");
        }
        if (!secIsConfigured())
            PutStr("(the system is not configured for multi-user operation: everybody is privileged)\n");

        if (args[ARG_USERS])
        {
            struct secUserInfo *info = secAllocUserInfo();
            ULONG key = secKeyType_First;

            if (info)
            {
                PutStr("UserID           uid   gid   Name\n");
                while (secGetUserInfo(info, key))
                {
                    Printf("%-16s %-5lu %-5lu %s\n", info->UserID, (ULONG)info->uid, (ULONG)info->gid, info->UserName);
                    key = secKeyType_Next;
                }
                secFreeUserInfo(info);
            }
        }
        else if (args[ARG_GROUPS])
        {
            struct secGroupInfo *info = secAllocGroupInfo();
            ULONG key = secKeyType_First;

            if (info)
            {
                PutStr("GroupID          gid   mgr   Name\n");
                while (secGetGroupInfo(info, key))
                {
                    Printf("%-16s %-5lu %-5lu %s\n", info->GroupID, (ULONG)info->gid, (ULONG)info->MgrUid, info->GroupName);
                    key = secKeyType_Next;
                }
                secFreeGroupInfo(info);
            }
        }
        else if (args[ARG_ALL] && (TaskResBase = OpenResource("task.resource")))
        {
            struct TaskList *tl;
            struct Task *t;

            PutStr("Task               Name                             Owner\n");
            if ((tl = LockTaskList(LTF_ALL)))
            {
                while ((t = NextTaskEntry(tl, LTF_ALL)))
                {
                    DescribeOwner(secGetTaskOwner(t), buf, sizeof(buf));
                    Printf("%p %-32s %s\n", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : (char *)"", buf);
                }
                UnLockTaskList(tl, LTF_ALL);
            }
        }
        else
        {
            DescribeOwner(secGetTaskOwner(NULL), buf, sizeof(buf));
            Printf("You are %s\n", buf);
        }
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "Who");

    CloseLibrary(secBase);
    return rc;
}
