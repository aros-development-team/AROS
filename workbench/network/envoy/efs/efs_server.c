/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFS client - the server-to-client channel on the mount's public
          entity: volume information / liveness (action 5680) and
          notification events (5681) (re/spec/efs-protocol.md §4.5, §2.9).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "efs_intern.h"
#include <proto/nipc.h>

static void FireNotify(struct Globals *glob, struct EfsNotify *n)
{
    struct NotifyRequest *nr = n->NR;

    if (nr->nr_Flags & NRF_SEND_SIGNAL)
    {
        Signal(nr->nr_stuff.nr_Signal.nr_Task, 1UL << nr->nr_stuff.nr_Signal.nr_SignalNum);
    }
    else if (nr->nr_Flags & NRF_SEND_MESSAGE)
    {
        struct EfsNotifyMsg *m;

        if ((nr->nr_Flags & NRF_WAIT_REPLY) && nr->nr_MsgCount)
            return;
        if (!(m = AllocVec(sizeof(struct EfsNotifyMsg), MEMF_PUBLIC | MEMF_CLEAR)))
            return;
        m->NM.nm_ExecMessage.mn_ReplyPort = glob->NotifyPort;
        m->NM.nm_ExecMessage.mn_Length = sizeof(struct NotifyMessage);
        m->NM.nm_Class = NOTIFY_CLASS;
        m->NM.nm_Code = NOTIFY_CODE;
        m->NM.nm_NReq = nr;
        m->Owner = n;
        nr->nr_MsgCount++;
        PutMsg(nr->nr_stuff.nr_Msg.nr_Port, &m->NM.nm_ExecMessage);
    }
}

void HandleNotifyReplies(struct Globals *glob)
{
    struct EfsNotifyMsg *m;

    while ((m = (struct EfsNotifyMsg *)GetMsg(glob->NotifyPort)))
    {
        struct EfsNotify *n;
        ForeachNode(&glob->Notifies, n)
        {
            if (n == m->Owner && n->NR->nr_MsgCount)
            {
                n->NR->nr_MsgCount--;
                break;
            }
        }
        FreeVec(m);
    }
}

void FreeNotifies(struct Globals *glob)
{
    struct EfsNotify *n;
    while ((n = (struct EfsNotify *)RemHead((struct List *)&glob->Notifies)))
        FreeVec(n);
}

/* a request from the server on our entity */
static void ServerRequest(struct Globals *glob, struct Transaction *t)
{
    UBYTE *req = t->trans_RequestData;
    UBYTE *resp = t->trans_ResponseData ? t->trans_ResponseData : req;
    ULONG rlen = t->trans_ResponseData ? t->trans_RespDataLength : t->trans_ReqDataLength;
    ULONG action;

    t->trans_Error = 0;
    if (t->trans_Command != EFSCMD_PACKET || t->trans_ReqDataActual < EFS_HDR)
    {
        t->trans_Error = EFSERR_UNKNOWNCMD;
        t->trans_RespDataActual = 0;
        ReplyTransaction(t);
        return;
    }
    action = GetL(req, WH_ACTION);
    EFSLOG("server action %lu\n", (unsigned long)action);
    switch (action)
    {
    case EFSACT_VOLINFO:
        if (GetL(req, WH_MOUNTID) == glob->MountID && t->trans_ReqDataActual >= EFS_HDR + EFS_VOLBLOCK)
        {
            UBYTE blk[EFS_VOLBLOCK];
            memcpy(blk, req + EFS_HDR, EFS_VOLBLOCK);
            ProcessVolumeBlock(glob, blk);
        }
        /* the reply must still start with the mount ID; echo the whole request */
        if (resp != req)
            memcpy(resp, req, t->trans_ReqDataActual > rlen ? rlen : t->trans_ReqDataActual);
        t->trans_RespDataActual = t->trans_ReqDataActual > rlen ? rlen : t->trans_ReqDataActual;
        break;

    case EFSACT_NOTIFY:
    {
        ULONG key = GetL(req, WH_RES1);
        struct EfsNotify *n;
        ForeachNode(&glob->Notifies, n)
        {
            if (n->Key == key)
            {
                FireNotify(glob, n);
                break;
            }
        }
        if (resp != req)
            memcpy(resp, req, EFS_HDR);
        t->trans_RespDataActual = EFS_HDR;
        break;
    }

    default:
        if (resp != req)
            memcpy(resp, req, EFS_HDR);
        PutL(resp, WH_RES1, 0);
        PutL(resp, WH_RES2, ERROR_ACTION_NOT_KNOWN);
        t->trans_RespDataActual = EFS_HDR;
        break;
    }
    ReplyTransaction(t);
}

/*
 * Everything that arrives on the mount's entity: requests from the server
 * are served at once; our own returning transactions are marked as arrived
 * (trans_ClientPrivate = 1) for WaitFor().
 */
void HandleEntity(struct Globals *glob)
{
    struct Transaction *t;

    while ((t = GetTransaction(glob->Entity)))
    {
        if (t->trans_Type == TYPE_SERVICING)
            ServerRequest(glob, t);
        else
            t->trans_ClientPrivate = 1;
    }
}

/* Begin a transaction of ours; WaitFor() completes it */
BOOL StartTrans(struct Globals *glob, struct Transaction *t)
{
    t->trans_ClientPrivate = 0;
    if (!BeginTransaction(glob->Service, glob->Entity, t))
    {
        t->trans_ClientPrivate = 1;
        if (!t->trans_Error)
            t->trans_Error = ENVOYERR_CANTDELIVER;
        return FALSE;
    }
    return TRUE;
}

/*
 * Wait for one of our transactions while still serving the server's
 * requests: the server calls back (action 5680) before it answers a mount,
 * so a plain WaitTransaction() would deadlock (re/spec/efs-protocol.md §1.4).
 */
ULONG WaitFor(struct Globals *glob, struct Transaction *t)
{
    for (;;)
    {
        HandleEntity(glob);
        if (t->trans_ClientPrivate)
            return t->trans_Error;
        if (t->trans_Type == TYPE_NOT_ISSUED)
            return t->trans_Error ? t->trans_Error : ENVOYERR_CANTDELIVER;
        Wait(1UL << glob->EntSig);
    }
}

ULONG DoTrans(struct Globals *glob, struct Transaction *t)
{
    if (!StartTrans(glob, t))
        return t->trans_Error;
    return WaitFor(glob, t);
}
