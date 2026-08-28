/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library server process. Owns the user/group database
          and the configuration; clients talk to it with secSPackets.
          Derived from MultiUser Server.c (c) Geert Uytterhoeven.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_groupinfo.h"
#include "security_userinfo.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_crypto.h"
#include "security_plugins.h"
#include "security_support.h"

#define SERVERPRI               (4)
#define SERVERSTACK             (AROS_STACKSIZE * 2)

/*
 * Send a packet to the server and wait for the reply. Returns 0 if the
 * server is not running.
 */
SIPTR SendServerPacket(struct SecurityBase *secBase, SIPTR type, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4)
{
    struct secSPacket pkt;
    struct MsgPort *port;
    SIPTR res = 0;

    D(bug(DEBUG_NAME_STR " %s(%ld)\n", __func__, (long)type);)

    if (!secBase->ServerPort)
        return 0;

    /* The server must not wait for itself */
    if (FindTask(NULL) == (struct Task *)secBase->Server)
    {
        D(bug(DEBUG_NAME_STR " %s: called from the server!\n", __func__);)
        return 0;
    }

    if ((port = CreateMsgPort()))
    {
        memset(&pkt, 0, sizeof(pkt));
        pkt.Msg.mn_ReplyPort = port;
        pkt.Msg.mn_Length = sizeof(struct secSPacket);
        pkt.Type = type;
        pkt.Arg1 = arg1;
        pkt.Arg2 = arg2;
        pkt.Arg3 = arg3;
        pkt.Arg4 = arg4;

        Forbid();
        if (secBase->ServerPort)
        {
            PutMsg(secBase->ServerPort, (struct Message *)&pkt);
            Permit();
            do
            {
                WaitPort(port);
            } while (GetMsg(port) != (struct Message *)&pkt);
            res = pkt.Res1;
        }
        else
            Permit();
        DeleteMsgPort(port);
    }
    return res;
}

/*
 * Fill in the User Information
 */
static void FillUserInfo(struct secUserDef *def, struct secPrivUserInfo *info)
{
    strncpy(info->Pub.UserID, def->UserID, secUSERIDSIZE - 1);
    info->Pub.UserID[secUSERIDSIZE - 1] = '\0';
    info->Pub.uid = def->uid;
    info->Pub.gid = def->gid;
    strncpy(info->Pub.UserName, def->UserName, secUSERNAMESIZE - 1);
    info->Pub.UserName[secUSERNAMESIZE - 1] = '\0';
    strncpy(info->Pub.HomeDir, def->HomeDir, secHOMEDIRSIZE - 1);
    info->Pub.HomeDir[secHOMEDIRSIZE - 1] = '\0';
    if (info->Pub.NumSecGroups)
        Free(info->Pub.SecGroups, info->Pub.NumSecGroups * sizeof(UWORD));
    if (def->NumSecGroups && (info->Pub.SecGroups = MAlloc(def->NumSecGroups * sizeof(UWORD))))
    {
        info->Pub.NumSecGroups = def->NumSecGroups;
        CopyMem(def->SecGroups, info->Pub.SecGroups, def->NumSecGroups * sizeof(UWORD));
    }
    else
    {
        info->Pub.NumSecGroups = 0;
        info->Pub.SecGroups = NULL;
    }
    strncpy(info->Pub.Shell, def->Shell, secSHELLSIZE - 1);
    info->Pub.Shell[secSHELLSIZE - 1] = '\0';
    info->Password = (def->Password[0] != '\0');
}

/*
 * Fill in the Group Information
 */
static void FillGroupInfo(struct secGroupDef *def, struct secPrivGroupInfo *info)
{
    strncpy(info->Pub.GroupID, def->GroupID, secGROUPIDSIZE - 1);
    info->Pub.GroupID[secGROUPIDSIZE - 1] = '\0';
    info->Pub.gid = def->gid;
    info->Pub.MgrUid = def->MgrUid;
    strncpy(info->Pub.GroupName, def->GroupName, secGROUPNAMESIZE - 1);
    info->Pub.GroupName[secGROUPNAMESIZE - 1] = '\0';
}

/*
 * Check if a user is authorised to login. Returns a private user info
 * (allocated with secAllocUserInfo(), to be freed by the caller) or NULL.
 */
static struct secPrivUserInfo *CheckUser(struct SecurityBase *secBase, ULONG user, STRPTR userid, STRPTR pwd, BOOL nopasswd, BOOL nolog)
{
    struct secPrivUserInfo *info = NULL;
    struct secUserDef *def;
    UWORD uid = user >> 16;

