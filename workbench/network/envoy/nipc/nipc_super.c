/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - the supervisor process ("NIPC"). Owns the sockets,
          runs RDP, the resolver, the inquiry responder and the timers, and
          serves the requests of the library functions.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <dos/dostags.h>
#include <dos/notify.h>
#include <string.h>

#include "nipc_intern.h"

#define TimerBase               ((struct Device *)NIPCBase->nipc_TimerBase)

void GetNow(struct NIPCBase *NIPCBase, struct timeval *tv)
{
    if (NIPCBase->nipc_TimerBase)
        GetSysTime(tv);
    else
    {
        tv->tv_secs = 0;
        tv->tv_micro = 0;
    }
}

ULONG TimevalDiffUs(const struct timeval *a, const struct timeval *b)
{
    LONG s = (LONG)(a->tv_secs - b->tv_secs);
    LONG us = (LONG)a->tv_micro - (LONG)b->tv_micro;
    if (s < 0 || (s == 0 && us < 0))
        return 0;
    return (ULONG)s * 1000000UL + us;
}

void ReplySuperReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    if (req->Msg.mn_ReplyPort)
        ReplyMsg(&req->Msg);
    else
        FreeVec(req);
}

static void SetDefaults(struct NIPCBase *NIPCBase)
{
    NIPCBase->DefaultTTL = 30;
    NIPCBase->DefaultTOS = 0;
    NIPCBase->FragmentTO = 30;
    NIPCBase->ArpResolveTO = 3;
    NIPCBase->ArpResolveRetries = 15;
    NIPCBase->ArpEntryTO = 60;
    NIPCBase->Rdp.InactivityCheck = 0;
    NIPCBase->Rdp.InactivityLimit = 60;
    NIPCBase->Rdp.TransmitRetries = 5;
    NIPCBase->Rdp.TransmitMinTO = 2;
    NIPCBase->Rdp.TransmitMaxTO = 10;
    NIPCBase->Rdp.InitialRoundTripTO = 1;
    NIPCBase->Rdp.ConnectTO = 10;
    NIPCBase->Rdp.ConnectRetries = 3;
}

static BOOL OpenTimer(struct NIPCBase *NIPCBase)
{
    if (!(NIPCBase->TimerPort = CreateMsgPort()))
        return FALSE;
    if (!(NIPCBase->TimerReq = (struct timerequest *)CreateIORequest(NIPCBase->TimerPort, sizeof(struct timerequest))))
        return FALSE;
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)NIPCBase->TimerReq, 0))
    {
        DeleteIORequest((struct IORequest *)NIPCBase->TimerReq);
        NIPCBase->TimerReq = NULL;
        return FALSE;
    }
    NIPCBase->nipc_TimerBase = (struct Library *)NIPCBase->TimerReq->tr_node.io_Device;
    return TRUE;
}

static void CloseTimer(struct NIPCBase *NIPCBase)
{
    if (NIPCBase->TimerReq)
    {
        CloseDevice((struct IORequest *)NIPCBase->TimerReq);
        DeleteIORequest((struct IORequest *)NIPCBase->TimerReq);
        NIPCBase->TimerReq = NULL;
    }
    NIPCBase->nipc_TimerBase = NULL;
    if (NIPCBase->TimerPort)
    {
        DeleteMsgPort(NIPCBase->TimerPort);
        NIPCBase->TimerPort = NULL;
    }
}

static void StartPrefsNotify(struct NIPCBase *NIPCBase)
{
    struct NotifyRequest *nr;
    LONG bit;

    if ((bit = AllocSignal(-1)) < 0)
        return;
    if (!(nr = AllocVec(sizeof(struct NotifyRequest), MEMF_CLEAR | MEMF_PUBLIC)))
    {
        FreeSignal(bit);
        return;
    }
    nr->nr_Name = "ENV:Envoy/nipc.prefs";
    nr->nr_Flags = NRF_SEND_SIGNAL;
    nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
    nr->nr_stuff.nr_Signal.nr_SignalNum = bit;
    if (StartNotify(nr))
    {
        NIPCBase->PrefsNotify = nr;
        NIPCBase->NotifySigBit = bit;
    }
    else
    {
        FreeVec(nr);
        FreeSignal(bit);
    }
}

static void EndPrefsNotify(struct NIPCBase *NIPCBase)
{
    if (NIPCBase->PrefsNotify)
    {
        EndNotify(NIPCBase->PrefsNotify);
        FreeVec(NIPCBase->PrefsNotify);
        FreeSignal(NIPCBase->NotifySigBit);
        NIPCBase->PrefsNotify = NULL;
    }
}

static void HandleReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    switch (req->Type)
    {
    case SREQ_FINDENTITY:
    case SREQ_LOSEENTITY:
    case SREQ_DELETEENTITY:
    case SREQ_TRANSACT:
    case SREQ_REPLY:
    case SREQ_ABORT:
    case SREQ_PING:
        LinkHandleReq(NIPCBase, req);
        break;

    case SREQ_INQUIRY:
    case SREQ_INQUIRYABORT:
        InquiryHandleReq(NIPCBase, req);
        break;

    case SREQ_GETMTU:
    {
        struct Iface *ifc = NetIfaceFor(NIPCBase, req->Value);
        req->Error = ifc ? ifc->Mtu : 0;
        ReplySuperReq(NIPCBase, req);
        break;
    }

    case SREQ_RECONFIG:
        LoadConfig(NIPCBase);
        NetRefreshIfaces(NIPCBase);
        ReplySuperReq(NIPCBase, req);
        break;

    default:
        ReplySuperReq(NIPCBase, req);
        break;
    }
}

