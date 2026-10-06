/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - the TCP/IP stack as nipc's network layer: one raw
          socket for RDP (IP protocol 27), one UDP socket on port 376 for
          inquiries, and the stack's interface list.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <libraries/bsdsocket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>

#include "nipc_intern.h"

#define SocketBase              (NIPCBase->SocketBase)

/* Not every SDK exports a stub for this one; call the vector directly */
static long QueryIface(struct NIPCBase *NIPCBase, STRPTR name, struct TagItem *tags)
{
    return AROS_LC2(long, QueryInterfaceTagList, AROS_LCA(STRPTR, name, A0), AROS_LCA(struct TagItem *, tags, A1),
                    struct Library *, SocketBase, 78, BSDSocket);
}
#define RXBUFSIZE               65536

static UBYTE *RxBuf;

static ULONG SockAddrIP(struct sockaddr *sa)
{
    return sa->sa_family == AF_INET ? ntohl(((struct sockaddr_in *)sa)->sin_addr.s_addr) : 0;
}

BOOL NetOpen(struct NIPCBase *NIPCBase)
{
    struct sockaddr_in sin;
    LONG on = 1, size = 262144;

    NIPCBase->RawSock = NIPCBase->UdpSock = -1;
    if (!(SocketBase = OpenLibrary("bsdsocket.library", 4)))
    {
        NLOG(DEBUG_NAME_STR " no bsdsocket.library: network disabled\n");
        return FALSE;
    }
    if (!RxBuf && !(RxBuf = AllocVec(RXBUFSIZE, MEMF_PUBLIC)))
        return FALSE;

    NIPCBase->RawSock = socket(AF_INET, SOCK_RAW, NIPC_IPPROTO_RDP);
    if (NIPCBase->RawSock >= 0)
        setsockopt(NIPCBase->RawSock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    else
        NLOG(DEBUG_NAME_STR " raw socket failed, errno %ld\n", (long)Errno());

    NIPCBase->UdpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (NIPCBase->UdpSock >= 0)
    {
        setsockopt(NIPCBase->UdpSock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        setsockopt(NIPCBase->UdpSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_len = sizeof(sin);
        sin.sin_port = htons(NIPC_INQUIRY_PORT);
        sin.sin_addr.s_addr = INADDR_ANY;
        if (bind(NIPCBase->UdpSock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
            NLOG(DEBUG_NAME_STR " bind *:376 failed, errno %ld\n", (long)Errno());
    }
    return TRUE;
}

void NetClose(struct NIPCBase *NIPCBase)
{
    struct Iface *ifc;

    if (SocketBase)
    {
        if (NIPCBase->RawSock >= 0)
            CloseSocket(NIPCBase->RawSock);
        if (NIPCBase->UdpSock >= 0)
            CloseSocket(NIPCBase->UdpSock);
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
    NIPCBase->RawSock = NIPCBase->UdpSock = -1;
    while ((ifc = (struct Iface *)RemHead((struct List *)&NIPCBase->Ifaces)))
        FreeVec(ifc);
    if (RxBuf)
    {
        FreeVec(RxBuf);
        RxBuf = NULL;
    }
}

/* Read the stack's interface list */
void NetRefreshIfaces(struct NIPCBase *NIPCBase)
{
    struct ifconf ifc;
    struct ifreq *ifr, req;
    struct Iface *ifa;
    char buf[4096];
    LONG n;

    while ((ifa = (struct Iface *)RemHead((struct List *)&NIPCBase->Ifaces)))
        FreeVec(ifa);
    if (!SocketBase || NIPCBase->UdpSock < 0)
        return;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (IoctlSocket(NIPCBase->UdpSock, SIOCGIFCONF, (char *)&ifc) < 0)
        return;

    for (n = 0; n < ifc.ifc_len; )
    {
        ULONG salen;
        ifr = (struct ifreq *)(buf + n);
        salen = ifr->ifr_addr.sa_len > sizeof(struct sockaddr) ? ifr->ifr_addr.sa_len : sizeof(struct sockaddr);
        n += sizeof(ifr->ifr_name) + salen;
        if (ifr->ifr_addr.sa_family != AF_INET)
            continue;
        /* one entry per name */
        {
            BOOL seen = FALSE;
            ForeachNode(&NIPCBase->Ifaces, ifa)
                if (!strncmp(ifa->Name, ifr->ifr_name, sizeof(ifa->Name) - 1)) seen = TRUE;
            if (seen)
                continue;
        }
        if (!(ifa = AllocVec(sizeof(struct Iface), MEMF_CLEAR | MEMF_PUBLIC)))
            break;
        strncpy(ifa->Name, ifr->ifr_name, sizeof(ifa->Name) - 1);
        ifa->Address = SockAddrIP(&ifr->ifr_addr);

        memset(&req, 0, sizeof(req));
        strncpy(req.ifr_name, ifa->Name, sizeof(req.ifr_name) - 1);
        if (IoctlSocket(NIPCBase->UdpSock, SIOCGIFFLAGS, (char *)&req) == 0)
        {
            if (!(req.ifr_flags & IFF_UP))
            {
                FreeVec(ifa);
                continue;
            }
            ifa->Loopback = (req.ifr_flags & IFF_LOOPBACK) ? TRUE : FALSE;
        }
        if (IoctlSocket(NIPCBase->UdpSock, SIOCGIFNETMASK, (char *)&req) == 0)
            ifa->Netmask = SockAddrIP(&req.ifr_addr);
        if (!ifa->Netmask)
            ifa->Netmask = (ifa->Address >> 31) == 0 ? 0xFF000000 : (ifa->Address >> 30) == 2 ? 0xFFFF0000 : 0xFFFFFF00;
        if (IoctlSocket(NIPCBase->UdpSock, SIOCGIFBRDADDR, (char *)&req) == 0)
            ifa->Broadcast = SockAddrIP(&req.ifr_broadaddr);
        if (!ifa->Broadcast)
            ifa->Broadcast = ifa->Address | ~ifa->Netmask;
        if (IoctlSocket(NIPCBase->UdpSock, SIOCGIFMTU, (char *)&req) == 0)
            ifa->Mtu = req.ifr_mtu;
        if (!ifa->Mtu)
            ifa->Mtu = 1500;
        {
            ULONG bps = 0;
            struct TagItem tags[] = { { IFQ_BPS, (IPTR)&bps }, { TAG_DONE, 0 } };
            QueryIface(NIPCBase, ifa->Name, tags);
            ifa->Bps = bps ? bps : 10000000;
        }
        if (ifa->Loopback)
        {
            ifa->Mtu = 0x7FFFFFFF;
            ifa->Bps = 0x7FFFFFFF;
        }
        AddTail((struct List *)&NIPCBase->Ifaces, (struct Node *)ifa);
        NLOG(DEBUG_NAME_STR " interface %s %08lx/%08lx bcast %08lx mtu %lu bps %lu\n", ifa->Name,
              (unsigned long)ifa->Address, (unsigned long)ifa->Netmask, (unsigned long)ifa->Broadcast,
              (unsigned long)ifa->Mtu, (unsigned long)ifa->Bps);
    }
}

/* The interface a datagram to dest leaves through */
struct Iface *NetIfaceFor(struct NIPCBase *NIPCBase, ULONG dest)
{
    struct Iface *ifa, *first = NULL;

    ForeachNode(&NIPCBase->Ifaces, ifa)
    {
        if ((dest >> 24) == 127 && ifa->Loopback)
            return ifa;
        if (ifa->Address && dest == ifa->Address)
            return ifa;
    }
    ForeachNode(&NIPCBase->Ifaces, ifa)
    {
        if (ifa->Loopback || !ifa->Address)
            continue;
        if (!first)
            first = ifa;
        if ((dest & ifa->Netmask) == (ifa->Address & ifa->Netmask))
            return ifa;
    }
    return first;       /* default route: the first real interface */
}

BOOL NetIsLocalAddress(struct NIPCBase *NIPCBase, ULONG addr)
{
    struct Iface *ifa;

    if ((addr >> 24) == 127)
        return TRUE;
    ForeachNode(&NIPCBase->Ifaces, ifa)
        if (ifa->Address == addr)
            return TRUE;
    return FALSE;
}

static BOOL SendTo(struct NIPCBase *NIPCBase, LONG sock, ULONG dest, UWORD port, const UBYTE *data, ULONG len)
{
    struct sockaddr_in sin;
    LONG r;

    if (!SocketBase || sock < 0)
        return FALSE;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(sin);
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(dest);
    r = sendto(sock, (APTR)data, len, 0, (struct sockaddr *)&sin, sizeof(sin));
    if (r < 0)
        NLOG(DEBUG_NAME_STR " sendto %08lx:%d failed, errno %ld\n", (unsigned long)dest, port, (long)Errno());
    return r >= 0;
}

BOOL NetSendRdp(struct NIPCBase *NIPCBase, ULONG dest, const UBYTE *seg, ULONG len)
{
    return SendTo(NIPCBase, NIPCBase->RawSock, dest, 0, seg, len);
}

/* dest 0xFFFFFFFF = limited broadcast: one directed broadcast per interface
 * (the stack has no default route to send 255.255.255.255 through, and real
 * Envoy accepts directed broadcasts) */
BOOL NetSendUdp(struct NIPCBase *NIPCBase, ULONG dest, const UBYTE *data, ULONG len)
{
    if (dest == 0xFFFFFFFF)
    {
        struct Iface *ifa;
        BOOL any = FALSE;
        ForeachNode(&NIPCBase->Ifaces, ifa)
        {
            if (ifa->Loopback || !ifa->Address)
                continue;
            if (SendTo(NIPCBase, NIPCBase->UdpSock, ifa->Broadcast, NIPC_INQUIRY_PORT, data, len))
                any = TRUE;
        }
        return any;
    }
    return SendTo(NIPCBase, NIPCBase->UdpSock, dest, NIPC_INQUIRY_PORT, data, len);
}

/* Wait for socket input, a signal, or the time limit; process what arrived */
void NetPoll(struct NIPCBase *NIPCBase, ULONG waitus, ULONG *sigmask)
{
    fd_set rfds;
    struct timeval tv;
    LONG nfds = 0, r;

    tv.tv_secs = waitus / 1000000;
    tv.tv_micro = waitus % 1000000;

    if (!SocketBase)
    {
        /* no stack: wait for signals only */
        ULONG got = *sigmask;
        if (waitus == 0)
            got = SetSignal(0, 0) & *sigmask;
        else
        {
            /* a timer-less wait: use Delay() for the tick */
            ULONG ticks = (waitus + 19999) / 20000;
            got = SetSignal(0, 0) & *sigmask;
            if (!got)
            {
                Delay(ticks ? ticks : 1);
                got = SetSignal(0, 0) & *sigmask;
            }
        }
        *sigmask = got;
        return;
    }

    FD_ZERO(&rfds);
    if (NIPCBase->RawSock >= 0) { FD_SET(NIPCBase->RawSock, &rfds); if (NIPCBase->RawSock >= nfds) nfds = NIPCBase->RawSock + 1; }
    if (NIPCBase->UdpSock >= 0) { FD_SET(NIPCBase->UdpSock, &rfds); if (NIPCBase->UdpSock >= nfds) nfds = NIPCBase->UdpSock + 1; }

    r = WaitSelect(nfds, &rfds, NULL, NULL, &tv, sigmask);
    if (r <= 0)
        return;

    if (NIPCBase->RawSock >= 0 && FD_ISSET(NIPCBase->RawSock, &rfds))
    {
        struct sockaddr_in from;
        LONG fl = sizeof(from), n;
        n = recvfrom(NIPCBase->RawSock, RxBuf, RXBUFSIZE, 0, (struct sockaddr *)&from, &fl);
        if (n > 20)
        {
            ULONG hl = (RxBuf[0] & 15) * 4;
            if (hl >= 20 && (ULONG)n > hl)
                RdpInput(NIPCBase, ntohl(from.sin_addr.s_addr), nipc_get32(RxBuf + 16), RxBuf + hl, n - hl);
        }
    }
    if (NIPCBase->UdpSock >= 0 && FD_ISSET(NIPCBase->UdpSock, &rfds))
    {
        struct sockaddr_in from;
        LONG fl = sizeof(from), n;
        n = recvfrom(NIPCBase->UdpSock, RxBuf, RXBUFSIZE, 0, (struct sockaddr *)&from, &fl);
        if (n > 0)
            InquiryInput(NIPCBase, ntohl(from.sin_addr.s_addr), RxBuf, n);
    }
}