    for (def = GetUserDefs(secBase); def; def = def->Next)
    {
        if (!strcmp(userid, def->UserID))
        {
            if (nopasswd || verifypass(secBase, def->UserID, def->Password, pwd ? pwd : (STRPTR)""))
            {
                if ((info = (struct secPrivUserInfo *)secAllocUserInfo()))
                    FillUserInfo(def, info);
            }
            break;
        }
    }

    if (info)
        CallMonitors(secBase, secTrgB_Login, uid, info->Pub.uid, userid);
    else
        CallMonitors(secBase, secTrgB_LoginFail, uid, 0, userid);

    if (!nolog && ((info && (secBase->Config.LogFlags & secLogF_Login)) ||
                   (!info && (secBase->Config.LogFlags & secLogF_LoginFail))))
    {
        SIPTR args[2];
        args[0] = uid;
        args[1] = (SIPTR)userid;
        VLogF(secBase, GetLogStr(secBase, info ? MSG_LOG_LOGIN : MSG_LOG_LOGINFAIL), args);
    }

    return info;
}

/*
 * Check the Password of a User
 */
static BOOL CheckPasswd(struct SecurityBase *secBase, ULONG user, STRPTR pwd)
{
    BOOL valid = FALSE;
    UWORD uid = user >> 16;
    struct secUserDef *def;

    for (def = GetUserDefs(secBase); def; def = def->Next)
    {
        if (def->uid == uid)
        {
            valid = verifypass(secBase, def->UserID, def->Password, pwd ? pwd : (STRPTR)"");
            break;
        }
    }

    CallMonitors(secBase, valid ? secTrgB_CheckPasswd : secTrgB_CheckPasswdFail, uid, 0, NULL);

    if ((valid && (secBase->Config.LogFlags & secLogF_CheckPasswd)) ||
        (!valid && (secBase->Config.LogFlags & secLogF_CheckPasswdFail)))
    {
        SIPTR args[1];
        args[0] = uid;
        VLogF(secBase, GetLogStr(secBase, valid ? MSG_LOG_CHECKPASSWD : MSG_LOG_CHECKPASSWDFAIL), args);
    }

    return valid;
}

/*
 * Change the Password of a user
 */
static BOOL Passwd(struct SecurityBase *secBase, ULONG user, STRPTR oldpwd, STRPTR newpwd)
{
    BOOL changed = FALSE;
    UWORD uid = user >> 16;
    UWORD gid = user & secMASK_GID;
    struct secUserDef *def;

    if ((uid <= secBase->Config.PasswduidLevel) || (gid <= secBase->Config.PasswdgidLevel))
    {
        for (def = GetUserDefs(secBase); def; def = def->Next)
        {
            if (def->uid == uid)
            {
                if (verifypass(secBase, def->UserID, def->Password, oldpwd ? oldpwd : (STRPTR)"") &&
                    EncryptPassword(secBase, def->Password, def->UserID, newpwd ? newpwd : (STRPTR)""))
                    changed = UpdateUserDefs(secBase);
                break;
            }
        }
    }

    CallMonitors(secBase, changed ? secTrgB_Passwd : secTrgB_PasswdFail, uid, 0, NULL);

    if ((changed && (secBase->Config.LogFlags & secLogF_Passwd)) ||
        (!changed && (secBase->Config.LogFlags & secLogF_PasswdFail)))
    {
        SIPTR args[1];
        args[0] = uid;
        VLogF(secBase, GetLogStr(secBase, changed ? MSG_LOG_PASSWD : MSG_LOG_PASSWDFAIL), args);
    }

    return changed;
}

/* Does a user belong to a group? */
static BOOL Belongs2(struct secUserDef *def, UWORD gid)
{
    int i;

    if (def->gid == gid)
        return TRUE;
    for (i = 0; i < def->NumSecGroups; i++)
        if (def->SecGroups[i] == gid)
            return TRUE;
    return FALSE;
}

/* Prepare a case-insensitive pattern in info->Pattern */
static BOOL PreparePattern(struct SecurityBase *secBase, STRPTR **pattern, CONST_STRPTR src)
{
    ULONG len = 2 * strlen(src) + 2;

    FreeV(*pattern);
    if ((*pattern = MAllocV(len)) && (ParsePatternNoCase(src, (STRPTR)*pattern, len) != -1))
        return TRUE;
    FreeV(*pattern);
    *pattern = NULL;
    return FALSE;
}

/*
 * Get Information about a User
 */
