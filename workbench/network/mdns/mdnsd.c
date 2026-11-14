/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdnsd - the host's multicast DNS responder (RFC 6762).
 *
 * It answers for the host's name in the mDNS domain, "<hostname>.local", on
 * every interface that can multicast, so that other machines on the link
 * find this one by name without a DNS server, and for the services that
 * programs on this host register through dnssd.library (RFC 6763), whose
 * browses, resolves, lookups and record questions it answers from its
 * cache. The library reaches it through the public message port
 * DNSSD_IPC_PORT; no client shares memory with it.
 *
 * The AROSTCP service manager starts it from db/services.d/mdns and stops
 * it with the stack. Started by hand it runs until Ctrl-C.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <string.h>
#include <sys/socket.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <libraries/bsdsocket.h>
#include <netinet/in.h>

#include <dnssd/dnssd.h>

#include "mdnsd.h"

/* Seconds between looks at the stack's interfaces. */
#define MDNSD_RESCAN_SECONDS 3
/* Seconds between looks for clients that ended without closing the
 * library. */
#define MDNSD_REAP_SECONDS 5

static LONG daemonErrno;

ULONG
mdnsNow(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)ds.ds_Days * 86400UL + (ULONG)ds.ds_Minute * 60UL +
        (ULONG)(ds.ds_Tick / 50);
}

/* Fiftieths of a second; only differences mean anything. */
ULONG
mdnsTicks(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)ds.ds_Days * 4320000UL + (ULONG)ds.ds_Minute * 3000UL +
        (ULONG)ds.ds_Tick;
}

/* The stack's base and the sockets belong to this task and go away
 * together: around a reconfiguration, and for good at the end. */
static void
net_down(struct MDNSDaemon *md)
{
    struct Library *SocketBase = md->md_SocketBase;

    mdnsRegGoodbyeAll(md);
    mdnsHostGoodbye(md);
    mdnsNetClose(md);
    if (SocketBase != NULL) {
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0,
            TAG_END);
        CloseLibrary(SocketBase);
        md->md_SocketBase = NULL;
    }
}

static void
net_up(struct MDNSDaemon *md, ULONG addressMask)
{
    struct Library *SocketBase;

    if (md->md_SocketBase == NULL) {
        SocketBase = OpenLibrary("bsdsocket.library", 4);
        if (SocketBase == NULL) {
            return;
        }
        md->md_SocketBase = SocketBase;
        SocketBaseTags(
            SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(daemonErrno))),
            (IPTR)&daemonErrno,
            SBTM_SETREF(SBTC_LOGTAGPTR), (IPTR)"mdnsd",
            SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), addressMask,
            TAG_END);
    }
    /* An interface came, went or changed its address: the name is asked
     * for again with the addresses as they are now. */
    if (mdnsNetRescan(md)) {
        mdnsCacheLinksChanged(md);
        mdnsHostRestart(md);
        mdnsRegRestartAll(md);
    }
}

/* The slot of the link with this interface index, or -1. */
static LONG
link_slot(const struct MDNSDaemon *md, ULONG index)
{
    LONG i;

    for (i = 0; index != 0 && i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Index == index) {
            return i;
        }
    }
    return -1;
}

/* A datagram on the IPv4 socket: IP_PKTINFO names the link it came by, and
 * one of a link mdnsd does not serve is dropped. */
static void
receive4(struct MDNSDaemon *md, struct Library *SocketBase)
{
    union {
        struct cmsghdr align;
        UBYTE data[CMSG_SPACE(sizeof(struct in_pktinfo))];
    } control;
    struct sockaddr_in from;
    struct msghdr message;
    struct iovec vector;
    struct cmsghdr *header;
    LONG slot = -1;
    int got;

    memset(&message, 0, sizeof(message));
    memset(&from, 0, sizeof(from));
    vector.iov_base = md->md_RxBuf;
    vector.iov_len = MDNS_MAX_PACKET;
    message.msg_name = &from;
    message.msg_namelen = sizeof(from);
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
    message.msg_control = &control;
    message.msg_controllen = sizeof(control);
    got = recvmsg((int)md->md_Socket4, &message, 0);
    if (got <= 0) {
        return;
    }
    for (header = CMSG_FIRSTHDR(&message); header != NULL;
            header = CMSG_NXTHDR(&message, header)) {
        if (header->cmsg_level == IPPROTO_IP &&
            header->cmsg_type == IP_PKTINFO &&
            header->cmsg_len == CMSG_LEN(sizeof(struct in_pktinfo))) {
            struct in_pktinfo info;

            memcpy(&info, CMSG_DATA(header), sizeof(info));
            slot = link_slot(md, (ULONG)info.ipi_ifindex);
            break;
        }
    }
    if (slot < 0) {
        return;
    }
    md->md_RxPackets++;
    md->md_SendIf = slot;
    md->md_SendFamily = AF_INET;
    mdnsHandlePacket(md, md->md_RxBuf, (LONG)got,
        (ULONG)ntohl((unsigned long)from.sin_addr.s_addr),
        (UWORD)ntohs(from.sin_port));
    md->md_SendIf = -1;
    md->md_SendFamily = 0;
}

