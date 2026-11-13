/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - RDP (IP protocol 27) as Envoy speaks it:
          re/spec/nipc-rdp.md. RFC 908/1151 version 2 segments, initial
          sequence number 1, no EACK, single-RST close, per-segment
          retransmission timers driven by the one-second heartbeat.
*/

#include <proto/exec.h>
#include <string.h>

#include "nipc_intern.h"

#define SEQ_AFTER(a, b)         (((ULONG)((a) - (b))) < 1024)        /* a is at/after b */
#define CONN(n)                 ((struct RdpConn *)(n))

static void FinishDeferredClose(struct RdpConn *conn);

/* ---- helpers ------------------------------------------------------------ */

UWORD RdpAllocPort(struct NIPCBase *NIPCBase)
{
    UWORD p = NIPCBase->NextPort++;
    if (NIPCBase->NextPort < NIPC_FIRST_DYNPORT)
        NIPCBase->NextPort = NIPC_FIRST_DYNPORT;
    return p;
}

static UWORD LocMaxForBps(ULONG bps)
{
    if (bps <= 60000)    return 3;
    if (bps <= 100000)   return 4;
    if (bps <= 500000)   return 5;
    if (bps <= 2000000)  return 6;
    if (bps <= 10000000) return 8;
    return 32;
}

static void SetState(struct RdpConn *conn, UWORD state)
{
    struct NIPCBase *NIPCBase = conn->Base;

    if (conn->State == state)
        return;
    NLOG(DEBUG_NAME_STR " rdp %p %d:%d state %d -> %d\n", conn, conn->LocalPort, conn->RemotePort, conn->State, state);
    conn->State = state;
    if (conn->Status)
    {
        conn->InCallback = TRUE;
        conn->Status(conn, conn->UserData);
        conn->InCallback = FALSE;
        FinishDeferredClose(conn);
    }
}

static void LearnIface(struct RdpConn *conn, ULONG peer)
{
    struct Iface *ifa = NetIfaceFor(conn->Base, peer);

    if (ifa)
    {
        if (!conn->LocalIP)
            conn->LocalIP = ifa->Address;
        conn->Mtu = ifa->Mtu;
        conn->Bps = ifa->Bps;
    }
    else
    {
        conn->Mtu = 1500;
        conn->Bps = 10000000;
    }
    conn->LocMax = LocMaxForBps(conn->Bps);
    conn->LocMss = (conn->Mtu > 38 + 64 && conn->Mtu < 65536) ? conn->Mtu - 38 : 8192;
}

static void Negotiate(struct RdpConn *conn, const UBYTE *seg)
{
    UWORD peermax = nipc_get16(seg + 18);
    UWORD peermss = nipc_get16(seg + 20);

    conn->SndLimit = peermax < conn->LocMax ? peermax : conn->LocMax;
    conn->Mss = peermss < conn->LocMss ? peermss : conn->LocMss;
}

/* Build and send a control segment (§4.2) */
static void SendCtl(struct RdpConn *conn, UBYTE flags, ULONG seq, ULONG ack)
{
    struct NIPCBase *NIPCBase = conn->Base;
    UBYTE seg[RDP_SYNHDRLEN];
    ULONG len = (flags & RDP_FLAG_SYN) ? RDP_SYNHDRLEN : RDP_HDRLEN;

    memset(seg, 0, sizeof(seg));
    seg[0] = flags | RDP_VERSION;
    seg[1] = len / 2;
    nipc_put16(seg + 2, conn->LocalPort);
    nipc_put16(seg + 4, conn->RemotePort);
    nipc_put16(seg + 6, 0);
    nipc_put32(seg + 8, seq);
    nipc_put32(seg + 12, ack);
    if (flags & RDP_FLAG_SYN)
    {
        nipc_put16(seg + 18, conn->LocMax);
        nipc_put16(seg + 20, conn->LocMss);
        nipc_put16(seg + 22, 0);
    }
    nipc_put16(seg + 16, ~InetChecksum(seg, len, 0));
    if (flags & RDP_FLAG_ACK)
    {
        conn->RcvAcked = conn->RcvCur;
        conn->AckPending = FALSE;
    }
    conn->LastCtlFlags = flags;
    conn->ConnCountdown = (conn->Rto >> 10) + NIPCBase->Rdp.ConnectTO;
    conn->IdleCount = 0;
    NetSendRdp(NIPCBase, conn->RemoteIP, seg, len);
}