static struct secPrivUserInfo *GetUserInfo(struct SecurityBase *secBase, struct secPrivUserInfo *info, ULONG keytype)
{
    struct secUserDef *def;
    ULONG count = 0;

    if (!info || !(def = GetUserDefs(secBase)))
        return NULL;

    switch (keytype)
    {
    case secKeyType_First:
        break;

    case secKeyType_Next:
        while ((count <= info->Count) && (def = def->Next))
            count++;
        break;

    case secKeyType_UserID:
        while (strcmp(def->UserID, info->Pub.UserID) && (def = def->Next))
            count++;
        break;

    case secKeyType_uid:
        while ((def->uid != info->Pub.uid) && (def = def->Next))
            count++;
        break;

    case secKeyType_gid:
        info->Tgid = info->Pub.gid;
        while (!Belongs2(def, info->Tgid) && (def = def->Next))
            count++;
        break;

    case secKeyType_gidNext:
        while ((count <= info->Count) && (def = def->Next))
            count++;
        if (def)
            while (!Belongs2(def, info->Tgid) && (def = def->Next))
                count++;
        break;

    case secKeyType_UserName:
        while (Stricmp(def->UserName, info->Pub.UserName) && (def = def->Next))
            count++;
        break;

    case secKeyType_WUserID:
        if (PreparePattern(secBase, (STRPTR **)&info->Pattern, info->Pub.UserID))
            while (!MatchPatternNoCase(info->Pattern, def->UserID) && (def = def->Next))
                count++;
        else
            def = NULL;
        break;

    case secKeyType_WUserIDNext:
        if (info->Pattern)
        {
            while ((count <= info->Count) && (def = def->Next))
                count++;
            if (def)
                while (!MatchPatternNoCase(info->Pattern, def->UserID) && (def = def->Next))
                    count++;
        }
        else
            def = NULL;
        break;

    case secKeyType_WUserName:
        if (PreparePattern(secBase, (STRPTR **)&info->Pattern, info->Pub.UserName))
            while (!MatchPatternNoCase(info->Pattern, def->UserName) && (def = def->Next))
                count++;
        else
            def = NULL;
        break;

    case secKeyType_WUserNameNext:
        if (info->Pattern)
        {
            while ((count <= info->Count) && (def = def->Next))
                count++;
            if (def)
                while (!MatchPatternNoCase(info->Pattern, def->UserName) && (def = def->Next))
                    count++;
        }
        else
            def = NULL;
        break;

    default:
        def = NULL;
        break;
    }

    if (def)
    {
        FillUserInfo(def, info);
        info->Count = count;
        return info;
    }
    return NULL;
}

/*
 * Get Information about a Group
 */
static struct secPrivGroupInfo *GetGroupInfo(struct SecurityBase *secBase, struct secPrivGroupInfo *info, ULONG keytype)
{
    struct secGroupDef *def;
    ULONG count = 0;

    if (!info || !(def = GetGroupDefs(secBase)))
        return NULL;

    switch (keytype)
    {
    case secKeyType_First:
        break;

    case secKeyType_Next:
        while ((count <= info->Count) && (def = def->Next))
            count++;
        break;

    case secKeyType_GroupID:
        while (strcmp(def->GroupID, info->Pub.GroupID) && (def = def->Next))
            count++;
        break;

    case secKeyType_gid:
        while ((def->gid != info->Pub.gid) && (def = def->Next))
            count++;
        break;

    case secKeyType_GroupName:
        while (Stricmp(def->GroupName, info->Pub.GroupName) && (def = def->Next))
            count++;
        break;

    case secKeyType_WGroupID:
        if (PreparePattern(secBase, (STRPTR **)&info->Pattern, info->Pub.GroupID))
            while (!MatchPatternNoCase(info->Pattern, def->GroupID) && (def = def->Next))
                count++;
        else
            def = NULL;
        break;

    case secKeyType_WGroupIDNext:
        if (info->Pattern)
        {
            while ((count <= info->Count) && (def = def->Next))
                count++;
            if (def)
                while (!MatchPatternNoCase(info->Pattern, def->GroupID) && (def = def->Next))
                    count++;
        }
        else
            def = NULL;
        break;

    case secKeyType_WGroupName:
        if (PreparePattern(secBase, (STRPTR **)&info->Pattern, info->Pub.GroupName))
            while (!MatchPatternNoCase(info->Pattern, def->GroupName) && (def = def->Next))
                count++;
        else
            def = NULL;
        break;

    case secKeyType_WGroupNameNext:
        if (info->Pattern)
        {
            while ((count <= info->Count) && (def = def->Next))
                count++;
            if (def)
                while (!MatchPatternNoCase(info->Pattern, def->GroupName) && (def = def->Next))
                    count++;
        }
        else
            def = NULL;
        break;

    case secKeyType_MgrUid:
        while ((def->MgrUid != info->Pub.MgrUid) && (def = def->Next))
            count++;
        break;

    case secKeyType_MgrUidNext:
        while ((count <= info->Count) && (def = def->Next))
            count++;
        if (def)
            while ((def->MgrUid != info->Pub.MgrUid) && (def = def->Next))
                count++;
        break;

    default:
        def = NULL;
        break;
    }