/* Waits for a signal, a datagram on any interface, or a second to pass: a
 * quarter of one while a name is being probed, and a fiftieth while an
 * answer is waiting to be sent. */
static ULONG
wait_and_receive(struct MDNSDaemon *md, ULONG sigmask, BOOL probing)
{
    struct Library *SocketBase = md->md_SocketBase;
    struct timeval tv;
    fd_set readfds;
    LONG top = -1;

    if (SocketBase == NULL) {
        /* Between "reconfigure begin" and "end" there is no stack to ask. */
        return Wait(sigmask);
    }

    FD_ZERO(&readfds);
    tv.tv_sec = (probing || md->md_AnswerPending) ? 0 : 1;
    tv.tv_usec = md->md_AnswerPending ? 20000 : probing ? 250000 : 0;
    if (md->md_Socket4 >= 0) {
        FD_SET(md->md_Socket4, &readfds);
        top = md->md_Socket4;
    }
    if (md->md_Socket6 >= 0) {
        FD_SET(md->md_Socket6, &readfds);
        if (md->md_Socket6 > top) {
            top = md->md_Socket6;
        }
    }
    if (WaitSelect(top + 1, top >= 0 ? &readfds : NULL, NULL, NULL, &tv,
            &sigmask) <= 0) {
        return sigmask;
    }

    if (md->md_Socket4 >= 0 && FD_ISSET(md->md_Socket4, &readfds)) {
        receive4(md, SocketBase);
    }
    if (md->md_Socket6 >= 0 && FD_ISSET(md->md_Socket6, &readfds)) {
        union {
            struct cmsghdr align;
            UBYTE data[CMSG_SPACE(sizeof(struct in6_pktinfo))];
        } control;
        struct sockaddr_in6 from6;
        struct in6_pktinfo packetInfo;
        struct msghdr message;
        struct iovec vector;
        BOOL havePacketInfo = FALSE;
        LONG i;
        int got;

        memset(&packetInfo, 0, sizeof(packetInfo));
        memset(&message, 0, sizeof(message));
        memset(&from6, 0, sizeof(from6));
        vector.iov_base = md->md_RxBuf;
        vector.iov_len = MDNS_MAX_PACKET;
        message.msg_name = &from6;
        message.msg_namelen = sizeof(from6);
        message.msg_iov = &vector;
        message.msg_iovlen = 1;
        message.msg_control = &control;
        message.msg_controllen = sizeof(control);
        got = recvmsg((int)md->md_Socket6, &message, 0);

        if (got > 0) {
            struct cmsghdr *header;

            for (header = CMSG_FIRSTHDR(&message); header != NULL;
                    header = CMSG_NXTHDR(&message, header)) {
                if (header->cmsg_level == IPPROTO_IPV6 &&
                    header->cmsg_type == IPV6_PKTINFO &&
                    header->cmsg_len >=
                        CMSG_LEN(sizeof(struct in6_pktinfo))) {
                    memcpy(&packetInfo, CMSG_DATA(header),
                        sizeof(packetInfo));
                    havePacketInfo = TRUE;
                    break;
                }
            }
        }

        i = havePacketInfo ? link_slot(md, packetInfo.ipi6_ifindex) : -1;
        if (got > 0 && i >= 0) {
            md->md_RxPackets++;
            /* What this packet causes to be sent goes back to its link
             * and, for unicast, its address family. */
            md->md_SendIf = i;
            md->md_SendFamily = AF_INET6;
            mdnsHandlePacket6(md, md->md_RxBuf, (LONG)got,
                &from6.sin6_addr, (UWORD)ntohs(from6.sin6_port));
            md->md_SendIf = -1;
            md->md_SendFamily = 0;
        }
    }
    return sigmask;
}

