/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - entity connections: transactions over RDP links,
          FindEntity for remote hosts, pings. re/spec/nipc-transactions.md.
          Runs in the supervisor.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

#define ENTITY(n)               ((struct Entity *)((IPTR)(n) - offsetof(struct Entity, Node)))
#define TRANS(n)                ((struct Transaction *)((IPTR)(n) - offsetof(struct PrivTransaction, Node)))

/* A received fragment awaiting reassembly */
struct RxFrag
{
    struct MinNode      Node;
    ULONG               Seq;
    ULONG               Aux;
    ULONG               Len;
    UWORD               FragNo;
    UBYTE               Cmd;
    UBYTE               Flags;
    UBYTE               Data[0];
};

/* FindEntity in progress */
#define FINDER_RESOLVING        1
#define FINDER_WAITOPEN         2
#define FINDER_WAITREPLY        3
#define FINDER_WAITENTITY       4

struct Finder
{
    struct MinNode      Node;
    struct SuperReq     *Req;
    UWORD               State;
    UWORD               Countdown;
    ULONG               IP;
    struct RdpConn      *ResConn;
    struct Entity       *Link;
    struct Inquiry      *Inq;
    char                EntityName[80];
};

struct PingReq
{
    struct MinNode      Node;
    struct SuperReq     *Req;
    struct Entity       *Link;
    ULONG               Cookie;
    struct timeval      Sent;
    struct timeval      Deadline;
};

static ULONG PingCookie = 0x4E495043;

static void FreeFrags(struct MinList *l)
{
    struct RxFrag *f;
    while ((f = (struct RxFrag *)RemHead((struct List *)l)))
        FreeVec(f);
}

/* ---- data movement for fragments --------------------------------------- */

static ULONG TransDataLen(struct Transaction *t, BOOL request)
{
    return request ? t->trans_ReqDataActual : t->trans_RespDataActual;
}

static void TransCopyOut(struct NIPCBase *NIPCBase, struct Transaction *t, BOOL request, ULONG off, UBYTE *dst, ULONG len)
{
    APTR src = request ? t->trans_RequestData : t->trans_ResponseData;
    BOOL nb = request ? (t->trans_Flags & TRANSF_REQNIPCBUFF) : (t->trans_Flags & TRANSF_RESPNIPCBUFF);

    if (!src || !len)
        return;
    if (nb)
        CopyFromNIPCBuff((struct NIPCBuff *)src, dst, off, len);
    else
        CopyMem((UBYTE *)src + off, dst, len);
}

static ULONG TransCopyIn(struct NIPCBase *NIPCBase, struct Transaction *t, ULONG off, const UBYTE *src, ULONG len)
{
    if (!t->trans_ResponseData || !len)
        return 0;
    if (t->trans_Flags & TRANSF_RESPNIPCBUFF)
        return CopyToNIPCBuff((UBYTE *)src, (struct NIPCBuff *)t->trans_ResponseData, off, len);
    CopyMem((APTR)src, (UBYTE *)t->trans_ResponseData + off, len);
    return len;
}

/* ---- sending: one fragment per RDP segment, paced by the window -------- */

static void FailOutgoing(struct NIPCBase *NIPCBase, struct Entity *link, struct Transaction *t, ULONG error)
{
    Remove((struct Node *)&PRIVTRANS(t)->Node);
    if (link->Flags & ENTF_SERVERLINK)
    {
        /* a response that cannot be sent: nothing to tell anyone */
        FreeTransaction(t);
        if (link->UseCount > 0)
            link->UseCount--;
    }
    else
        ReturnTransaction(NIPCBase, t, error);
}

static void TeardownServerLink(struct NIPCBase *NIPCBase, struct Entity *link);

