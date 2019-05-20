
#include <aros/debug.h>

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
#include "security_support.h"

#define SERVERPRI               (4)
#define SERVERSTACK		(16384)

SIPTR SendServerPacket(struct SecurityBase *secBase, SIPTR type, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4)
{
    struct secSPacket pkt;
    struct MsgPort *port;

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if ((port = CreateMsgPort())) {

        /*
         *      Initialise the Server Packet
         */

        pkt.Msg.mn_Node.ln_Succ = NULL;
        pkt.Msg.mn_Node.ln_Pred = NULL;
        pkt.Msg.mn_Node.ln_Type = 0;
        pkt.Msg.mn_Node.ln_Pri = 0;
        pkt.Msg.mn_Node.ln_Name = NULL;
        pkt.Msg.mn_ReplyPort = port;
        pkt.Msg.mn_Length = sizeof(struct secSPacket);
        pkt.Type = type;
        pkt.Arg1 = arg1;
        pkt.Arg2 = arg2;
        pkt.Arg3 = arg3;
        pkt.Arg4 = arg4;
        pkt.Res1 = 0;

        /*
         *      Transmit the packet and wait for reply
         */

        Forbid();
        if (secBase->ServerPort) {
            PutMsg(secBase->ServerPort, (struct Message *)&pkt);
            Permit();
            do	{
                WaitPort(port);
            } while (GetMsg(port) != (struct Message *)&pkt);
        } else	{
            Permit();
        }
        DeleteMsgPort(port);
        return pkt.Res1;
    }
    return (SIPTR)NULL;
}

/*
 *      Fill in the User Information
 */

static void FillUserInfo(struct secUserDef *def, struct secPrivUserInfo *info)
{
    strncpy(info->Pub.UserID, def->UserID, secUSERIDSIZE-1);
    info->Pub.UserID[secUSERIDSIZE-1] = '\0';
    info->Pub.uid = def->uid;
    info->Pub.gid = def->gid;
    strncpy(info->Pub.UserName, def->UserName, secUSERNAMESIZE-1);
    info->Pub.UserName[secUSERNAMESIZE-1] = '\0';
    strncpy(info->Pub.HomeDir, def->HomeDir, secHOMEDIRSIZE-1);
    info->Pub.HomeDir[secHOMEDIRSIZE-1] = '\0';
    if (info->Pub.NumSecGroups)
        Free(info->Pub.SecGroups, info->Pub.NumSecGroups*sizeof(UWORD));
    if (def->NumSecGroups && (info->Pub.SecGroups = MAlloc(def->NumSecGroups*sizeof(UWORD)))) {
        info->Pub.NumSecGroups = def->NumSecGroups;
        CopyMem(def->SecGroups, info->Pub.SecGroups, def->NumSecGroups*sizeof(UWORD));
    } else {
        info->Pub.NumSecGroups = 0;
        info->Pub.SecGroups = NULL;
    }
    strncpy(info->Pub.Shell, def->Shell, secSHELLSIZE-1);
    info->Password = !!strlen(def->Password);
}

/*
 *      Fill in the Group Information
 */

static void FillGroupInfo(struct secGroupDef *def, struct secPrivGroupInfo *info)
{
    strncpy(info->Pub.GroupID, def->GroupID, secGROUPIDSIZE-1);
    info->Pub.GroupID[secGROUPIDSIZE-1] = '\0';
    info->Pub.gid = def->gid;
    info->Pub.MgrUid = def->MgrUid;
    strncpy(info->Pub.GroupName, def->GroupName, secGROUPNAMESIZE-1);
    info->Pub.GroupName[secGROUPNAMESIZE-1] = '\0';
}

/*
 *      Check if a user is authorised to login
 */

static struct secPrivUserInfo *CheckUser(struct SecurityBase *secBase, ULONG user, STRPTR userid, STRPTR pwd, BOOL nopasswd, BOOL nolog)
{
    BOOL found;
#if (0)
    char buffer[64];
#endif
    struct secPrivUserInfo *info = NULL;
    struct secUserDef *def;
    UWORD uid;

    uid = user>>16;

    if ((def = GetUserDefs((struct Library *)secBase)))
        do
            if (
                    (found = (!strcmp(userid, def->UserID)))
                    && (nopasswd || (verifypass(secBase, def->UserID, def->Password, pwd)))
                    && (info = (struct secPrivUserInfo *)secAllocUserInfo())
                )
                FillUserInfo(def, info);
            else
                def = def->Next;
        while (!found && def);