int
main(void)
{
    static struct MDNSDaemon daemon;
    struct MDNSDaemon *md = &daemon;
    struct MDNSDSignals sig;
    ULONG dportmask;
    struct DNSSDIPCMsg *dmsg;
    struct MsgPort *dport;
    ULONG lastScan = 0, lastReap = 0;
    ULONG addressMask = 0, goneMask = 0;
    BYTE addressSignal = -1, goneSignal = -1;
    BOOL running = TRUE, probing = FALSE;

    /* One responder owns the host's names. */
    Forbid();
    dport = FindPort((STRPTR)DNSSD_IPC_PORT);
    Permit();
    if (dport != NULL) {
        PutStr("mdnsd: already running\n");
        return RETURN_WARN;
    }

    md->md_Socket = -1;
    md->md_Socket4 = -1;
    md->md_Socket6 = -1;
    md->md_SendIf = -1;
    md->md_NextHandle = 1;
    md->md_DaemonID = GetETaskID(FindTask(NULL));
    if (dnssd_link_inventory_init(&md->md_LinkInventory,
            md->md_Links, md->md_LinkScratch, MDNS_IFS_MAX,
            md->md_Addresses, md->md_AddressScratch,
            MDNS_ADDRESS_MAX) != DNSSD_LINK_OK) {
        return RETURN_FAIL;
    }
    NEWLIST(&md->md_Services);
    md->md_RxBuf = (UBYTE *)AllocMem(MDNS_MAX_PACKET, MEMF_CLEAR);
    md->md_TxBuf = (UBYTE *)AllocMem(MDNS_MAX_PACKET, MEMF_CLEAR);
    dport = CreateMsgPort();
    if (md->md_RxBuf == NULL || md->md_TxBuf == NULL ||
        dport == NULL || !mdnsCacheInit(md)) {
        if (md->md_RxBuf != NULL) {
            FreeMem(md->md_RxBuf, MDNS_MAX_PACKET);
        }
        if (md->md_TxBuf != NULL) {
            FreeMem(md->md_TxBuf, MDNS_MAX_PACKET);
        }
        if (dport != NULL) {
            DeleteMsgPort(dport);
        }
        return RETURN_FAIL;
    }
    md->md_DNSSDPort = dport;
    dportmask = 1UL << dport->mp_SigBit;

    /* Without the service registry, for example when started by hand, it
     * still runs and Ctrl-C stops it. */
    mdnsdServiceOpen(&sig);
    addressSignal = AllocSignal(-1);
    if (addressSignal >= 0) {
        addressMask = 1UL << addressSignal;
    }
    net_up(md, addressMask);
    /* dnssd.library's sessions, and the signal that a client task went */
    goneSignal = AllocSignal(-1);
    if (goneSignal >= 0) {
        goneMask = 1UL << goneSignal;
    }
    if (!dnssdServerInit(md, goneMask)) {
        PutStr("mdnsd: no memory for dnssd.library's sessions\n");
    }
    /* dnssd.library's port */
    dport->mp_Node.ln_Name = (STRPTR)DNSSD_IPC_PORT;
    AddPort(dport);

    while (running) {
        ULONG got = wait_and_receive(md,
            dportmask | sig.ms_Stop | sig.ms_Begin | sig.ms_End |
            addressMask | goneMask | SIGBREAKF_CTRL_C, probing);

        if (got & (sig.ms_Stop | SIGBREAKF_CTRL_C)) {
            running = FALSE;
        }
        /*
         * The stack signals the start and the end of a reconfiguration, and
         * may well have finished before this task runs: then both signals
         * are here at once, and the sockets are closed and opened again in
         * one go. Between a start and an end that come apart the base is
         * closed, which keeps the periodic look from opening anything.
         */
        if (got & sig.ms_Begin) {
            net_down(md);
        }
        if ((got & (sig.ms_End | addressMask)) ||
            mdnsNow() - lastScan >= MDNSD_RESCAN_SECONDS) {
            lastScan = mdnsNow();
            if (md->md_SocketBase != NULL || (got & sig.ms_End)) {
                net_up(md, addressMask);
            }
        }

        if (got & goneMask) {
            dnssdServerGone(md);        /* before a message of a new task */
        }
        while ((dmsg = (struct DNSSDIPCMsg *)GetMsg(dport)) != NULL) {
            dnssdServerMessage(md, dmsg);
            ReplyMsg(&dmsg->dm_Msg);
        }
        if (running && mdnsNow() - lastReap >= MDNSD_REAP_SECONDS) {
            lastReap = mdnsNow();
            dnssdServerReap(md);
        }

        /* the cache first: what it drops is told to the operations that
         * watch it, which then look again in their own turn */
        mdnsCacheTick(md);
        dnssdServerTick(md);
        probing = FALSE;
        if (running && md->md_Socket >= 0) {
            /* the host's name first: no service is announced before it */
            probing = mdnsHostTick(md);
            if (mdnsRegTick(md)) {
                probing = TRUE;
            }
        }
    }

    /* Every lent notification goes back while the port is still public: a
     * client that cannot find the port any more never waits for one.
     * Messages still queued are not acted on, so none is lent again. */
    dnssdServerCancel(md);

    /* No client may find the port any more; whoever already sent is told
     * that mdnsd is gone. */
    RemPort(dport);
    while ((dmsg = (struct DNSSDIPCMsg *)GetMsg(dport)) != NULL) {
        if (DNSSD_IPC_REQUEST_ROOM(dmsg->dm_Msg.mn_Length) >= sizeof(dmsg->dm_Request)) {
            dmsg->dm_Request.result = DNSSD_ERR_NOENGINE;
        }
        ReplyMsg(&dmsg->dm_Msg);
    }
    DeleteMsgPort(dport);
    md->md_DNSSDPort = NULL;

    net_down(md);
    mdnsRegFree(md);
    dnssdServerFree(md);
    mdnsCacheFree(md);
    mdnsdServiceClose();
    if (addressSignal >= 0) {
        FreeSignal(addressSignal);
    }
    if (goneSignal >= 0) {
        FreeSignal(goneSignal);         /* the hook is off (dnssdServerFree()) */
    }
    FreeMem(md->md_RxBuf, MDNS_MAX_PACKET);
    FreeMem(md->md_TxBuf, MDNS_MAX_PACKET);
    return RETURN_OK;
}