static void PumpLink(struct NIPCBase *NIPCBase, struct Entity *link)
{
    struct RdpConn *conn = link->Conn;
    struct MinNode *n;

    while (conn && conn->State == RDP_STATE_OPEN && !RdpWindowFull(conn) && (n = (struct MinNode *)GetHead(&link->Outgoing)))
    {
        struct Transaction *t = TRANS(n);
        struct PrivTransaction *p = PRIVTRANS(t);
        BOOL request = !(link->Flags & ENTF_SERVERLINK);
        ULONG total = TransDataLen(t, request);
        ULONG limit = conn->Mss > NIPC_HEADER_SIZE ? conn->Mss - NIPC_HEADER_SIZE : 1;
        ULONG piece = total - p->TxOffset;
        BOOL last;
        UBYTE *seg;

        if (piece > limit)
            piece = limit;
        last = (p->TxOffset + piece >= total);
        if (!(seg = AllocVec(NIPC_HEADER_SIZE + piece, MEMF_PUBLIC)))
        {
            FailOutgoing(NIPCBase, link, t, ENVOYERR_NORESOURCES);
            continue;
        }
        seg[0] = 0;
        seg[1] = 0;
        seg[2] = t->trans_Command;
        seg[3] = (last ? 0x01 : 0) | ((request && t->trans_RequestData == t->trans_ResponseData && t->trans_RequestData) ? 0x02 : 0);
        nipc_put32(seg + 4, t->trans_Sequence);
        nipc_put32(seg + 8, piece);
        nipc_put32(seg + 12, request ? t->trans_RespDataLength : t->trans_Error);
        nipc_put16(seg + 16, p->TxFragNo);
        TransCopyOut(NIPCBase, t, request, p->TxOffset, seg + NIPC_HEADER_SIZE, piece);
        if (!RdpSend(conn, seg, NIPC_HEADER_SIZE + piece))
        {
            FreeVec(seg);
            FailOutgoing(NIPCBase, link, t, ENVOYERR_NORESOURCES);
            continue;
        }
        FreeVec(seg);
        p->TxOffset += piece;
        p->TxFragNo++;
        if (last)
        {
            Remove((struct Node *)&p->Node);
            if (request)
            {
                AddTail((struct List *)&link->Pending, (struct Node *)&p->Node);
                p->Timer = t->trans_Timeout;
            }
            else
            {
                FreeTransaction(t);
                if (link->UseCount > 0)
                    link->UseCount--;
                if ((link->Flags & ENTF_CONNDEAD) && link->UseCount == 0)
                {
                    TeardownServerLink(NIPCBase, link);
                    return;
                }
            }
        }
    }
}

void LinkPumpAll(struct NIPCBase *NIPCBase)
{
    struct MinNode *n, *next;

    ForeachNodeSafe(&NIPCBase->Entities, n, next)
    {
        struct Entity *e = ENTITY(n);
        if ((e->Flags & ENTF_LINK) && !IsMinListEmpty(&e->Outgoing))
            PumpLink(NIPCBase, e);
    }
}

/* ---- pings ------------------------------------------------------------- */

static void PingAnswer(struct RdpConn *conn, UBYTE *data)
{
    UBYTE pong[6];
    CopyMem(data, pong, 6);
    pong[0] = 2;
    RdpSend(conn, pong, 6);
}

static void PingMatch(struct NIPCBase *NIPCBase, struct Entity *link, UBYTE *data)
{
    ULONG cookie = nipc_get32(data + 2);
    struct PingReq *p;
    struct timeval now;

    ForeachNode(&NIPCBase->Pings, p)
    {
        if (p->Link == link && p->Cookie == cookie)
        {
            GetNow(NIPCBase, &now);
            p->Req->Value = TimevalDiffUs(&now, &p->Sent);
            Remove((struct Node *)p);
            ReplySuperReq(NIPCBase, p->Req);
            FreeVec(p);
            return;
        }
    }
}

static void CancelPings(struct NIPCBase *NIPCBase, struct Entity *link)
{
    struct PingReq *p, *next;

    ForeachNodeSafe(&NIPCBase->Pings, p, next)
    {
        if (!link || p->Link == link)
        {
            p->Req->Value = 0xFFFFFFFF;
            Remove((struct Node *)p);
            ReplySuperReq(NIPCBase, p->Req);
            FreeVec(p);
        }
    }
}

/* ---- client side of a link --------------------------------------------- */

static void FailPending(struct NIPCBase *NIPCBase, struct Entity *link, ULONG error)
{
    struct MinNode *n;

    while ((n = (struct MinNode *)GetHead(&link->Pending)))
    {
        Remove((struct Node *)n);
        ReturnTransaction(NIPCBase, TRANS(n), error);
    }
    while ((n = (struct MinNode *)GetHead(&link->Outgoing)))
        FailOutgoing(NIPCBase, link, TRANS(n), error);
    FreeFrags(&link->RxQueue);
}

void LinkConnDied(struct NIPCBase *NIPCBase, struct Entity *link)
{
    link->Flags |= ENTF_CONNDEAD;
    FailPending(NIPCBase, link, ENVOYERR_CANTDELIVER);
    CancelPings(NIPCBase, link);
}

