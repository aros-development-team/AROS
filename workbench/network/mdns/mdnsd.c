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
 * programs on this host register through bonami.library (RFC 6763). The
 * library reaches it through the public message port MDNS_IPC_PORT; no
 * client shares memory with it.
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
    mdnsDiscoveryReset(md);
    mdnsQueryReset(md);
    mdnsRegGoodbyeAll(md);
    mdnsHostGoodbye(md);
    mdnsNetClose(md);
    if (md->md_SocketBase != NULL) {
        CloseLibrary(md->md_SocketBase);
        md->md_SocketBase = NULL;
    }
}

static void
net_up(struct MDNSDaemon *md)
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
            TAG_END);
    }
    /* An interface came, went or changed its address: the name is asked
     * for again with the addresses as they are now. */
    if (mdnsNetRescan(md)) {
        mdnsDiscoveryReset(md);
        mdnsQueryReset(md);
        mdnsHostRestart(md);
        mdnsRegRestartAll(md);
    }
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
    LONG i, top = -1;

    if (SocketBase == NULL) {
        /* Between "reconfigure begin" and "end" there is no stack to ask. */
        return Wait(sigmask);
    }

    FD_ZERO(&readfds);
    tv.tv_sec = (probing || md->md_AnswerPending) ? 0 : 1;
    tv.tv_usec = md->md_AnswerPending ? 20000 : probing ? 250000 : 0;
    for (i = 0; i < md->md_IfCount; i++) {
        LONG sock = md->md_Ifs[i].mi_Socket;

        if (sock >= 0) {
            FD_SET(sock, &readfds);
            if (sock > top) {
                top = sock;
            }
        }
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

    for (i = 0; i < md->md_IfCount; i++) {
        LONG sock = md->md_Ifs[i].mi_Socket;

        if (sock >= 0 && FD_ISSET(sock, &readfds)) {
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            int got = recvfrom(sock, (char *)md->md_RxBuf, MDNS_MAX_PACKET,
                0, (struct sockaddr *)&from, &fromlen);

            if (got > 0) {
                md->md_RxPackets++;
                md->md_SendIf = i;
                md->md_SendFamily = AF_INET;
                mdnsHandlePacket(md, md->md_RxBuf, (LONG)got,
                    (ULONG)ntohl((unsigned long)from.sin_addr.s_addr),
                    (UWORD)ntohs(from.sin_port));
                md->md_SendIf = -1;
                md->md_SendFamily = 0;
            }
        }
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
        int got;

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

        for (i = 0; havePacketInfo && i < md->md_IfCount; i++) {
            if (packetInfo.ipi6_ifindex == md->md_Ifs[i].mi_Index) {
                break;
            }
        }
        if (got > 0 && havePacketInfo && i < md->md_IfCount) {
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
    struct MDNSIPCMsg *msg;
    struct MsgPort *port;
    ULONG portmask;
    ULONG lastScan = 0, lastReap = 0;
    BOOL running = TRUE, probing = FALSE;

    /* One responder owns the host's names. */
    Forbid();
    port = FindPort((STRPTR)MDNS_IPC_PORT);
    Permit();
    if (port != NULL) {
        PutStr("mdnsd: already running\n");
        return RETURN_WARN;
    }

    md->md_Socket = -1;
    md->md_Socket6 = -1;
    md->md_SendIf = -1;
    md->md_NextHandle = 1;
    md->md_DaemonID = GetETaskID(FindTask(NULL));
    NEWLIST(&md->md_Services);
    md->md_RxBuf = (UBYTE *)AllocMem(MDNS_MAX_PACKET, MEMF_CLEAR);
    md->md_TxBuf = (UBYTE *)AllocMem(MDNS_MAX_PACKET, MEMF_CLEAR);
    port = CreateMsgPort();
    if (md->md_RxBuf == NULL || md->md_TxBuf == NULL || port == NULL) {
        if (md->md_RxBuf != NULL) {
            FreeMem(md->md_RxBuf, MDNS_MAX_PACKET);
        }
        if (md->md_TxBuf != NULL) {
            FreeMem(md->md_TxBuf, MDNS_MAX_PACKET);
        }
        if (port != NULL) {
            DeleteMsgPort(port);
        }
        return RETURN_FAIL;
    }
    md->md_Port = port;
    portmask = 1UL << port->mp_SigBit;

    /* Without the service registry, for example when started by hand, it
     * still runs and Ctrl-C stops it. */
    mdnsdServiceOpen(&sig);
    net_up(md);
    port->mp_Node.ln_Name = (STRPTR)MDNS_IPC_PORT;
    AddPort(port);

    while (running) {
        ULONG got = wait_and_receive(md,
            portmask | sig.ms_Stop | sig.ms_Begin | sig.ms_End |
            SIGBREAKF_CTRL_C, probing);

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
        if ((got & sig.ms_End) ||
            mdnsNow() - lastScan >= MDNSD_RESCAN_SECONDS) {
            lastScan = mdnsNow();
            if (md->md_SocketBase != NULL || (got & sig.ms_End)) {
                net_up(md);
            }
        }

        while ((msg = (struct MDNSIPCMsg *)GetMsg(port)) != NULL) {
            mdnsClientMessage(md, msg);
            ReplyMsg(&msg->mi_Msg);
        }
        if (running && mdnsNow() - lastReap >= MDNSD_REAP_SECONDS) {
            lastReap = mdnsNow();
            mdnsClientReap(md);
            mdnsDiscoveryReap(md);
        }

        mdnsDiscoveryTick(md);
        mdnsQueryTick(md);
        probing = FALSE;
        if (running && md->md_Socket >= 0) {
            /* the host's name first: no service is announced before it */
            probing = mdnsHostTick(md);
            if (mdnsRegTick(md)) {
                probing = TRUE;
            }
        }
    }

    /* Every lent request for events goes back while the port is still
     * public: a client that cannot find the port any more never waits for
     * one. Messages still queued are not acted on, so none is lent again. */
    mdnsDiscoveryCancel(md);
    mdnsQueryCancel(md);

    /* No client may find the port any more; whoever already sent is told
     * that mdnsd is gone. */
    RemPort(port);
    while ((msg = (struct MDNSIPCMsg *)GetMsg(port)) != NULL) {
        msg->mi_Result = MDNSERR_NOENGINE;
        ReplyMsg(&msg->mi_Msg);
    }
    DeleteMsgPort(port);
    md->md_Port = NULL;

    net_down(md);
    mdnsRegFree(md);
    mdnsDiscoveryFree(md);
    mdnsQueryFree(md);
    mdnsdServiceClose();
    FreeMem(md->md_RxBuf, MDNS_MAX_PACKET);
    FreeMem(md->md_TxBuf, MDNS_MAX_PACKET);
    return RETURN_OK;
}
