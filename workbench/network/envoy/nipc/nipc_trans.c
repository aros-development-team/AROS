/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - transactions (the application side)
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

/* Synchronous request to the supervisor */
LONG SendSuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    struct MsgPort *port;

    if (!NIPCBase->SuperPort)
        return FALSE;
    if (FindTask(NULL) == (struct Task *)NIPCBase->Super)
    {
        NLOG(DEBUG_NAME_STR " %s: called from the supervisor itself\n", __func__);
        return FALSE;
    }
    if (!(port = CreateMsgPort()))
        return FALSE;
    req->Msg.mn_ReplyPort = port;
    req->Msg.mn_Length = sizeof(struct SuperReq);
    req->Caller = FindTask(NULL);
    Forbid();
    if (NIPCBase->SuperPort)
    {
        PutMsg(NIPCBase->SuperPort, &req->Msg);
        Permit();
        do
        {
            WaitPort(port);
        } while (GetMsg(port) != &req->Msg);
    }
    else
        Permit();
    DeleteMsgPort(port);
    return TRUE;
}

/* Asynchronous request: the supervisor frees it (mn_ReplyPort NULL) */
BOOL PostSuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    BOOL ok = FALSE;

    req->Msg.mn_ReplyPort = NULL;
    req->Msg.mn_Length = sizeof(struct SuperReq);
    req->Caller = FindTask(NULL);
    Forbid();
    if (NIPCBase->SuperPort)
    {
        PutMsg(NIPCBase->SuperPort, &req->Msg);
        ok = TRUE;
    }
    Permit();
    return ok;
}

/* Every completion goes through here: type RESPONSE, back to the source */
void ReturnTransaction(struct NIPCBase *NIPCBase, struct Transaction *t, ULONG error)
{
    struct MsgPort *port;

    if (error)
        t->trans_Error = error;
    t->trans_Type = TYPE_RESPONSE;
    port = t->trans_Msg.mn_ReplyPort ? t->trans_Msg.mn_ReplyPort : &t->trans_SourceEntity->Port;
    PutMsg(port, &t->trans_Msg);
}

/* A transaction built by the library for a received request (§4.3) */
struct Transaction *AllocRequestTransaction(struct NIPCBase *NIPCBase, ULONG reqlen, ULONG resplen, BOOL inplace)
{
    struct PrivTransaction *p;
    ULONG reqalloc = inplace ? (reqlen > resplen ? reqlen : resplen) : reqlen;