static struct Transaction *FindPendingSeq(struct Entity *link, ULONG seq)
{
    struct MinNode *n;
    ForeachNode(&link->Pending, n)
        if (TRANS(n)->trans_Sequence == seq)
            return TRANS(n);
    return NULL;
}

static struct RxFrag *QueueFrag(struct Entity *link, UBYTE *data, ULONG len)
{
    struct RxFrag *f;
    ULONG dlen = len - NIPC_HEADER_SIZE;

    if (!(f = AllocVec(sizeof(struct RxFrag) + dlen, MEMF_PUBLIC)))
        return NULL;
    f->Cmd = data[2];
    f->Flags = data[3];
    f->Seq = nipc_get32(data + 4);
    f->Len = nipc_get32(data + 8);
    if (f->Len > dlen)
        f->Len = dlen;
    f->Aux = nipc_get32(data + 12);
    f->FragNo = nipc_get16(data + 16);
    CopyMem(data + NIPC_HEADER_SIZE, f->Data, f->Len);
    AddTail((struct List *)&link->RxQueue, (struct Node *)f);
    return f;
}

/* Response fragments (§1.3, client rules) */
static void ClientFragment(struct NIPCBase *NIPCBase, struct Entity *link, UBYTE *data, ULONG len)
{
    ULONG seq = nipc_get32(data + 4);
    struct Transaction *t;
    struct RxFrag *f, *next;
    UWORD expect = 0;
    ULONG stored = 0;
    BOOL bad = FALSE, full = FALSE;

    if (!(t = FindPendingSeq(link, seq)))
        return;                                 /* no transaction pending: dropped */
    if (!QueueFrag(link, data, len))
        return;
    if (!(data[3] & 0x01))
        return;                                 /* wait for the last fragment */

    t->trans_Command = data[2];
    t->trans_Error = nipc_get32(data + 12);
    t->trans_RespDataActual = 0;
    ForeachNodeSafe(&link->RxQueue, f, next)
    {
        if (f->Seq != seq)
        {
            Remove((struct Node *)f);
            FreeVec(f);
            continue;
        }
        if (!bad && !full)
        {
            if (f->FragNo != expect)
            {
                NLOG(DEBUG_NAME_STR " wrong response fragment %d cmd $%02x, expected %d\n", f->FragNo, f->Cmd, expect);
                bad = TRUE;
            }
            else
            {
                ULONG room = t->trans_RespDataLength > stored ? t->trans_RespDataLength - stored : 0;
                ULONG n = f->Len > room ? room : f->Len;
                stored += TransCopyIn(NIPCBase, t, stored, f->Data, n);
                if (n < f->Len)
                {
                    t->trans_Error = ENVOYERR_SMALLRESPBUFF;
                    full = TRUE;
                }
                expect++;
            }
        }
        Remove((struct Node *)f);
        FreeVec(f);
    }
    if (bad)
        return;                                 /* stays pending until timeout or abort */
    t->trans_RespDataActual = stored;
    Remove((struct Node *)&PRIVTRANS(t)->Node);
    ReturnTransaction(NIPCBase, t, 0);
}

static void ClientDataIn(struct RdpConn *conn, UBYTE *data, ULONG len, APTR userdata)
{
    struct Entity *link = userdata;
    struct NIPCBase *NIPCBase = link->Base;

    if (!len)
        return;
    switch (data[0])
    {
    case 0:
        if (len >= NIPC_HEADER_SIZE)
            ClientFragment(NIPCBase, link, data, len);
        break;
    case 1:
        if (len >= 6)
            PingAnswer(conn, data);
        break;
    case 2:
        if (len >= 6)
            PingMatch(NIPCBase, link, data);
        break;
    }
}

static void FinderConnStatus(struct NIPCBase *NIPCBase, struct Entity *link, struct RdpConn *conn);

static void ClientStatus(struct RdpConn *conn, APTR userdata)
{
    struct Entity *link = userdata;
    struct NIPCBase *NIPCBase = link->Base;

    if (link->Finder)
    {
        FinderConnStatus(NIPCBase, link, conn);
        return;
    }
    if (conn->State == RDP_STATE_CLOSED)
        LinkConnDied(NIPCBase, link);
}

/* ---- server side of a link --------------------------------------------- */