    if (info)
        CallMonitors(secBase, secTrgB_Login, uid, info->Pub.uid, userid);
    else
        CallMonitors(secBase, secTrgB_LoginFail, uid, 0, userid);

    if (!nolog && ((info && (secBase->Config.LogFlags & muLogF_Login)) ||
                                            (!info && (secBase->Config.LogFlags & muLogF_LoginFail)))) {
        SIPTR args[2];
        args[0] = uid;
        args[1] = (SIPTR)userid;
        if (info)
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_LOGIN), args);
        else
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_LOGINFAIL), args);
    }

    return(info);
}


/*
 *      Check the Password of a User
 */

static BOOL CheckPasswd(struct SecurityBase *secBase, ULONG user, STRPTR pwd)
{
    BOOL found;
    BOOL valid = FALSE;
    UWORD uid;
    char buffer[64];
    struct secUserDef *def;

    uid = user>>16;

    if ((def = GetUserDefs((struct Library *)secBase)))
        do
            if ((found = (def->uid == uid)))
                valid = Encrypt(buffer, pwd, def->UserID) && !strcmp(buffer, def->Password);
            else
                def = def->Next;
        while (!found && def);

    if (valid)
        CallMonitors(secBase, secTrgB_CheckPasswd, uid, 0, NULL);
    else
        CallMonitors(secBase, secTrgB_CheckPasswdFail, uid, 0, NULL);

    if ((valid && (secBase->Config.LogFlags & muLogF_CheckPasswd)) ||
         (!valid && (secBase->Config.LogFlags & muLogF_CheckPasswdFail))) {
        SIPTR args[1];
        args[0] = uid;
        if (valid)
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_CHECKPASSWD), args);
        else
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_CHECKPASSWDFAIL), args);
    }

    return(valid);
}


/*
 *      Change the Password of a user
 */

static BOOL Passwd(struct SecurityBase *secBase, ULONG user, STRPTR oldpwd, STRPTR newpwd)
{
    BOOL found;
    BOOL changed = FALSE;
    UWORD uid, gid;
    char buffer[64];
    struct secUserDef *def;

    uid = user>>16;
    gid = user&secMASK_GID;

    if (((uid <= secBase->Config.PasswduidLevel) || (gid <= secBase->Config.PasswdgidLevel)) &&
             (def = GetUserDefs((struct Library *)secBase))){
        do
            if ((found = (def->uid == uid)))
                changed = Encrypt(buffer, oldpwd, def->UserID) &&
                 !strcmp(buffer, def->Password) &&
                 Encrypt(def->Password, newpwd, def->UserID) && UpdateUserDefs((struct Library *)secBase);
            else
                def = def->Next;
        while (!found && def);
    }

    if (changed)
        CallMonitors(secBase, secTrgB_Passwd, uid, 0, NULL);
    else
        CallMonitors(secBase, secTrgB_PasswdFail, uid, 0, NULL);

    if ((changed && (secBase->Config.LogFlags & muLogF_Passwd)) ||
         (!changed && (secBase->Config.LogFlags & muLogF_PasswdFail))) {
        SIPTR args[1];
        args[0] = uid;
        if (changed)
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_PASSWD), args);
        else
            VLogF((struct Library *)secBase, GetLogStr(secBase, MSG_LOG_PASSWDFAIL), args);
    }

    return(changed);
}

/*
 *      Checker whether a user belongs to a group
 */

static BOOL Belongs2(struct secUserDef *def, UWORD gid)
{
    int i;

    if (def->gid == gid)
        return(TRUE);
    for (i = 0; i < def->NumSecGroups; i++)
        if (def->SecGroups[i] == gid)
            return(TRUE);
    return(FALSE);
}

/*
 *      Get Information about a User
 */

static struct secPrivUserInfo *GetUserInfo(struct SecurityBase *secBase, struct secPrivUserInfo *info, ULONG keytype)
{
    struct secUserDef *def;
    ULONG len;
    ULONG count = 0;