    if (!(p = AllocVec(sizeof(struct PrivTransaction), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    p->Magic = PRIVTRANS_MAGIC;
    p->Pub.trans_Msg.mn_Node.ln_Type = NT_MESSAGE;
    p->Pub.trans_Msg.mn_Length = sizeof(struct Transaction);
    p->Pub.trans_Type = TYPE_NOT_ISSUED;
    if (reqalloc)
    {
        if (!(p->Pub.trans_RequestData = AllocVec(reqalloc, MEMF_CLEAR | MEMF_PUBLIC)))
        {
            FreeVec(p);
            return NULL;
        }
        p->Pub.trans_Flags |= TRANSF_REQBUFFERALLOC;
    }
    p->Pub.trans_ReqDataLength = reqalloc;
    p->Pub.trans_ReqDataActual = reqlen;
    if (inplace)
    {
        p->Pub.trans_ResponseData = p->Pub.trans_RequestData;
    }
    else if (resplen)
    {
        if (!(p->Pub.trans_ResponseData = AllocVec(resplen, MEMF_CLEAR | MEMF_PUBLIC)))
        {
            if (p->Pub.trans_RequestData)
                FreeVec(p->Pub.trans_RequestData);
            FreeVec(p);
            return NULL;
        }
        p->Pub.trans_Flags |= TRANSF_RESPBUFFERALLOC;
    }
    p->Pub.trans_RespDataLength = resplen;
    p->Pub.trans_RespDataActual = resplen;
    return &p->Pub;
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct Transaction *, AllocTransactionA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 19, NIPC)

/*  FUNCTION
        Allocate a transaction. TRN_AllocReqBuffer / TRN_AllocRespBuffer
        (sizes) also allocate the buffers; TRN_ReqDataNIPCBuff /
        TRN_RespDataNIPCBuff declare that the caller's buffers are
        struct NIPCBuff.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PrivTransaction *p;
    ULONG reqlen = GetTagData(TRN_AllocReqBuffer, 0, tags);
    ULONG resplen = GetTagData(TRN_AllocRespBuffer, 0, tags);

    if (!(p = AllocVec(sizeof(struct PrivTransaction), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    p->Magic = PRIVTRANS_MAGIC;
    p->Pub.trans_Msg.mn_Node.ln_Type = NT_MESSAGE;
    p->Pub.trans_Msg.mn_Length = sizeof(struct Transaction);
    p->Pub.trans_Type = TYPE_NOT_ISSUED;
    if (GetTagData(TRN_ReqDataNIPCBuff, FALSE, tags))
        p->Pub.trans_Flags |= TRANSF_REQNIPCBUFF;
    if (GetTagData(TRN_RespDataNIPCBuff, FALSE, tags))
        p->Pub.trans_Flags |= TRANSF_RESPNIPCBUFF;
    if (reqlen)
    {
        if (!(p->Pub.trans_RequestData = AllocVec(reqlen, MEMF_CLEAR | MEMF_PUBLIC)))
        {
            FreeVec(p);
            return NULL;
        }
        p->Pub.trans_Flags |= TRANSF_REQBUFFERALLOC;
        p->Pub.trans_ReqDataLength = p->Pub.trans_ReqDataActual = reqlen;
    }
    if (resplen)
    {
        if (!(p->Pub.trans_ResponseData = AllocVec(resplen, MEMF_CLEAR | MEMF_PUBLIC)))
        {
            if (p->Pub.trans_RequestData)
                FreeVec(p->Pub.trans_RequestData);
            FreeVec(p);
            return NULL;
        }
        p->Pub.trans_Flags |= TRANSF_RESPBUFFERALLOC;
        p->Pub.trans_RespDataLength = p->Pub.trans_RespDataActual = resplen;
    }
    return &p->Pub;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(APTR, FreeTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Transaction *, transaction, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 20, NIPC)

/*  FUNCTION
        Free a transaction and the buffers AllocTransactionA() allocated.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t = transaction;

    if (!t)
        return NULL;
    if (PRIVTRANS(t)->Magic != PRIVTRANS_MAGIC)
    {
        NLOG(DEBUG_NAME_STR " FreeTransaction(%p): not from AllocTransactionA()\n", t);
        return NULL;
    }
    if ((t->trans_Flags & TRANSF_RESPBUFFERALLOC) && t->trans_ResponseData && t->trans_ResponseData != t->trans_RequestData)
        FreeVec(t->trans_ResponseData);
    if ((t->trans_Flags & TRANSF_REQBUFFERALLOC) && t->trans_RequestData)
        FreeVec(t->trans_RequestData);
    PRIVTRANS(t)->Magic = 0;
    FreeVec(t);
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(BOOL, BeginTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, dest_entity, A0),
        AROS_LHA(struct Entity *, src_entity, A1),
        AROS_LHA(struct Transaction *, transaction, A2),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 26, NIPC)

/*  FUNCTION
        Start a transaction from src_entity to dest_entity. The structure
        belongs to the library until it returns to the source entity as
        TYPE_RESPONSE. A local destination receives the structure itself;
        a remote one receives the request data over the link, and the
        response is written back into the response buffer.

        Unlike the original, this implementation never blocks the caller
        while the link's transmit window is full: fragments are queued and
        sent as the window opens.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t = transaction;

    if (!t || !src_entity)
        return FALSE;
    t->trans_SourceEntity = src_entity;
    t->trans_DestinationEntity = dest_entity;
    t->trans_Error = 0;
    t->trans_Msg.mn_Node.ln_Type = NT_MESSAGE;
    t->trans_Msg.mn_ReplyPort = NULL;

    if (!dest_entity || (dest_entity->Flags & ENTF_DELETED))
    {
        t->trans_Type = TYPE_REQUEST;
        ReturnTransaction(NIPCBase, t, ENVOYERR_CANTDELIVER);
        return TRUE;
    }

    if (dest_entity->Flags & ENTF_LINK)
    {
        struct SuperReq *req;

        if (!(req = AllocVec(sizeof(struct SuperReq), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            t->trans_Type = TYPE_REQUEST;
            ReturnTransaction(NIPCBase, t, ENVOYERR_NORESOURCES);
            return TRUE;
        }
        t->trans_Type = TYPE_REQUEST;
        req->Type = SREQ_TRANSACT;
        req->Entity = dest_entity;
        req->Trans = t;
        if (!PostSuperReq(NIPCBase, req))
        {
            FreeVec(req);
            ReturnTransaction(NIPCBase, t, ENVOYERR_CANTDELIVER);
        }
        return TRUE;
    }

    /* local: the structure is the message */
    t->trans_Type = TYPE_REQUEST;
    PutMsg(&dest_entity->Port, &t->trans_Msg);
    return TRUE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, DoTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, dest_entity, A0),
        AROS_LHA(struct Entity *, src_entity, A1),
        AROS_LHA(struct Transaction *, transaction, A2),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 25, NIPC)

/*  FUNCTION
        BeginTransaction() followed by WaitTransaction().

    RESULT
        trans_Error.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!BeginTransaction(dest_entity, src_entity, transaction))
        return ENVOYERR_NULLPTR;
    return WaitTransaction(transaction);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(struct Transaction *, GetTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Entity *, entity, A0),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 27, NIPC)

/*  FUNCTION
        Take the next transaction off an entity: a request (returned as
        TYPE_SERVICING) or one of the entity's own returning transactions
        (TYPE_RESPONSE).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t;

    if (!entity)
        return NULL;
    if ((t = (struct Transaction *)GetMsg(&entity->Port)))
    {
        if (t->trans_Type == TYPE_REQUEST)
            t->trans_Type = TYPE_SERVICING;
    }
    return t;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, ReplyTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Transaction *, transaction, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 28, NIPC)

/*  FUNCTION
        Return a serviced transaction to its sender, with the response data
        and trans_Error. For a request that came over the network the
        library sends the response and frees the transaction; do not touch
        it afterwards.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t = transaction;

    if (!t)
        return;
    if (t->trans_Type != TYPE_SERVICING)
    {
        NLOG(DEBUG_NAME_STR " ReplyTransaction(%p): type %d is not TYPE_SERVICING\n", t, t->trans_Type);
        return;
    }
    if (t->trans_SourceEntity && (t->trans_SourceEntity->Flags & ENTF_SERVERLINK))
    {
        struct SuperReq *req;

        if ((req = AllocVec(sizeof(struct SuperReq), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            req->Type = SREQ_REPLY;
            req->Entity = t->trans_SourceEntity;
            req->Trans = t;
            if (PostSuperReq(NIPCBase, req))
                return;
            FreeVec(req);
        }
        /* no supervisor: nothing can be sent; drop the transaction */
        FreeTransaction(t);
        return;
    }
    ReturnTransaction(NIPCBase, t, 0);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, CheckTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Transaction *, transaction, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 29, NIPC)