static void TeardownServerLink(struct NIPCBase *NIPCBase, struct Entity *link)
{
    struct Entity *owner = link->Owner;

    CancelPings(NIPCBase, link);
    FreeFrags(&link->RxQueue);
    {
        struct MinNode *n;
        while ((n = (struct MinNode *)GetHead(&link->Outgoing)))
        {
            Remove((struct Node *)n);
            FreeTransaction(TRANS(n));
        }
    }
    if (link->Conn)
    {
        RdpClose(link->Conn);
        link->Conn = NULL;
    }
    FreeEntity(NIPCBase, link);
    if (owner)
        ReleaseEntity(NIPCBase, owner);
}

/* Request fragments (§1.3, server rules) */
static void ServerFragment(struct NIPCBase *NIPCBase, struct Entity *link, UBYTE *data, ULONG len)
{
    ULONG seq = nipc_get32(data + 4);
    struct RxFrag *f, *next;
    struct Transaction *t;
    struct Entity *owner = link->Owner;
    UWORD expect = 0;
    ULONG total = 0, off = 0, aux, flags;
    UBYTE cmd;
    BOOL bad = FALSE;

    if (!QueueFrag(link, data, len))
        return;
    if (!(data[3] & 0x01))
        return;
    cmd = data[2];
    flags = data[3];
    aux = nipc_get32(data + 12);

    /* first pass: validate numbering and sum the length */
    ForeachNode(&link->RxQueue, f)
    {
        if (f->Seq != seq)
            continue;
        if (f->FragNo != expect)
        {
            NLOG(DEBUG_NAME_STR " wrong request fragment %d cmd $%02x, expected %d\n", f->FragNo, f->Cmd, expect);
            bad = TRUE;
            break;
        }
        expect++;
        total += f->Len;
    }
    if (bad || !owner || (owner->Flags & ENTF_DELETED) ||
        !(t = AllocRequestTransaction(NIPCBase, total, aux, (flags & 0x02) ? TRUE : FALSE)))
    {
        FreeFrags(&link->RxQueue);
        return;                                 /* no response of any kind */
    }
    ForeachNodeSafe(&link->RxQueue, f, next)
    {
        if (f->Seq == seq && f->Len)
        {
            CopyMem(f->Data, (UBYTE *)t->trans_RequestData + off, f->Len);
            off += f->Len;
        }
        Remove((struct Node *)f);
        FreeVec(f);
    }
    t->trans_Command = cmd;
    t->trans_Sequence = seq;
    t->trans_SourceEntity = link;
    t->trans_DestinationEntity = NULL;
    t->trans_Type = TYPE_REQUEST;
    t->trans_Error = 0;
    link->UseCount++;                           /* in service until replied */
    link->IdleSeconds = 0;
    PutMsg(&owner->Port, &t->trans_Msg);
}

static void ServerDataIn(struct RdpConn *conn, UBYTE *data, ULONG len, APTR userdata)
{
    struct Entity *link = userdata;
    struct NIPCBase *NIPCBase = link->Base;

    if (!len)
        return;
    switch (data[0])
    {
    case 0:
        if (len >= NIPC_HEADER_SIZE)
            ServerFragment(NIPCBase, link, data, len);
        break;
    case 1:
        if (len >= 6)
            PingAnswer(conn, data);
        break;
    case 2:
        if (len >= 6)
            PingMatch(NIPCBase, link, data);
        break;
    }
}

static void ServerStatus(struct RdpConn *conn, APTR userdata)
{
    struct Entity *link = userdata;
    struct NIPCBase *NIPCBase = link->Base;

    if (conn->State == RDP_STATE_CLOSED)
    {
        link->Flags |= ENTF_CONNDEAD;
        if (link->UseCount == 0)
            TeardownServerLink(NIPCBase, link);
    }
}

/* Called by the resolver for every accepted query: a server link with its
 * own one-shot listener. Returns the port, 0 on failure. */
UWORD CreateServerLink(struct NIPCBase *NIPCBase, struct Entity *owner, CONST_STRPTR srcname, CONST_STRPTR srchost)
{
    struct Entity *link;
    struct RdpConn *conn;

    if (!(link = AllocEntity(NIPCBase, srcname, ENTF_LINK | ENTF_SERVERLINK)))
        return 0;
    strncpy(link->HostName, srchost, NIPC_HOSTSIZE - 1);
    link->Owner = owner;
    if (!(conn = RdpOpenPassive(NIPCBase, 0, FALSE, ServerDataIn, ServerStatus, link)))
    {
        FreeEntity(NIPCBase, link);
        return 0;
    }
    link->Conn = conn;
    ObtainSemaphore(&NIPCBase->Sem);
    owner->UseCount++;
    ReleaseSemaphore(&NIPCBase->Sem);
    return conn->LocalPort;
}