    if ((def = GetUserDefs((struct Library *)secBase))) {
        switch (keytype) {
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
                while (stricmp(def->UserName, info->Pub.UserName) && (def = def->Next))
                    count++;
                break;

            case secKeyType_WUserID:
                FreeV(info->Pattern);
                len = 2*strlen(info->Pub.UserID)+2;
                if ((info->Pattern = MAllocV(len)) &&
                     (ParsePatternNoCase(info->Pub.UserID, info->Pattern, len) != -1))
                    while (!MatchPatternNoCase(info->Pattern, def->UserID) && (def = def->Next))
                        count++;
                else {
                    FreeV(info->Pattern);
                    info->Pattern = NULL;
                    def = NULL;
                }
                break;

            case secKeyType_WUserIDNext:
                if (info->Pattern) {
                    while ((count <= info->Count) && (def = def->Next))
                        count++;
                    if (def)
                        while (!MatchPatternNoCase(info->Pattern, def->UserID) && (def = def->Next))
                            count++;
                } else
                    def = NULL;
                break;

            case secKeyType_WUserName:
                FreeV(info->Pattern);
                len = 2*strlen(info->Pub.UserName)+2;
                if ((info->Pattern = MAllocV(len)) &&
                     (ParsePatternNoCase(info->Pub.UserName, info->Pattern, len) != -1))
                    while (!MatchPatternNoCase(info->Pattern, def->UserName) && (def = def->Next))
                        count++;
                else {
                    FreeV(info->Pattern);
                    info->Pattern = NULL;
                    def = NULL;
                }
                break;

            case secKeyType_WUserNameNext:
                if (info->Pattern) {
                    while ((count <= info->Count) && (def = def->Next))
                        count++;
                    if (def)
                        while (!MatchPatternNoCase(info->Pattern, def->UserName) && (def = def->Next))
                            count++;
                } else
                    def = NULL;
                break;

            default:
                def = NULL;
                break;
        }
        if (def) {
            FillUserInfo(def, info);
            info->Count = count;
        } else
            info = NULL;
    } else	{
        info = NULL;
    }
    return(info);
}

/*
 *      Get Information about a Group
 */

static struct secPrivGroupInfo *GetGroupInfo(struct SecurityBase *secBase, struct secPrivGroupInfo *info, ULONG keytype)
{
    struct secGroupDef *def;
    ULONG len;
    ULONG count = 0;

    if ((def = GetGroupDefs((struct Library *)secBase))) {
        switch (keytype) {
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
                while (stricmp(def->GroupName, info->Pub.GroupName) && (def = def->Next))
                        count++;
                break;

            case secKeyType_WGroupID:
                FreeV(info->Pattern);
                len = 2*strlen(info->Pub.GroupID)+2;
                if ((info->Pattern = MAllocV(len)) &&
                         (ParsePatternNoCase(info->Pub.GroupID, info->Pattern, len) != -1))
                        while (!MatchPatternNoCase(info->Pattern, def->GroupID) && (def = def->Next))
                                count++;
                else {
                        FreeV(info->Pattern);
                        info->Pattern = NULL;
                        def = NULL;
                }
                break;

            case secKeyType_WGroupIDNext:
                if (info->Pattern) {
                        while ((count <= info->Count) && (def = def->Next))
                                count++;
                        if (def)
                                while (!MatchPatternNoCase(info->Pattern, def->GroupID) && (def = def->Next))
                                        count++;
                } else
                        def = NULL;
                break;

            case secKeyType_WGroupName:
                FreeV(info->Pattern);
                len = 2*strlen(info->Pub.GroupName)+2;
                if ((info->Pattern = MAllocV(len)) &&
                         (ParsePatternNoCase(info->Pub.GroupName, info->Pattern, len) != -1))
                        while (!MatchPatternNoCase(info->Pattern, def->GroupName) && (def = def->Next))
                                count++;
                else {
                        FreeV(info->Pattern);
                        info->Pattern = NULL;
                        def = NULL;
                }
                break;

            case secKeyType_WGroupNameNext:
                if (info->Pattern) {
                        while ((count <= info->Count) && (def = def->Next))
                                count++;
                        if (def)
                                while (!MatchPatternNoCase(info->Pattern, def->GroupName) && (def = def->Next))
                                        count++;
                } else
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
        if (def) {
            FillGroupInfo(def, info);
            info->Count = count;
        } else
            info = NULL;
    } else
        info = NULL;

    return(info);
}

/**/


/*
 *      The Server's Process
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

    D(
        bug( DEBUG_NAME_STR " %s()\n", __func__);
        bug( DEBUG_NAME_STR " %s: " SERVERNAME " starting\n", __func__);
    )

    secBase = (struct SecurityBase *)serverProc->pr_Task.tc_UserData;

    D(
        bug( DEBUG_NAME_STR " %s: secBase @ 0x%p\n", __func__, secBase);
        bug( DEBUG_NAME_STR " %s: waiting for startup packet\n", __func__);
    )
    /*
     *  Get Startup Message
     */

    spkt = WaitPkt();

    /*
     *  Do all necessary initialisations
     */