    if (def)
    {
        FillGroupInfo(def, info);
        info->Count = count;
        return info;
    }
    return NULL;
}

/*
 * The Server's Process
 */
static void ServerProcess(void)
{
    struct SecurityBase *secBase;
    struct Process *serverProc = (struct Process *)FindTask(NULL);
    struct secSPacket *pkt;
    BOOL quit = FALSE;
    ULONG user;
    ULONG signals;
    struct DosPacket *spkt;
    struct MsgPort *serverport = NULL, *monitorport = NULL;
    BYTE notifysig = -1, consistencysig = -1;

    secBase = (struct SecurityBase *)serverProc->pr_Task.tc_UserData;

    D(bug(DEBUG_NAME_STR " %s: " SERVERNAME " starting, secBase @ %p\n", __func__, secBase);)

    spkt = WaitPkt();

    serverProc->pr_WindowPtr = (APTR)-1;

    if (((notifysig = AllocSignal(-1)) == -1) ||
        ((consistencysig = AllocSignal(-1)) == -1) ||
        !(serverport = CreateMsgPort()) ||
        !(monitorport = CreateMsgPort()))
    {
        D(bug(DEBUG_NAME_STR " %s: Init failed!\n", __func__);)
        if (monitorport) DeleteMsgPort(monitorport);
        if (serverport) DeleteMsgPort(serverport);
        if (consistencysig != -1) FreeSignal(consistencysig);
        if (notifysig != -1) FreeSignal(notifysig);
        ReplyPkt(spkt, DOSFALSE, 0);
        return;
    }
    secBase->NotifySig = notifysig;
    secBase->ConsistencySig = consistencysig;
    secBase->MonitorPort = monitorport;

    D(bug(DEBUG_NAME_STR " %s: Initialising Volumes ..\n", __func__);)
    InitVolumes(secBase);

    /* From now on clients may talk to us */
    secBase->ServerPort = serverport;
    ReplyPkt(spkt, DOSTRUE, 0);

    D(bug(DEBUG_NAME_STR " %s: Going Live ..\n", __func__);)

    do
    {
        signals = Wait((1UL << notifysig) | (1UL << consistencysig) |
                       (1UL << serverport->mp_SigBit) | (1UL << monitorport->mp_SigBit));

        if (signals & (1UL << notifysig))
        {
            D(bug(DEBUG_NAME_STR " %s: database changed, reloading\n", __func__);)
            FreeDefs(secBase);
            GetUserDefs(secBase);
        }

        if (signals & (1UL << consistencysig))
        {
            FreeVolumes(secBase);
            InitVolumes(secBase);
        }

        if (signals & (1UL << serverport->mp_SigBit))
        {
            while (!quit && (pkt = (struct secSPacket *)GetMsg(serverport)))
            {
                struct Task *client = pkt->Msg.mn_ReplyPort ? pkt->Msg.mn_ReplyPort->mp_SigTask : NULL;

                switch (pkt->Type)
                {
                case secSAction_Quit:
                    D(bug(DEBUG_NAME_STR " %s: secSAction_Quit\n", __func__);)
                    quit = TRUE;
                    pkt->Res1 = TRUE;
                    break;

                case secSAction_CheckUser:
                    /* Arg1: STRPTR userid, Arg2: STRPTR pwd, Arg3: BOOL nopasswd, Arg4: BOOL nolog
                     * Res1: struct secPrivUserInfo * (NULL on failure) */
                    user = client ? GetTaskOwner(secBase, client) : secOWNER_NOBODY;
                    pkt->Res1 = (SIPTR)CheckUser(secBase, user, (STRPTR)pkt->Arg1, (STRPTR)pkt->Arg2,
                                                 (BOOL)pkt->Arg3, (BOOL)pkt->Arg4);
                    break;

                case secSAction_Passwd:
                    /* Arg1: STRPTR oldpwd, Arg2: STRPTR newpwd. Res1: BOOL */
                    user = client ? GetTaskOwner(secBase, client) : secOWNER_NOBODY;
                    pkt->Res1 = (SIPTR)Passwd(secBase, user, (STRPTR)pkt->Arg1, (STRPTR)pkt->Arg2);
                    break;

                case secSAction_GetUserInfo:
                    /* Arg1: struct secPrivUserInfo *, Arg2: keytype. Res1: info or NULL */
                    pkt->Res1 = (SIPTR)GetUserInfo(secBase, (struct secPrivUserInfo *)pkt->Arg1, (ULONG)pkt->Arg2);
                    break;

                case secSAction_CheckPasswd:
                    /* Arg1: STRPTR pwd. Res1: BOOL */
                    user = client ? GetTaskOwner(secBase, client) : secOWNER_NOBODY;
                    pkt->Res1 = (SIPTR)CheckPasswd(secBase, user, (STRPTR)pkt->Arg1);
                    break;

                case secSAction_PasswdDirLock:
                    pkt->Res1 = secBase->_pwdLock ? (SIPTR)DupLock(secBase->_pwdLock) : 0;
                    break;

                case secSAction_ConfigDirLock:
                    pkt->Res1 = secBase->_cfgLock ? (SIPTR)DupLock(secBase->_cfgLock) : 0;
                    break;

                case secSAction_GetGroupInfo:
                    pkt->Res1 = (SIPTR)GetGroupInfo(secBase, (struct secPrivGroupInfo *)pkt->Arg1, (ULONG)pkt->Arg2);
                    break;

                case secSAction_InitModule:
                {
                    secPluginModule *mod = (secPluginModule *)pkt->Arg1;
                    pkt->Res1 = mod->header->Initialize((struct Library *)secBase, mod);
                    break;
                }

                case secSAction_FiniModule:
                {
                    secPluginModule *mod = (secPluginModule *)pkt->Arg1;
                    mod->header->Terminate();
                    pkt->Res1 = TRUE;
                    break;
                }

                case secSAction_LoadPlugin:
                    pkt->Res1 = loadPlugin(secBase, (STRPTR)pkt->Arg1);
                    break;

                case secSAction_UnloadPlugin:
                    pkt->Res1 = unloadPluginName(secBase, (STRPTR)pkt->Arg1);
                    break;

                case secSAction_Log:
                    /* Arg1: fmt, Arg2: SIPTR *args */
                    VLogF(secBase, (CONST_STRPTR)pkt->Arg1, (SIPTR *)pkt->Arg2);
                    pkt->Res1 = TRUE;
                    break;

                default:
                    pkt->Res1 = 0;
                    break;
                }
                ReplyMsg((struct Message *)pkt);
            }
        }

        if (signals & (1UL << monitorport->mp_SigBit))
            FreeRepliedMonMsg(secBase);

    } while (!quit);

    Forbid();
    secBase->ServerPort = NULL;
    Permit();

    /* Bounce whatever is still queued */
    while ((pkt = (struct secSPacket *)GetMsg(serverport)))
    {
        pkt->Res1 = 0;
        ReplyMsg((struct Message *)pkt);
    }

    FreeVolumes(secBase);

    secBase->MonitorPort = NULL;
    DeleteMsgPort(monitorport);
    DeleteMsgPort(serverport);
    FreeSignal(consistencysig);
    FreeSignal(notifysig);
    secBase->ConsistencySig = 0;
    secBase->NotifySig = 0;
    secBase->Server = NULL;
}

struct Process *CreateServer(struct SecurityBase *secBase)
{
    struct TagItem tags[] =
    {
        { NP_Entry,     (IPTR)ServerProcess     },
        { NP_Name,      (IPTR)SERVERNAME        },
        { NP_Priority,  SERVERPRI               },
        { NP_StackSize, SERVERSTACK             },
        { NP_UserData,  (IPTR)secBase           },
        { TAG_DONE,     0                       }
    };

    secBase->Server = CreateNewProc(tags);

    D(bug(DEBUG_NAME_STR " %s: '" SERVERNAME "' @ 0x%p\n", __func__, secBase->Server);)

    return secBase->Server;
}

/*
 * Activate the Server by sending the Startup Message
 */
BOOL StartServer(struct SecurityBase *secBase)
{
    if (!secBase->Server)
        return FALSE;
    return (BOOL)DoPkt(&secBase->Server->pr_MsgPort, ACTION_STARTUP, 0, 0, 0, 0, 0);
}