/* ---- FindEntity -------------------------------------------------------- */

static void FinderDone(struct NIPCBase *NIPCBase, struct Finder *f, struct Entity *result, ULONG error)
{
    if (f->Inq)
    {
        InquiryCancel(NIPCBase, f->Inq);
        f->Inq = NULL;
    }
    if (f->ResConn)
    {
        RdpClose(f->ResConn);
        f->ResConn = NULL;
    }
    if (!result && f->Link)
    {
        struct Entity *src = f->Link->Owner;
        f->Link->Finder = NULL;
        if (f->Link->Conn)
            RdpClose(f->Link->Conn);
        FreeEntity(NIPCBase, f->Link);
        if (src)
            ReleaseEntity(NIPCBase, src);
        f->Link = NULL;
    }
    if (result && (result->Flags & ENTF_LINK))
        result->Finder = NULL;
    f->Req->Result = result;
    f->Req->Error = result ? 0 : error;
    Remove((struct Node *)f);
    ReplySuperReq(NIPCBase, f->Req);
    FreeVec(f);
}

static void FinderResolved(struct NIPCBase *NIPCBase, struct Inquiry *q, ULONG ip)
{
    struct Finder *f = q->DoneData;

    f->Inq = NULL;      /* the inquiry is finished by the caller */
    if (!ip)
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_UNKNOWNHOST);
        return;
    }
    f->IP = ip;
    f->State = FINDER_WAITOPEN;
    f->Countdown = 30;
    if (!(f->ResConn = RdpOpenActive(NIPCBase, ip, NIPC_RESOLVER_PORT, NULL, NULL, f)))
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOLVER);
        return;
    }
    /* callbacks installed after creation so that an immediate failure above is clean */
    extern void FinderResDataIn(struct RdpConn *, UBYTE *, ULONG, APTR);
    extern void FinderResStatus(struct RdpConn *, APTR);
    f->ResConn->DataIn = FinderResDataIn;
    f->ResConn->Status = FinderResStatus;
}

static void SendResolverQuery(struct NIPCBase *NIPCBase, struct Finder *f)
{
    UBYTE q[288];
    struct Entity *src = f->Req->Entity;

    memset(q, 0, sizeof(q));
    strncpy((char *)q, f->EntityName, 79);
    strncpy((char *)q + 80, src && src->Name[0] ? src->Name : "UNNAMED ENTITY", 79);
    LocalHostName(NIPCBase, (STRPTR)q + 160, 128);
    if (RdpSend(f->ResConn, q, sizeof(q)))
        f->State = FINDER_WAITREPLY;
}

void FinderResStatus(struct RdpConn *conn, APTR userdata)
{
    struct Finder *f = userdata;
    struct NIPCBase *NIPCBase = conn->Base;

    if (conn->State == RDP_STATE_OPEN && f->State == FINDER_WAITOPEN)
        SendResolverQuery(NIPCBase, f);
    else if (conn->State == RDP_STATE_CLOSED)
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOLVER);
}

void FinderResDataIn(struct RdpConn *conn, UBYTE *data, ULONG len, APTR userdata)
{
    struct Finder *f = userdata;
    struct NIPCBase *NIPCBase = conn->Base;
    struct Entity *src = f->Req->Entity, *link;
    UWORD port;

    if (f->State != FINDER_WAITREPLY || len < 2)
        return;
    port = nipc_get16(data);
    /* the resolver connection is used for one query only */
    RdpClose(f->ResConn);
    f->ResConn = NULL;
    if (!port)
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_UNKNOWNENTITY);
        return;
    }
    if (!(link = AllocEntity(NIPCBase, f->EntityName, ENTF_LINK)))
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOURCES);
        return;
    }
    if (len > 2)
    {
        ULONG n = len - 2 > NIPC_HOSTSIZE - 1 ? NIPC_HOSTSIZE - 1 : len - 2, i;
        for (i = 0; i < n && data[2 + i]; i++)
            link->HostName[i] = data[2 + i];
        link->HostName[i] = '\0';
    }
    link->Owner = src;
    link->Finder = f;
    f->Link = link;
    ObtainSemaphore(&NIPCBase->Sem);
    src->UseCount++;
    ReleaseSemaphore(&NIPCBase->Sem);
    f->State = FINDER_WAITENTITY;
    if (!(link->Conn = RdpOpenActive(NIPCBase, f->IP, port, ClientDataIn, ClientStatus, link)))
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOURCES);
}