    serverProc->pr_WindowPtr = (APTR)-1;

    if (((secBase->NotifySig = AllocSignal(-1)) == -1) ||
         ((secBase->ConsistencySig = AllocSignal(-1)) == -1) || !(secBase->ServerPort = CreateMsgPort()) ||
         !(secBase->MonitorPort = CreateMsgPort()))
        /* Init NetInfo here */
    {
        D(bug( DEBUG_NAME_STR " %s: Init failed!\n", __func__);)
        ReplyPkt(spkt, DOSFALSE, 0);
        Die(NULL, AN_Unknown | AG_NoSignal);
    }

#if (0)
    D(bug( DEBUG_NAME_STR " %s: Adding mufsnetinfo.device\n", __func__);)
    if (! InitNetInfo())	{
        /* Die() */
    }
    D(bug( DEBUG_NAME_STR " %s: Adding mufsUserGroup.library\n", __func__);)
    if (! InitUserGroup())	{
        /* Die() */
    }
#endif
    /*
     *  Reply Startup Message
     */

    ReplyPkt(spkt, DOSTRUE, 0);

    D(bug( DEBUG_NAME_STR " %s: Initialising Volumes ..\n", __func__);)

    InitVolumes((struct Library *)secBase);

    D(bug( DEBUG_NAME_STR " %s: Going Live ..\n", __func__);)

    do {
        signals = Wait(1<<secBase->NotifySig | 1<<secBase->ConsistencySig |
                                                1<<secBase->ServerPort->mp_SigBit | 1<<secBase->MonitorPort->mp_SigBit);

        if (signals & 1<<secBase->NotifySig)
            FreeDefs();

        if (signals & 1<<secBase->ConsistencySig) {
            FreeVolumes((struct Library *)secBase);
            InitVolumes((struct Library *)secBase);
        }

        if (signals & 1<<secBase->ServerPort->mp_SigBit)
            while (!quit && (pkt = (struct secSPacket *)GetMsg(secBase->ServerPort))) {
                switch (pkt->Type) {
                    case secSAction_Quit:

                        /*
                         *		Quit
                         *
                         *
                         *		Arg1:	/
                         *		Arg2:	/
                         *		Arg3:	/
                         *
                         *		Res1:	BOOL success
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_Quit arrived\n", __func__);)
                        quit = TRUE;
                        pkt->Res1 = TRUE;

                        /*
                         *		Ensure the Server will be RemTask()ed BEFORE any
                         *		other task will get the processor !!
                         */

                        Forbid();
                        break;

                    case secSAction_CheckUser:

                        /*
                         *		CheckUser
                         *
                         *
                         *		Arg1:	STRPTR uid
                         *		Arg2:	STRPTR pwd
                         *		Arg3:	BOOL nopasswd
                         *
                         *		Res1:	struct secPrivUserInfo *info (NULL for failure)
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_CheckUser arrived\n", __func__);)
                        user = GetTaskOwner(secBase, pkt->Msg.mn_ReplyPort->mp_SigTask);
                        pkt->Res1 = (SIPTR)CheckUser(secBase, user, (STRPTR)pkt->Arg1, (STRPTR)pkt->Arg2,
                                             (BOOL)pkt->Arg3, (BOOL)pkt->Arg4);
                        break;

                    case secSAction_Passwd:

                        /*
                         *		Passwd
                         *
                         *
                         *		Arg1:	STRPTR oldpwd
                         *		Arg2:	STRPTR newpwd
                         *		Arg3:	/
                         *
                         *		Res1:	BOOL success
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_Passwd arrived\n", __func__);)
                        user = GetTaskOwner(secBase, pkt->Msg.mn_ReplyPort->mp_SigTask);
                        pkt->Res1 = (SIPTR)Passwd(secBase, user, (STRPTR)pkt->Arg1, (STRPTR)pkt->Arg2);
                        break;

                    case secSAction_GetUserInfo:

                        /*
                         *		GetUserInfo
                         *
                         *
                         *		Arg1:	struct secPrivUserInfo *info
                         *		Arg2:	ULONG keytype
                         *		Arg3:	/
                         *
                         *		Res1:	struct secPrivUserInfo *info (NULL for failure)
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_GetUserInfo arrived\n", __func__);)
                        pkt->Res1 = (SIPTR)GetUserInfo(secBase, (struct secPrivUserInfo *)pkt->Arg1, (ULONG)pkt->Arg2);
                        break;

                    case secSAction_CheckPasswd:

                        /*
                         *		CheckPasswd
                         *
                         *
                         *		Arg1:	STRPTR pwd
                         *		Arg2:	/
                         *		Arg3:	/
                         *
                         *		Res1:	BOOL success
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_CheckPasswd arrived\n", __func__);)
                        user = GetTaskOwner(secBase, pkt->Msg.mn_ReplyPort->mp_SigTask);
                        pkt->Res1 = (SIPTR)CheckPasswd(secBase, user, (STRPTR)pkt->Arg1);
                        break;

                    case secSAction_PasswdDirLock:

                        /*		PasswdDirLock
                         *
                         *
                         *		Arg1:	/
                         *		Arg2:	/
                         *		Arg3:	/
                         *
                         *		Res1:	BPTR lock
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_PasswdDirLock arrived\n", __func__);)
                        if (secBase->_pwdLock)
                                pkt->Res1 = (SIPTR)DupLock(secBase->_pwdLock);
                        break;

                    case secSAction_ConfigDirLock:

                        /*		ConfigDirLock
                         *
                         *
                         *		Arg1:	/
                         *		Arg2:	/
                         *		Arg3:	/
                         *
                         *		Res1:	BPTR lock
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_ConfigDirLock arrived\n", __func__);)
                        if (secBase->_cfgLock)
                                pkt->Res1 = (SIPTR)DupLock(secBase->_cfgLock);
                        break;

                    case secSAction_GetGroupInfo:

                        /*
                         *		GetGroupInfo
                         *
                         *
                         *		Arg1:	struct secPrivGroupInfo *info
                         *		Arg2:	ULONG keytype
                         *		Arg3:	/
                         *
                         *		Res1:	struct secPrivGroupInfo *info (NULL for failure)
                         */

                        D(bug( DEBUG_NAME_STR " %s: secSAction_GetGroupInfo arrived\n", __func__);)
                        pkt->Res1 = (SIPTR)GetGroupInfo(secBase, (struct secPrivGroupInfo *)pkt->Arg1, (ULONG)pkt->Arg2);
                        break;

                    case secSAction_InitModule:
                        {
                            secPluginModule * mod = (secPluginModule*)pkt->Arg1;
                            /*
                             * Init a module.
                             * Arg1:	PluginModule * mod
                             * Res1:	Return value of the init function for that module
                             */
                            D(bug( DEBUG_NAME_STR " %s: secSAction_InitModule arrived\n", __func__);)
                            pkt->Res1 = mod->header->Initialize((struct Library *)secBase, mod);
                        }
                        break;

                    case secSAction_FiniModule:
                        {
                            secPluginModule * mod = (secPluginModule*)pkt->Arg1;
                            /*
                             * Init a module.
                             * Arg1:	PluginModule * mod
                             * Res1:	Return value of the init function for that module
                             */
                            D(bug( DEBUG_NAME_STR " %s: secSAction_FiniModule arrived\n", __func__);)
                            mod->header->Terminate();
                            pkt->Res1 = TRUE;
                        }
                        break;

                    default:
                        break;
                }
                ReplyMsg((struct Message *)pkt);
            }

        if (signals & 1<<secBase->MonitorPort->mp_SigBit)
            FreeRepliedMonMsg(secBase);

    } while (!quit);