/*  FUNCTION
        TRUE if the transaction is complete, failed, or was never sent.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!transaction)
        return TRUE;
    return (transaction->trans_Type == TYPE_RESPONSE || transaction->trans_Type == TYPE_NOT_ISSUED);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, AbortTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Transaction *, transaction, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 30, NIPC)

/*  FUNCTION
        Abort a transaction started with BeginTransaction() if it is still
        a request: it returns to the source entity with ENVOYERR_ABORTED.
        Follow with WaitTransaction(). Nothing is sent on the wire.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t = transaction;

    if (!t || t->trans_Type != TYPE_REQUEST)
        return;
    if (t->trans_DestinationEntity && (t->trans_DestinationEntity->Flags & ENTF_LINK))
    {
        struct SuperReq req;
        memset(&req, 0, sizeof(req));
        req.Type = SREQ_ABORT;
        req.Entity = t->trans_DestinationEntity;
        req.Trans = t;
        SendSuperReq(NIPCBase, &req);
        return;
    }
    /* local: only while the server has not fetched it */
    Forbid();
    if (t->trans_Type == TYPE_REQUEST && t->trans_Msg.mn_Node.ln_Type == NT_MESSAGE)
    {
        Remove(&t->trans_Msg.mn_Node);
        Permit();
        ReturnTransaction(NIPCBase, t, ENVOYERR_ABORTED);
        return;
    }
    Permit();

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, WaitTransaction,

/*  SYNOPSIS */
        AROS_LHA(struct Transaction *, transaction, A1),

/*  LOCATION */
        struct NIPCBase *, NIPCBase, 31, NIPC)

/*  FUNCTION
        Wait until the transaction has completed, then remove it from the
        source entity and return trans_Error. Returns at once for a
        transaction that was never sent.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Transaction *t = transaction;
    struct MsgPort *port;

    if (!t)
        return ENVOYERR_NULLPTR;
    if (t->trans_Type == TYPE_NOT_ISSUED)
        return t->trans_Error;
    port = t->trans_Msg.mn_ReplyPort ? t->trans_Msg.mn_ReplyPort : (t->trans_SourceEntity ? &t->trans_SourceEntity->Port : NULL);
    if (!port)
        return t->trans_Error;
    for (;;)
    {
        Disable();
        if (t->trans_Type == TYPE_RESPONSE)
        {
            /* it is on the port's list once ReturnTransaction() has run */
            if (t->trans_Msg.mn_Node.ln_Succ)
                Remove(&t->trans_Msg.mn_Node);
            Enable();
            break;
        }
        Enable();
        if (port->mp_Flags == PA_SIGNAL && port->mp_SigTask == FindTask(NULL))
            Wait(1UL << port->mp_SigBit);
        else
            Delay(1);
    }
    return t->trans_Error;

    AROS_LIBFUNC_EXIT
}