static void FinderConnStatus(struct NIPCBase *NIPCBase, struct Entity *link, struct RdpConn *conn)
{
    struct Finder *f = link->Finder;

    if (conn->State == RDP_STATE_OPEN)
        FinderDone(NIPCBase, f, link, 0);
    else if (conn->State == RDP_STATE_CLOSED)
    {
        NLOG(DEBUG_NAME_STR " FindEntity: entity connection died\n");
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOURCES);
    }
}

/* Dotted quad? */
static BOOL ParseDottedQuad(CONST_STRPTR s, ULONG *ip)
{
    ULONG v = 0;
    int part = 0;

    while (*s == ' ') s++;
    for (;;)
    {
        ULONG n = 0;
        int digits = 0;
        while (*s >= '0' && *s <= '9')
        {
            n = n * 10 + (*s++ - '0');
            if (n > 255) return FALSE;
            digits++;
        }
        if (!digits)
            return FALSE;
        v = (v << 8) | n;
        part++;
        while (*s == ' ') s++;
        if (part == 4)
            break;
        if (*s != '.')
            return FALSE;
        s++;
        while (*s == ' ') s++;
    }
    if (*s)
        return FALSE;
    *ip = v;
    return TRUE;
}

static void StartFinder(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    struct Finder *f;
    char host[NIPC_HOSTSIZE], realm[NIPC_HOSTSIZE];
    CONST_STRPTR colon;
    ULONG ip;

    req->Result = NULL;
    req->Error = ENVOYERR_UNKNOWNHOST;
    if (!req->Host || !req->Host[0] || strlen(req->Host) > 128 || !req->Name || strlen(req->Name) > 79)
    {
        ReplySuperReq(NIPCBase, req);
        return;
    }
    if (!(f = AllocVec(sizeof(struct Finder), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        req->Error = ENVOYERR_NORESOURCES;
        ReplySuperReq(NIPCBase, req);
        return;
    }
    f->Req = req;
    strncpy(f->EntityName, req->Name, 79);
    AddTail((struct List *)&NIPCBase->Finders, (struct Node *)f);

    if (ParseDottedQuad(req->Host, &ip))
    {
        if (NetIsLocalAddress(NIPCBase, ip))
            goto local;
        f->State = FINDER_RESOLVING;
        {
            struct Inquiry q;
            memset(&q, 0, sizeof(q));
            q.DoneData = f;
            FinderResolved(NIPCBase, &q, ip);
        }
        return;
    }

    /* "realm:host" */
    realm[0] = '\0';
    if ((colon = strchr(req->Host, ':')))
    {
        ULONG rl = colon - req->Host;
        if (rl >= sizeof(realm)) rl = sizeof(realm) - 1;
        CopyMem((APTR)req->Host, realm, rl);
        realm[rl] = '\0';
        strncpy(host, colon + 1, sizeof(host) - 1);
    }
    else
        strncpy(host, req->Host, sizeof(host) - 1);
    host[sizeof(host) - 1] = '\0';
    if (!host[0])
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_UNKNOWNHOST);
        return;
    }
    if (!Stricmp(host, NIPCBase->Config.HostName) &&
        (!realm[0] || !Stricmp(realm, NIPCBase->Config.RealmName)))
    {
local:
        {
            struct Entity *e;
            ObtainSemaphore(&NIPCBase->Sem);
            if ((e = FindPublicEntity(NIPCBase, f->EntityName)))
                e->UseCount++;
            ReleaseSemaphore(&NIPCBase->Sem);
            FinderDone(NIPCBase, f, e, ENVOYERR_UNKNOWNENTITY);
        }
        return;
    }
    if (realm[0] && !RealmServerInEffect(NIPCBase))
    {
        FinderDone(NIPCBase, f, NULL, ENVOYERR_UNKNOWNHOST);
        return;
    }
    {
        struct TagItem tags[4];
        int n = 0;
        if (realm[0])
        {
            tags[n].ti_Tag = MATCH_REALM; tags[n].ti_Data = (IPTR)realm; n++;
        }
        else if (RealmServerInEffect(NIPCBase))
        {
            tags[n].ti_Tag = MATCH_REALM; tags[n].ti_Data = (IPTR)NIPCBase->Config.RealmName; n++;
        }
        tags[n].ti_Tag = MATCH_HOSTNAME; tags[n].ti_Data = (IPTR)host; n++;
        tags[n].ti_Tag = QUERY_IPADDR; tags[n].ti_Data = 0; n++;
        tags[n].ti_Tag = TAG_DONE; tags[n].ti_Data = 0;
        f->State = FINDER_RESOLVING;
        f->Inq = InquiryStart(NIPCBase, tags, 5, 1, NULL, req->Caller, NULL);
        if (!f->Inq)
        {
            FinderDone(NIPCBase, f, NULL, ENVOYERR_UNKNOWNHOST);
            return;
        }
        f->Inq->Done = FinderResolved;
        f->Inq->DoneData = f;
    }
}