static BOOL HaveUsableIface(struct NIPCBase *NIPCBase)
{
    struct Iface *ifa;

    ForeachNode(&NIPCBase->Ifaces, ifa)
    {
        if (!ifa->Loopback && ifa->Address)
            return TRUE;
    }
    return FALSE;
}

static void Tick(struct NIPCBase *NIPCBase, const struct timeval *now)
{
    NIPCBase->Ticks++;
    EventTick(NIPCBase, now);
    InquiryTick(NIPCBase);
    LinkTick(NIPCBase);
    if (NIPCBase->Ticks % NIPC_TICKS_PER_HB == 0)
    {
        RdpHeartbeat(NIPCBase);
        LinkHeartbeat(NIPCBase);
        InquiryHeartbeat(NIPCBase);
        /* every 30 s; every second during the first two minutes while the stack is still
           coming up (no sockets, no resolver or no configured interface yet) */
        if (NIPCBase->Ticks % (NIPC_TICKS_PER_HB * 30) == 0 ||
            (NIPCBase->Ticks < NIPC_TICKS_PER_HB * 120 &&
             (!HaveUsableIface(NIPCBase) || NIPCBase->RawSock < 0 || NIPCBase->UdpSock < 0 || !NIPCBase->Resolver)))
        {
            if (!NIPCBase->SocketBase || NIPCBase->RawSock < 0 || NIPCBase->UdpSock < 0)
                NetOpen(NIPCBase);          /* opened before the stack was ready */
            if (!NIPCBase->Resolver)
                ResolverStart(NIPCBase);    /* needs the stack; retried until it is there */
            NetRefreshIfaces(NIPCBase);
            RefreshStackHostName(NIPCBase);
        }
    }
}

void SuperProcess(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct NIPCBase *NIPCBase = (struct NIPCBase *)me->pr_Task.tc_UserData;
    struct Message *startup;
    struct SuperReq *req, *quit = NULL;
    struct timeval now;
    BOOL ok = FALSE;

    WaitPort(&me->pr_MsgPort);
    startup = GetMsg(&me->pr_MsgPort);

    me->pr_WindowPtr = (APTR)-1;        /* no requesters on our behalf */
    SetDefaults(NIPCBase);
    NIPCBase->Ticks = 0;

    if (OpenTimer(NIPCBase) && (NIPCBase->SuperPort = CreateMsgPort()))
    {
        GetNow(NIPCBase, &NIPCBase->LastTick);
        NetOpen(NIPCBase);                  /* without the stack we still serve local entities */
        LoadConfig(NIPCBase);
        NetRefreshIfaces(NIPCBase);
        ResolverStart(NIPCBase);
        StartPrefsNotify(NIPCBase);
        ok = TRUE;
    }
    NIPCBase->SuperOK = ok;
    ReplyMsg(startup);
    if (!ok)
    {
        if (NIPCBase->SuperPort)
        {
            DeleteMsgPort(NIPCBase->SuperPort);
            NIPCBase->SuperPort = NULL;
        }
        CloseTimer(NIPCBase);
        Forbid();
        NIPCBase->Super = NULL;
        return;
    }

    NLOG(DEBUG_NAME_STR " supervisor running as '%s'\n", NIPCBase->Config.HostName);

    while (!quit)
    {
        ULONG sigmask = (1UL << NIPCBase->SuperPort->mp_SigBit);
        ULONG elapsed;

        if (NIPCBase->PrefsNotify)
            sigmask |= (1UL << NIPCBase->NotifySigBit);

        /* socket input, with the tick as time limit */
        GetNow(NIPCBase, &now);
        elapsed = TimevalDiffUs(&now, &NIPCBase->LastTick);
        NetPoll(NIPCBase, elapsed >= NIPC_TICK_US ? 0 : NIPC_TICK_US - elapsed, &sigmask);

        if (NIPCBase->PrefsNotify && (sigmask & (1UL << NIPCBase->NotifySigBit)))
        {
            LoadConfig(NIPCBase);
            NetRefreshIfaces(NIPCBase);
        }

        while ((req = (struct SuperReq *)GetMsg(NIPCBase->SuperPort)))
        {
            if (req->Type == SREQ_QUIT)
            {
                quit = req;
                break;
            }
            HandleReq(NIPCBase, req);
        }
        LinkPumpAll(NIPCBase);

        GetNow(NIPCBase, &now);
        if (TimevalDiffUs(&now, &NIPCBase->LastTick) >= NIPC_TICK_US)
        {
            NIPCBase->LastTick = now;
            Tick(NIPCBase, &now);
        }
    }

    /* shut-down */
    EndPrefsNotify(NIPCBase);
    LinkShutdown(NIPCBase);
    ResolverStop(NIPCBase);
    InquiryShutdown(NIPCBase);
    {
        struct RdpConn *c;
        while ((c = (struct RdpConn *)GetHead(&NIPCBase->Conns)))
            RdpClose(c);
    }
    NetClose(NIPCBase);
    FreeRealms(NIPCBase);

    Forbid();
    {
        struct MsgPort *port = NIPCBase->SuperPort;
        NIPCBase->SuperPort = NULL;
        Permit();
        /* bounce late requests */
        while ((req = (struct SuperReq *)GetMsg(port)))
            ReplySuperReq(NIPCBase, req);
        DeleteMsgPort(port);
    }
    CloseTimer(NIPCBase);
    ReplySuperReq(NIPCBase, quit);

    Forbid();
    NIPCBase->Super = NULL;
    if (NIPCBase->Closer)
        Signal(NIPCBase->Closer, SIGF_SINGLE);
    /* Forbid() is released when the process ends */
}