    FreeVolumes((struct Library *)secBase);

    DeleteMsgPort(secBase->MonitorPort);
    DeleteMsgPort(secBase->ServerPort);
    FreeSignal(secBase->ConsistencySig);
    FreeSignal(secBase->NotifySig);
    secBase->MonitorPort = NULL;
    secBase->ServerPort = NULL;
    secBase->ConsistencySig = 0;
    secBase->Server = NULL;
}

/**/

struct Process *CreateServer(struct SecurityBase *secBase)
{
    struct TagItem tags[] = {
        { NP_Entry,     (IPTR)ServerProcess     },
        { NP_Name,      (IPTR)SERVERNAME        },
        { NP_Priority,  SERVERPRI               },
        { NP_StackSize, SERVERSTACK             },
        { NP_UserData,  (IPTR)secBase           },
        { TAG_DONE,     0                       }
    };

    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    secBase->Server = CreateNewProc(tags);

    D(bug( DEBUG_NAME_STR " %s: '" SERVERNAME "' @ 0x%p\n", __func__, secBase->Server);)

    return (secBase->Server);
}

/*
*       Activate the Server by sending the Startup Message
*/

BOOL StartServer(struct SecurityBase *secBase)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    return((BOOL)DoPkt(&secBase->Server->pr_MsgPort, ACTION_STARTUP, 0, 0, 0, 0, 0));
}