/* ---- requests from the library functions -------------------------------- */

static void LoseLink(struct NIPCBase *NIPCBase, struct Entity *link)
{
    struct Entity *src = link->Owner;

    if (link->Conn)
    {
        RdpClose(link->Conn);
        link->Conn = NULL;
    }
    FailPending(NIPCBase, link, ENVOYERR_ABORTED);
    CancelPings(NIPCBase, link);
    FreeEntity(NIPCBase, link);
    if (src)
        ReleaseEntity(NIPCBase, src);
}

void LinkHandleReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    struct Entity *link = req->Entity;

    switch (req->Type)
    {
    case SREQ_FINDENTITY:
        StartFinder(NIPCBase, req);
        return;

    case SREQ_LOSEENTITY:
        if (link && (link->Flags & ENTF_LINK) && !(link->Flags & ENTF_SERVERLINK))
            LoseLink(NIPCBase, link);
        break;

    case SREQ_DELETEENTITY:
    {
        /* reset every server link of the entity */
        struct MinNode *n, *next;
        ForeachNodeSafe(&NIPCBase->Entities, n, next)
        {
            struct Entity *l = ENTITY(n);
            if ((l->Flags & ENTF_SERVERLINK) && l->Owner == link)
            {
                if (l->Conn)
                    RdpReset(l->Conn);      /* ServerStatus tears it down when idle */
                else if (l->UseCount == 0)
                    TeardownServerLink(NIPCBase, l);
            }
        }
        break;
    }

    case SREQ_TRANSACT:
    {
        struct Transaction *t = req->Trans;
        if (!link || !(link->Flags & ENTF_LINK) || (link->Flags & ENTF_CONNDEAD) || !link->Conn ||
            link->Conn->State == RDP_STATE_CLOSED)
        {
            ReturnTransaction(NIPCBase, t, ENVOYERR_CANTDELIVER);
            break;
        }
        PRIVTRANS(t)->TxOffset = 0;
        PRIVTRANS(t)->TxFragNo = 0;
        t->trans_Sequence = NIPCBase->Sequence++;
        if (t->trans_Timeout)
            t->trans_Timeout += 2 + RdpEstimateSeconds(link->Conn, 36 + t->trans_ReqDataActual + t->trans_RespDataLength);
        AddTail((struct List *)&link->Outgoing, (struct Node *)&PRIVTRANS(t)->Node);
        PumpLink(NIPCBase, link);
        break;
    }

    case SREQ_REPLY:
    {
        struct Transaction *t = req->Trans;
        if (!link || !(link->Flags & ENTF_SERVERLINK) || (link->Flags & ENTF_CONNDEAD) || !link->Conn ||
            link->Conn->State != RDP_STATE_OPEN)
        {
            FreeTransaction(t);
            if (link && link->UseCount > 0)
                link->UseCount--;
            if (link && (link->Flags & ENTF_CONNDEAD) && link->UseCount == 0)
                TeardownServerLink(NIPCBase, link);
            break;
        }
        PRIVTRANS(t)->TxOffset = 0;
        PRIVTRANS(t)->TxFragNo = 0;
        AddTail((struct List *)&link->Outgoing, (struct Node *)&PRIVTRANS(t)->Node);
        PumpLink(NIPCBase, link);
        break;
    }

    case SREQ_ABORT:
    {
        struct Transaction *t = req->Trans;
        struct MinNode *n;
        BOOL found = FALSE;
        if (link)
        {
            ForeachNode(&link->Pending, n) if (TRANS(n) == t) found = TRUE;
            if (!found)
                ForeachNode(&link->Outgoing, n) if (TRANS(n) == t) found = TRUE;
        }
        if (found)
        {
            Remove((struct Node *)&PRIVTRANS(t)->Node);
            ReturnTransaction(NIPCBase, t, ENVOYERR_ABORTED);
        }
        break;
    }

    case SREQ_PING:
    {
        struct PingReq *p;
        UBYTE ping[6];
        req->Value = req->Value;        /* maxTime in */
        if (!link || !(link->Flags & ENTF_LINK) || !link->Conn || link->Conn->State != RDP_STATE_OPEN ||
            !(p = AllocVec(sizeof(struct PingReq), MEMF_CLEAR | MEMF_PUBLIC)))
        {
            req->Value = 0xFFFFFFFF;
            break;
        }
        p->Req = req;
        p->Link = link;
        p->Cookie = PingCookie++;
        GetNow(NIPCBase, &p->Sent);
        p->Deadline = p->Sent;
        p->Deadline.tv_secs += req->Value / 1000000;
        p->Deadline.tv_micro += req->Value % 1000000;
        if (p->Deadline.tv_micro >= 1000000) { p->Deadline.tv_micro -= 1000000; p->Deadline.tv_secs++; }
        ping[0] = 1;
        ping[1] = 0;
        nipc_put32(ping + 2, p->Cookie);
        if (!RdpSend(link->Conn, ping, 6))
        {
            FreeVec(p);
            req->Value = 0xFFFFFFFF;
            break;
        }
        AddTail((struct List *)&NIPCBase->Pings, (struct Node *)p);
        return;                         /* replied on pong or deadline */
    }
    }
    ReplySuperReq(NIPCBase, req);
}