/* Control segments carry SND.NXT-1 (NUL: SND.NXT) and RCV.CUR */
static void SendFlags(struct RdpConn *conn, UBYTE flags)
{
    SendCtl(conn, flags, (flags & RDP_FLAG_NUL) ? conn->SndNxt : conn->SndNxt - 1, conn->RcvCur);
}

/* RST for a segment that matched no connection (§4.7) */
static void ForceReset(struct NIPCBase *NIPCBase, ULONG srcip, const UBYTE *in, UBYTE flags)
{
    UBYTE seg[RDP_HDRLEN];
    ULONG inseq = nipc_get32(in + 8), inack = nipc_get32(in + 12);

    if (flags & RDP_FLAG_RST)
        return;
    memset(seg, 0, sizeof(seg));
    seg[1] = RDP_HDRLEN / 2;
    nipc_put16(seg + 2, nipc_get16(in + 4));
    nipc_put16(seg + 4, nipc_get16(in + 2));
    if (flags & (RDP_FLAG_ACK | RDP_FLAG_NUL))
    {
        seg[0] = RDP_FLAG_RST | RDP_VERSION;
        nipc_put32(seg + 8, inack);
    }
    else
    {
        seg[0] = RDP_FLAG_RST | RDP_FLAG_ACK | RDP_VERSION;
        nipc_put32(seg + 8, 0xFFFFFFFF);
    }
    nipc_put32(seg + 12, inseq);
    nipc_put16(seg + 16, ~InetChecksum(seg, RDP_HDRLEN, 0));
    NetSendRdp(NIPCBase, srcip, seg, RDP_HDRLEN);
}

static void FreeSegs(struct MinList *list)
{
    struct RdpSeg *s;
    while ((s = (struct RdpSeg *)RemHead((struct List *)list)))
        FreeVec(s);
}

static struct RdpConn *NewConn(struct NIPCBase *NIPCBase, RdpDataFunc datain, RdpStatusFunc status, APTR userdata)
{
    struct RdpConn *conn;

