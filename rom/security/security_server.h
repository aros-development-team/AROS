/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library server process
*/
#ifndef _SECURITY_SERVER_H
#define _SECURITY_SERVER_H

#include <exec/ports.h>

#define SERVERNAME              "Security.server"

struct SecurityBase;

/*
 * Private Server Packet
 */
struct secSPacket
{
    struct Message      Msg;
    LONG                Type;                   /* See definitions below */
    SIPTR               Arg1;
    SIPTR               Arg2;
    SIPTR               Arg3;
    SIPTR               Arg4;
    SIPTR               Res1;
    ULONG               Stage;                  /* index into the plugin stack for this operation */
    ULONG               AsyncResult;            /* Result of an asynchronous plugin operation     */
};

#define secSAction_Quit                 0       /* Server Quit                                  */
#define secSAction_CheckUser            1       /* Login User                                   */
#define secSAction_Passwd               2       /* Change User Password                         */
#define secSAction_GetUserInfo          3       /* Get User Information                         */
#define secSAction_CheckPasswd          4       /* Check Password                               */
#define secSAction_PasswdDirLock        5       /* Get PasswdDirLock                            */
#define secSAction_ConfigDirLock        6       /* Get ConfigDirLock                            */
#define secSAction_GetGroupInfo         7       /* Get Group Information                        */
#define secSAction_InitModule           8       /* Call the init function for a module          */
#define secSAction_FiniModule           9       /* Call the fini function for a module          */
#define secSAction_LoadPlugin           10      /* Load a plugin by name                        */
#define secSAction_UnloadPlugin         11      /* Unload a plugin by name                      */
#define secSAction_Log                  12      /* Append a line to the log file                */
#define secSAction_GetUserHash          13      /* Security.auth only: user record + stored hash*/
#define secSAction_LockUser             14      /* root: lock (Arg2 TRUE) / unlock an account   */
#define secSAction_UserAdmin            15      /* root: secUserAdminA() (Arg1 struct secUARequest *) */

/* secUserAdminA() request (security_useradmin.c) */
struct secUARequest
{
    ULONG           Operation;      /* secUA_#?                                  */
    CONST_STRPTR    UserID;         /* the user (ADD: the new one)               */
    CONST_STRPTR    NewUserID;      /* MODIFY: rename, or NULL                   */
    LONG            uid, gid;       /* -1 = unchanged / not given                */
    CONST_STRPTR    UserName;       /* NULL = unchanged                          */
    CONST_STRPTR    HomeDir;
    CONST_STRPTR    Shell;
    CONST_STRPTR    Password;       /* ADD, PASSWORD: clear text, "" = none      */
    CONST_STRPTR    GroupID;        /* GROUP#?: the group (GROUPADD: the new one)*/
    CONST_STRPTR    NewGroupID;     /* GROUPMODIFY: rename, or NULL              */
    CONST_STRPTR    GroupName;      /* NULL = unchanged                          */
    LONG            MgrUid;         /* -1 = unchanged / root for a new group     */
    BOOL            HasMembers;     /* Members given (NULL = none)               */
    CONST_STRPTR   *Members;        /* NULL-terminated user IDs                  */
};

extern ULONG UserAdmin(struct SecurityBase *secBase, struct Task *client, struct secUARequest *req);

extern struct Process *CreateServer(struct SecurityBase *secBase);
extern BOOL StartServer(struct SecurityBase *secBase);
extern SIPTR SendServerPacket(struct SecurityBase *secBase, SIPTR type, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4);

#endif /* _SECURITY_SERVER_H */