/* ---- timers ------------------------------------------------------------ */

void LinkTick(struct NIPCBase *NIPCBase)
{
    struct PingReq *p, *next;
    struct timeval now;

    if (IsMinListEmpty(&NIPCBase->Pings))
        return;
    GetNow(NIPCBase, &now);
    ForeachNodeSafe(&NIPCBase->Pings, p, next)
    {
        if (now.tv_secs > p->Deadline.tv_secs || (now.tv_secs == p->Deadline.tv_secs && now.tv_micro >= p->Deadline.tv_micro))
        {
            p->Req->Value = 0xFFFFFFFF;
            Remove((struct Node *)p);
            ReplySuperReq(NIPCBase, p->Req);
            FreeVec(p);
        }
    }
}

void LinkHeartbeat(struct NIPCBase *NIPCBase)
{
    struct MinNode *n, *next, *tn, *tnext;
    struct Finder *f, *fnext;

    /* transaction timeouts on client links; idle server links */
    ForeachNodeSafe(&NIPCBase->Entities, n, next)
    {
        struct Entity *e = ENTITY(n);
        if (!(e->Flags & ENTF_LINK))
            continue;
        if (e->Flags & ENTF_SERVERLINK)
        {
            struct Entity *owner = e->Owner;
            if (owner && owner->TimeoutLinks && e->Conn && e->Conn->State != RDP_STATE_CLOSED)
            {
                if (++e->IdleSeconds > owner->TimeoutLinks)
                {
                    NLOG(DEBUG_NAME_STR " idle server link '%s' reset\n", e->Name);
                    RdpReset(e->Conn);
                }
            }
            continue;
        }
        ForeachNodeSafe(&e->Pending, tn, tnext)
        {
            struct Transaction *t = TRANS(tn);
            if (t->trans_Timeout == 0)
                continue;
            if (--t->trans_Timeout == 0)
            {
                Remove((struct Node *)tn);
                ReturnTransaction(NIPCBase, t, ENVOYERR_TIMEOUT);
            }
        }
    }
    /* resolver countdowns */
    ForeachNodeSafe(&NIPCBase->Finders, f, fnext)
    {
        if ((f->State == FINDER_WAITOPEN || f->State == FINDER_WAITREPLY) && f->Countdown && --f->Countdown == 0)
        {
            NLOG(DEBUG_NAME_STR " FindEntity: resolver timeout\n");
            FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOLVER);
        }
    }
}

void LinkShutdown(struct NIPCBase *NIPCBase)
{
    struct Finder *f;
    struct MinNode *n, *next;

    while ((f = (struct Finder *)GetHead(&NIPCBase->Finders)))
        FinderDone(NIPCBase, f, NULL, ENVOYERR_NORESOURCES);
    CancelPings(NIPCBase, NULL);
    ForeachNodeSafe(&NIPCBase->Entities, n, next)
    {
        struct Entity *e = ENTITY(n);
        if (e->Flags & ENTF_SERVERLINK)
            TeardownServerLink(NIPCBase, e);
        else if (e->Flags & ENTF_LINK)
        {
            if (e->Conn)
            {
                RdpClose(e->Conn);
                e->Conn = NULL;
            }
            LinkConnDied(NIPCBase, e);
        }
    }
}