    if (!(conn = AllocVec(sizeof(struct RdpConn), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    conn->Base = NIPCBase;
    conn->DataIn = datain;
    conn->Status = status;
    conn->UserData = userdata;
    conn->SndNxt = 2;               /* ISS is 1; SYN carries 1, first data 2 */
    conn->SndUna = 2;
    conn->Rto = NIPCBase->Rdp.InitialRoundTripTO << 10;
    conn->LocMax = 1;
    conn->LocMss = 8192;
    conn->Mss = 8192;
    conn->SndLimit = 1;
    NEWLIST(&conn->TxQueue);
    NEWLIST(&conn->RxQueue);
    AddHead((struct List *)&NIPCBase->Conns, (struct Node *)conn);
    return conn;
}

/* ---- opening and closing ---------------------------------------------- */

struct RdpConn *RdpOpenActive(struct NIPCBase *NIPCBase, ULONG ip, UWORD port, RdpDataFunc datain, RdpStatusFunc status, APTR userdata)
{
    struct RdpConn *conn;

    if (!NIPCBase->SocketBase || NIPCBase->RawSock < 0)
        return NULL;
    if (!(conn = NewConn(NIPCBase, datain, status, userdata)))
        return NULL;
    conn->RemoteIP = ip;
    conn->RemotePort = port;
    conn->LocalPort = RdpAllocPort(NIPCBase);
    LearnIface(conn, ip);
    conn->State = RDP_STATE_SYNSENT;        /* no callback for the initial state */
    SendCtl(conn, RDP_FLAG_SYN, 1, 0);
    return conn;
}

struct RdpConn *RdpOpenPassive(struct NIPCBase *NIPCBase, UWORD port, BOOL clone, RdpDataFunc datain, RdpStatusFunc status, APTR userdata)
{
    struct RdpConn *conn;

    if (!(conn = NewConn(NIPCBase, datain, status, userdata)))
        return NULL;
    conn->Passive = TRUE;
    conn->Clone = clone;
    conn->LocalPort = port ? port : RdpAllocPort(NIPCBase);
    conn->State = RDP_STATE_LISTEN;
    return conn;
}

/* A reset must not overtake an acknowledgement the peer is still waiting for */
static void FlushAck(struct RdpConn *conn)
{
    if (conn->AckPending && conn->State == RDP_STATE_OPEN)
        SendFlags(conn, RDP_FLAG_ACK);
}

void RdpReset(struct RdpConn *conn)
{
    if (conn->State != RDP_STATE_LISTEN && conn->State != RDP_STATE_CLOSED)
    {
        FlushAck(conn);
        SendFlags(conn, RDP_FLAG_RST);
        SetState(conn, RDP_STATE_CLOSED);
    }
}

static void FreeConn(struct RdpConn *conn)
{
    Remove((struct Node *)conn);
    FreeSegs(&conn->TxQueue);
    FreeSegs(&conn->RxQueue);
    FreeVec(conn);
}

void RdpClose(struct RdpConn *conn)
{
    if (!conn)
        return;
    conn->DataIn = NULL;
    conn->Status = NULL;            /* the owner is going away: no more callbacks */
    if (conn->InCallback)
    {
        /* from inside a data or status callback: the segment being processed
         * is acknowledged first, then the connection is reset and freed */
        conn->CloseDeferred = TRUE;
        return;
    }
    if (conn->State != RDP_STATE_CLOSED && conn->State != RDP_STATE_LISTEN)
    {
        FlushAck(conn);
        SendFlags(conn, RDP_FLAG_RST);
        conn->State = RDP_STATE_CLOSED;
    }
    FreeConn(conn);
}

/* After a callback has returned: a close requested inside it */
static void FinishDeferredClose(struct RdpConn *conn)
{
    if (conn->CloseDeferred && !conn->InCallback)
    {
        conn->CloseDeferred = FALSE;
        RdpClose(conn);
    }
}

BOOL RdpWindowFull(struct RdpConn *conn)
{
    return conn->State == RDP_STATE_OPEN && conn->Out >= conn->SndLimit;
}

ULONG RdpEstimateSeconds(struct RdpConn *conn, ULONG bytes)
{
    ULONG bps10 = conn->Bps / 10;
    return (conn->Rto >> 10) + 1 + (bps10 ? bytes / bps10 : 0);
}

/* ---- sending data ------------------------------------------------------ */

static void ArmSeg(struct RdpConn *conn, struct RdpSeg *s)
{
    struct NIPCBase *NIPCBase = conn->Base;
    ULONG xmit = conn->Bps ? conn->Mtu / (conn->Bps / 10 ? conn->Bps / 10 : 1) : 1;
    ULONG cd;

    if (xmit < 1)
        xmit = 1;
    cd = (conn->Rto >> 10) + xmit;
    if (cd < NIPCBase->Rdp.TransmitMinTO)
        cd = NIPCBase->Rdp.TransmitMinTO;
    s->Countdown = cd;
}

BOOL RdpSend(struct RdpConn *conn, const UBYTE *data, ULONG len)
{
    struct NIPCBase *NIPCBase = conn->Base;
    struct RdpSeg *s;
    UBYTE *seg;

    if (conn->State != RDP_STATE_OPEN)
        return FALSE;
    if (!(s = AllocVec(sizeof(struct RdpSeg) + RDP_HDRLEN + len, MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    seg = s->Data;
    seg[0] = (conn->AckPending ? RDP_FLAG_ACK : 0) | RDP_VERSION;
    seg[1] = RDP_HDRLEN / 2;
    nipc_put16(seg + 2, conn->LocalPort);
    nipc_put16(seg + 4, conn->RemotePort);
    nipc_put16(seg + 6, len);
    nipc_put32(seg + 8, conn->SndNxt);
    nipc_put32(seg + 12, conn->RcvCur);
    CopyMem((APTR)data, seg + RDP_HDRLEN, len);
    nipc_put16(seg + 16, ~InetChecksum(seg, RDP_HDRLEN + len, 0));
    if (conn->AckPending)
    {
        conn->RcvAcked = conn->RcvCur;
        conn->AckPending = FALSE;
    }
    s->Seq = conn->SndNxt++;
    s->Len = RDP_HDRLEN + len;
    s->RtoBase = conn->Rto;
    GetNow(NIPCBase, &s->SentAt);
    ArmSeg(conn, s);
    AddTail((struct List *)&conn->TxQueue, (struct Node *)s);
    conn->Out++;
    conn->IdleCount = 0;
    NetSendRdp(NIPCBase, conn->RemoteIP, seg, s->Len);
    return TRUE;
}

/* ---- receiving --------------------------------------------------------- */

static void RttSample(struct RdpConn *conn, ULONG m)
{
    LONG err;

    if (conn->Sa == 0 && conn->Sv == 0)
        conn->Sa = 8 * m;
    err = (LONG)m - (LONG)(conn->Sa >> 3);
    conn->Sa = (ULONG)((LONG)conn->Sa + err);
    conn->Sv = (ULONG)((LONG)conn->Sv + (err < 0 ? -err : err) - (LONG)(conn->Sv >> 2));
    conn->Rto = ((conn->Sa >> 2) + conn->Sv) >> 1;
    if (conn->Rto == 0)
        conn->Rto = 1;
}

/* Cumulative acknowledgement (§5.4) */
static void ProcessAck(struct RdpConn *conn, ULONG ack)
{
    ULONG a = ack + 1;
    struct RdpSeg *s;
    BOOL released = FALSE, clean = TRUE;
    struct timeval now, oldest;
    BOOL haveoldest = FALSE;

    if (a == conn->SndUna || !SEQ_AFTER(a, conn->SndUna))
        return;
    conn->SndUna = a;
    while ((s = (struct RdpSeg *)GetHead(&conn->TxQueue)) && !SEQ_AFTER(s->Seq, a))
    {
        if (!haveoldest)
        {
            oldest = s->SentAt;
            haveoldest = TRUE;
        }
        if (s->Retransmitted)
            clean = FALSE;
        Remove((struct Node *)s);
        FreeVec(s);
        if (conn->Out)
            conn->Out--;
        released = TRUE;
    }
    if (released && clean && haveoldest)
    {
        GetNow(conn->Base, &now);
        RttSample(conn, (TimevalDiffUs(&now, &oldest) * 1024UL) / 1000000UL);
    }
}

static void DecideAck(struct RdpConn *conn)
{
    ULONG threshold = conn->LocMax / 2;

    if (threshold < 1)
        threshold = 1;
    if ((ULONG)(conn->RcvCur - conn->RcvAcked) >= threshold)
        SendFlags(conn, RDP_FLAG_ACK);
    else
        conn->AckPending = TRUE;
}

/* Data path in OPEN (§5.3): order, deliver, acknowledge */
static void ProcessData(struct RdpConn *conn, ULONG seq, const UBYTE *data, ULONG len)
{
    struct NIPCBase *NIPCBase = conn->Base;
    ULONG expect = conn->RcvDlv + 1;
    struct RdpSeg *q, *next;

    if (conn->CloseDeferred)
        return;

    if (seq == expect)
    {
        /* in order: deliver now, then whatever follows directly from the queue */
        conn->RcvCur = seq;
        conn->RcvDlv = seq;
        if (conn->DataIn && len)
        {
            conn->InCallback = TRUE;
            conn->DataIn(conn, (UBYTE *)data, len, conn->UserData);
            conn->InCallback = FALSE;
        }
        while (conn->State == RDP_STATE_OPEN && (q = (struct RdpSeg *)GetHead(&conn->RxQueue)) && q->Seq == conn->RcvDlv + 1)
        {
            Remove((struct Node *)q);
            conn->RcvCur = conn->RcvDlv = q->Seq;
            if (conn->DataIn && q->Len)
            {
                conn->InCallback = TRUE;
                conn->DataIn(conn, q->Data, q->Len, conn->UserData);
                conn->InCallback = FALSE;
            }
            FreeVec(q);
        }
    }
    else if (!SEQ_AFTER(seq, expect))
    {
        NLOG(DEBUG_NAME_STR " rdp: discarded old segment %lu (expect %lu)\n", (unsigned long)seq, (unsigned long)expect);
    }
    else
    {
        /* ahead: queue in order unless already there */
        struct RdpSeg *s;
        BOOL dup = FALSE;
        ForeachNode(&conn->RxQueue, q)
            if (q->Seq == seq) dup = TRUE;
        if (!dup && (s = AllocVec(sizeof(struct RdpSeg) + len, MEMF_PUBLIC)))
        {
            s->Seq = seq;
            s->Len = len;
            CopyMem((APTR)data, s->Data, len);
            ForeachNodeSafe(&conn->RxQueue, q, next)
            {
                if (SEQ_AFTER(q->Seq, seq) && q->Seq != seq)
                {
                    Insert((struct List *)&conn->RxQueue, (struct Node *)s, ((struct Node *)q)->ln_Pred);
                    s = NULL;
                    break;
                }
            }
            if (s)
                AddTail((struct List *)&conn->RxQueue, (struct Node *)s);
        }
    }
    if (conn->State == RDP_STATE_OPEN || conn->State == RDP_STATE_CLOSED)
        DecideAck(conn);
    FinishDeferredClose(conn);
}

static void Handshake(struct RdpConn *conn, const UBYTE *seg, ULONG dstip)
{
    /* common initialisation on a SYN */
    if (!conn->LocalIP)
        conn->LocalIP = dstip;
    conn->RcvCur = conn->RcvDlv = conn->RcvAcked = nipc_get32(seg + 8);
    Negotiate(conn, seg);
}

static void StateMachine(struct RdpConn *conn, ULONG srcip, ULONG dstip, UBYTE *seg, ULONG len)
{
    struct NIPCBase *NIPCBase = conn->Base;
    UBYTE flags = seg[0] & 0xF8;
    ULONG hlen = seg[1] * 2;
    ULONG seq = nipc_get32(seg + 8), ack = nipc_get32(seg + 12);
    BOOL hasdata = len > hlen;

    conn->IdleCount = 0;

    switch (conn->State)
    {
    case RDP_STATE_LISTEN:
        if (flags & RDP_FLAG_RST)
            return;
        if (flags & (RDP_FLAG_ACK | RDP_FLAG_NUL))
        {
            SendCtl(conn, RDP_FLAG_RST, ack + 1, conn->RcvCur);
            return;
        }
        if (flags & RDP_FLAG_SYN)
        {
            conn->RemoteIP = srcip;
            conn->RemotePort = nipc_get16(seg + 2);
            LearnIface(conn, srcip);
            Handshake(conn, seg, dstip);
            SendCtl(conn, RDP_FLAG_SYN | RDP_FLAG_ACK, 1, conn->RcvCur);
            SetState(conn, RDP_STATE_SYNRCVD);
        }
        return;

    case RDP_STATE_SYNSENT:
        if (flags & RDP_FLAG_RST)
        {
            if (flags & RDP_FLAG_ACK)
                SendCtl(conn, RDP_FLAG_RST, 1, ack + 1);
            return;
        }
        if ((flags & (RDP_FLAG_SYN | RDP_FLAG_ACK)) == (RDP_FLAG_SYN | RDP_FLAG_ACK))
        {
            Handshake(conn, seg, dstip);
            conn->SndUna = ack + 1;
            SendCtl(conn, RDP_FLAG_ACK, 1, conn->RcvCur);
            SetState(conn, RDP_STATE_OPEN);
            return;
        }
        if (flags & RDP_FLAG_SYN)
        {
            Handshake(conn, seg, dstip);
            SendCtl(conn, RDP_FLAG_SYN | RDP_FLAG_ACK, 1, conn->RcvCur);
            SetState(conn, RDP_STATE_SYNRCVD);
            return;
        }
        if (flags & RDP_FLAG_ACK)
        {
            if (ack != 1)
            {
                SendFlags(conn, RDP_FLAG_RST);
                SetState(conn, RDP_STATE_CLOSED);
            }
        }
        return;

    case RDP_STATE_SYNRCVD:
        if (flags & RDP_FLAG_RST)
        {
            SetState(conn, conn->Passive && !conn->Clone ? RDP_STATE_LISTEN : RDP_STATE_CLOSED);
            return;
        }
        {
            ULONG window = conn->LocMax * 2 < 1024 ? conn->LocMax * 2 : 1024;
            if ((ULONG)(seq - conn->RcvCur) >= window)
            {
                SendFlags(conn, RDP_FLAG_ACK);
                return;
            }
        }
        if (flags & RDP_FLAG_SYN)
        {
            SendFlags(conn, RDP_FLAG_RST);
            SetState(conn, RDP_STATE_CLOSED);
            return;
        }
        if (!(flags & RDP_FLAG_ACK))
            return;
        if (ack != 1)
        {
            SendFlags(conn, RDP_FLAG_RST);
            SetState(conn, RDP_STATE_CLOSED);
            return;
        }
        SetState(conn, RDP_STATE_OPEN);
        if (hasdata && conn->State == RDP_STATE_OPEN)
            ProcessData(conn, seq, seg + hlen, len - hlen);
        return;

    case RDP_STATE_OPEN:
        if (flags == 0)
        {
            if (hasdata)
                ProcessData(conn, seq, seg + hlen, len - hlen);
            return;
        }
        if (flags & RDP_FLAG_RST)
        {
            SetState(conn, RDP_STATE_CLOSED);
            return;
        }
        if (flags & RDP_FLAG_NUL)
        {
            /* answer with our true RCV.CUR (see the spec's NUL caveat) */
            SendFlags(conn, RDP_FLAG_ACK);
            return;
        }
        if (flags & RDP_FLAG_SYN)
        {
            SendFlags(conn, RDP_FLAG_RST);
            SetState(conn, RDP_STATE_CLOSED);
            return;
        }
        if (flags & RDP_FLAG_ACK)
            ProcessAck(conn, ack);
        if (hasdata && conn->State == RDP_STATE_OPEN)
            ProcessData(conn, seq, seg + hlen, len - hlen);
        return;

    default:
        return;
    }
}

void RdpInput(struct NIPCBase *NIPCBase, ULONG srcip, ULONG dstip, UBYTE *seg, ULONG len)
{
    struct RdpConn *conn, *found = NULL, *next;
    UBYTE flags;
    UWORD sport, dport;

    if (len < RDP_HDRLEN || (seg[0] & 3) != RDP_VERSION || InetChecksum(seg, len, 0) != 0xFFFF)
    {
        NLOG(DEBUG_NAME_STR " bad RDP segment from %08lx (%lu bytes)\n", (unsigned long)srcip, (unsigned long)len);
        return;
    }
    flags = seg[0] & 0xF8;
    sport = nipc_get16(seg + 2);
    dport = nipc_get16(seg + 4);
    if ((flags & RDP_FLAG_SYN) && len < RDP_SYNHDRLEN)
        return;

    /* 1. an existing connection */
    ForeachNode(&NIPCBase->Conns, conn)
    {
        if (conn->State != RDP_STATE_CLOSED && conn->State != RDP_STATE_LISTEN &&
            conn->LocalPort == dport && conn->RemotePort == sport && conn->RemoteIP == srcip)
        {
            found = conn;
            break;
        }
    }
    if (found)
    {
        Remove((struct Node *)found);
        AddHead((struct List *)&NIPCBase->Conns, (struct Node *)found);
        StateMachine(found, srcip, dstip, seg, len);
        return;
    }

    if (flags & RDP_FLAG_SYN)
    {
        /* 2a. a host that restarted: purge its older passive connections */
        if (sport >= NIPC_FIRST_DYNPORT)
        {
            ForeachNodeSafe(&NIPCBase->Conns, conn, next)
            {
                if (conn->Passive && conn->State == RDP_STATE_OPEN && conn->RemoteIP == srcip &&
                    conn->RemotePort >= NIPC_FIRST_DYNPORT &&
                    (UWORD)(conn->RemotePort - sport) < 0x8000)
                {
                    NLOG(DEBUG_NAME_STR " rdp: killing old connection %p (port %d <= %d)\n", conn, conn->RemotePort, sport);
                    RdpReset(conn);
                }
            }
        }
        /* 2b. a listener */
        ForeachNode(&NIPCBase->Conns, conn)
        {
            if (conn->State == RDP_STATE_LISTEN && conn->LocalPort == dport)
            {
                struct RdpConn *target = conn;
                if (conn->Clone)
                {
                    if (!(target = NewConn(NIPCBase, conn->DataIn, conn->Status, conn->UserData)))
                        break;
                    target->Passive = TRUE;
                    target->LocalPort = conn->LocalPort;
                    target->State = RDP_STATE_LISTEN;
                }
                StateMachine(target, srcip, dstip, seg, len);
                return;
            }
        }
    }
    /* 3. nobody wants it */
    ForceReset(NIPCBase, srcip, seg, flags);
}

/* ---- timers ------------------------------------------------------------ */

static void RetransmitTimers(struct RdpConn *conn)
{
    struct NIPCBase *NIPCBase = conn->Base;
    struct RdpSeg *s, *next;

    ForeachNodeSafe(&conn->TxQueue, s, next)
    {
        if (s->Countdown > 0 && --s->Countdown > 0)
            continue;
        s->Retries++;
        if (s->Retries > NIPCBase->Rdp.TransmitRetries)
        {
            NLOG(DEBUG_NAME_STR " rdp %p: segment %lu lost after %d tries, resetting\n", conn, (unsigned long)s->Seq, s->Retries);
            RdpReset(conn);
            return;
        }
        if (!SEQ_AFTER(s->Seq, conn->SndUna))
        {
            Remove((struct Node *)s);
            FreeVec(s);
            if (conn->Out) conn->Out--;
            continue;
        }
        /* resend the identical segment, back off */
        s->Retransmitted = TRUE;
        NetSendRdp(NIPCBase, conn->RemoteIP, s->Data, s->Len);
        conn->IdleCount = 0;
        {
            ULONG backoff = s->RtoBase << s->Retries;
            ULONG cap = NIPCBase->Rdp.TransmitMaxTO << 10;
            if (backoff > cap || (s->RtoBase << s->Retries) < s->RtoBase)
                backoff = cap;
            if (backoff > conn->Rto)
                conn->Rto = backoff;
        }
        ArmSeg(conn, s);
    }
}

void RdpHeartbeat(struct NIPCBase *NIPCBase)
{
    struct RdpConn *conn, *next;

    ForeachNodeSafe(&NIPCBase->Conns, conn, next)
    {
        if (conn->FreeDeferred && !conn->InCallback)
        {
            FreeConn(conn);
            continue;
        }
        if (conn->State == RDP_STATE_CLOSED)
            continue;

        /* inactivity probing (passive side only, off by default) */
        if (NIPCBase->Rdp.InactivityCheck && conn->IdleCount < NIPCBase->Rdp.InactivityLimit && conn->IdleCount < 255)
        {
            conn->IdleCount++;
            if (conn->Passive && conn->IdleCount == NIPCBase->Rdp.InactivityCheck && conn->State == RDP_STATE_OPEN && conn->Out == 0)
                SendCtl(conn, RDP_FLAG_NUL, conn->SndNxt - 1, conn->RcvCur);
        }

        switch (conn->State)
        {
        case RDP_STATE_OPEN:
            if (conn->AckPending)
                SendFlags(conn, RDP_FLAG_ACK);
            RetransmitTimers(conn);
            break;

        case RDP_STATE_SYNSENT:
        case RDP_STATE_SYNRCVD:
            if (conn->ConnCountdown > 0 && --conn->ConnCountdown > 0)
                break;
            conn->ConnRetries++;
            if (conn->ConnRetries >= NIPCBase->Rdp.ConnectRetries)
            {
                NLOG(DEBUG_NAME_STR " rdp %p: handshake gave up\n", conn);
                RdpReset(conn);
            }
            else
            {
                /* repeat the last control segment */
                UBYTE f = conn->LastCtlFlags;
                SendCtl(conn, f, (f & RDP_FLAG_SYN) ? 1 : conn->SndNxt - 1, (f & RDP_FLAG_SYN) && !(f & RDP_FLAG_ACK) ? 0 : conn->RcvCur);
            }
            break;

        default:
            break;
        }
    }
}
