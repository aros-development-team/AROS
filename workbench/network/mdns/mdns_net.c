/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_net.c - IPv4/IPv6 UDP 5353 sockets for each interface
 *
 * mDNS is a protocol of the link: what is heard on an interface is answered
 * there, and an address record carries the address of the interface it is
 * sent on. Each interface that is up, can multicast and has an IPv4 address
 * and/or an IPv6 link-local address gets an IPv4 socket bound to it; one
 * IPv6 socket joins every link and obtains the input link from IPV6_PKTINFO.
 */

#include <exec/types.h>
#include <string.h>
#include <sys/socket.h>

#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include "mdnsd.h"

struct found
{
    char    name[IFNAMSIZ + 1];
    ULONG   index;
    ULONG   addr;               /* 0xAABBCCDD == A.B.C.D */
    struct in6_addr addr6;
};

static struct found *
found_interface(struct found *out, LONG *count, LONG max, const char *name)
{
    LONG i;

    for (i = 0; i < *count; i++) {
        if (strncmp(out[i].name, name, IFNAMSIZ) == 0) {
            return &out[i];
        }
    }
    if (*count >= max) {
        return NULL;
    }
    memset(&out[*count], 0, sizeof(out[*count]));
    memcpy(out[*count].name, name, IFNAMSIZ);
    return &out[(*count)++];
}

static BOOL
usable_interface(struct Library *SocketBase, int sock, const char *name,
    ULONG *index)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, name, IFNAMSIZ);
    if (IoctlSocket(sock, SIOCGIFFLAGS, (char *)&ifr) != 0 ||
        (ifr.ifr_flags & (IFF_UP | IFF_MULTICAST | IFF_LOOPBACK)) !=
            (IFF_UP | IFF_MULTICAST)) {
        return FALSE;
    }
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, name, IFNAMSIZ);
    if (IoctlSocket(sock, SIOCGIFINDEX, (char *)&ifr) != 0 ||
        ifr.ifr_index <= 0) {
        return FALSE;
    }
    *index = (ULONG)ifr.ifr_index;
    return TRUE;
}

/* The interfaces mDNS can use now. Returns how many were written. */
static LONG
find_interfaces(struct Library *SocketBase, struct found *out, LONG max)
{
    char buf[2048];
    struct ifconf ifc;
    struct ifreq *ifr;
    LONG count = 0;
    int sock, sock6;

    memset(out, 0, sizeof(*out) * max);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
    }
    if (sock >= 0 && IoctlSocket(sock, SIOCGIFCONF, (char *)&ifc) == 0) {
        struct ifreq *end;

        ifr = ifc.ifc_req;
        end = (struct ifreq *)&ifc.ifc_buf[ifc.ifc_len];
        while (ifr < end && count < max) {
            struct sockaddr *sa = &ifr->ifr_addr;

            if (sa->sa_family == AF_INET) {
                ULONG addr = (ULONG)ntohl((unsigned long)
                    ((struct sockaddr_in *)sa)->sin_addr.s_addr);
                ULONG index;

                if (addr != 0 && usable_interface(SocketBase, sock,
                        ifr->ifr_name, &index)) {
                    struct found *item = found_interface(out, &count, max,
                        ifr->ifr_name);

                    if (item != NULL) {
                        item->index = index;
                        item->addr = addr;
                    }
                }
            }
            /* an entry is the name and an address of at least sockaddr's size */
            if (sa->sa_len < sizeof(*sa)) {
                ifr = (struct ifreq *)((char *)sa + sizeof(*sa));
            } else {
                ifr = (struct ifreq *)((char *)sa + sa->sa_len);
            }
        }
    }
    if (sock >= 0) {
        CloseSocket(sock);
    }

    /* AROSTCP enumerates IPv6 addresses separately. Keep one link-local
     * address for each link; it is the address usable with ff02::fb. */
    sock6 = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock6 >= 0) {
        struct in6_ifreq req[32];
        struct in6_ifconf ifc6;

        memset(req, 0, sizeof(req));
        ifc6.ifc6_buf = (char *)req;
        ifc6.ifc6_len = sizeof(req);
        if (IoctlSocket(sock6, SIOCGLIFADDR, (char *)&ifc6) == 0) {
            LONG i, n;

            n = ifc6.ifc6_len / (LONG)sizeof(req[0]);
            for (i = 0; i < n; i++) {
                ULONG index;
                struct found *item;

                if (!IN6_IS_ADDR_LINKLOCAL(&req[i].ifr6_addr.sin6_addr) ||
                    !usable_interface(SocketBase, sock6, req[i].ifr_name,
                        &index)) {
                    continue;
                }
                item = found_interface(out, &count, max, req[i].ifr_name);
                if (item != NULL && IN6_IS_ADDR_UNSPECIFIED(&item->addr6)) {
                    item->index = index;
                    item->addr6 = req[i].ifr6_addr.sin6_addr;
                }
            }
        }
        CloseSocket(sock6);
    }
    return count;
}

static LONG
open_socket4(struct Library *SocketBase, const char *name, ULONG addr)
{
    struct sockaddr_in sin;
    struct ip_mreq mreq;
    int on;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }

    /* Every responder of the host shares the port, here one per interface. */
    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }
#ifdef SO_REUSEPORT
    on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));
#endif
    /* Only what arrives on this interface; what is sent leaves there. */
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)name,
            (int)mdnsTextLength(name, IFNAMSIZ)) < 0) {
        CloseSocket(sock);
        return -1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(MDNS_DNS_PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        CloseSocket(sock);
        return -1;
    }

    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(MDNS_MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(addr);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
            sizeof(mreq)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
            (char *)&mreq.imr_interface, sizeof(mreq.imr_interface)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    {
        /*
         * BSD takes these two as one byte. No copy of what mdnsd sends comes
         * back to it: a copy sent just after the interface changed its
         * address would look like another host using this host's names.
         */
        unsigned char value;

        value = 255;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&value,
                sizeof(value)) < 0) {
            CloseSocket(sock);
            return -1;
        }
        value = 0;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&value,
                sizeof(value)) < 0) {
            CloseSocket(sock);
            return -1;
        }
    }
    return (LONG)sock;
}

static LONG
open_socket6(struct Library *SocketBase, const struct MDNSDaemon *md)
{
    static const UBYTE group_bytes[16] = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb
    };
    struct sockaddr_in6 sin6;
    int on, sock;
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        if (!IN6_IS_ADDR_UNSPECIFIED(&md->md_Ifs[i].mi_Addr6)) {
            break;
        }
    }
    if (i == md->md_IfCount) {
        return -1;
    }

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));
#endif
    /* AROSTCP's SO_BINDTODEVICE is currently IPv4-only. One wildcard IPv6
     * socket joins every link and IPV6_PKTINFO identifies the receiving
     * link, including for unicast questions. */
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on,
            sizeof(on)) < 0 ||
        setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_len = sizeof(sin6);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(MDNS_DNS_PORT);
    if (bind(sock, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        CloseSocket(sock);
        return -1;
    }

    for (i = 0; i < md->md_IfCount; i++) {
        struct ipv6_mreq mreq;

        if (IN6_IS_ADDR_UNSPECIFIED(&md->md_Ifs[i].mi_Addr6)) {
            continue;
        }
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.ipv6mr_multiaddr, group_bytes, sizeof(group_bytes));
        mreq.ipv6mr_interface = (unsigned int)md->md_Ifs[i].mi_Index;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *)&mreq,
                sizeof(mreq)) < 0) {
            CloseSocket(sock);
            return -1;
        }
    }
    on = 255;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    on = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    return (LONG)sock;
}

static void
set_primary(struct Library *SocketBase, struct MDNSDaemon *md)
{
    LONG i;

    md->md_Socket = md->md_Socket6;
    for (i = 0; i < md->md_IfCount && md->md_Socket < 0; i++) {
        md->md_Socket = md->md_Ifs[i].mi_Socket;
    }
    md->md_HostAddr = 0;
    for (i = 0; i < md->md_IfCount && md->md_HostAddr == 0; i++) {
        md->md_HostAddr = md->md_Ifs[i].mi_Addr;
    }

    {
        char base[MDNS_MAX_HOST + 1];
        char *dot;

        base[0] = '\0';
        if (gethostname(base, MDNS_MAX_HOST) != 0 || base[0] == '\0') {
            mdnsTextCopy(base, (LONG)sizeof(base), "aros");
        }
        base[MDNS_MAX_HOST] = '\0';
        /* over mDNS a host is its first label, in the mDNS domain */
        if ((dot = strchr(base, '.')) != NULL) {
            *dot = '\0';
        }
        /* another name from the system starts without a number again */
        if (strcmp(base, md->md_HostBase) != 0) {
            mdnsTextCopy(md->md_HostBase, (LONG)sizeof(md->md_HostBase),
                base);
            md->md_HostSuffix = 0;
        }
    }
    mdnsNetNameHost(md);
}

/* The name in use, from what the system calls itself and the number the
 * host took after another host turned out to have the name. */
void
mdnsNetNameHost(struct MDNSDaemon *md)
{
    char digits[12];
    ULONG number = md->md_HostSuffix;
    LONG n = 0, len;

    mdnsTextCopy(md->md_HostName, (LONG)sizeof(md->md_HostName),
        md->md_HostBase);
    if (number == 0) {
        return;
    }
    do {
        digits[n++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0);
    /* a label has at most 63 characters */
    len = mdnsTextLength(md->md_HostName, MDNS_MAX_HOST);
    if (len > 63 - 1 - n) {
        len = 63 - 1 - n;
    }
    md->md_HostName[len++] = '-';
    while (n > 0) {
        md->md_HostName[len++] = digits[--n];
    }
    md->md_HostName[len] = '\0';
}

/*
 * Brings the sockets in line with the interfaces the stack has now: one that
 * went away or changed its address loses its socket, a new one gets one.
 * Returns TRUE if anything changed.
 */
BOOL
mdnsNetRescan(struct MDNSDaemon *md)
{
    struct Library *SocketBase;
    struct found now[MDNS_IFS_MAX];
    BOOL changed = FALSE;
    LONG count, i, k;

    if (md == NULL || md->md_SocketBase == NULL) {
        return FALSE;
    }
    SocketBase = md->md_SocketBase;
    count = find_interfaces(SocketBase, now, MDNS_IFS_MAX);

    for (i = 0; i < md->md_IfCount; ) {
        for (k = 0; k < count; k++) {
            if (now[k].index == md->md_Ifs[i].mi_Index &&
                now[k].addr == md->md_Ifs[i].mi_Addr &&
                memcmp(&now[k].addr6, &md->md_Ifs[i].mi_Addr6,
                    sizeof(now[k].addr6)) == 0 &&
                strcmp(now[k].name, md->md_Ifs[i].mi_Name) == 0) {
                break;
            }
        }
        if (k < count) {
            now[k].index = 0;        /* kept */
            i++;
            continue;
        }
        if (md->md_Ifs[i].mi_Socket >= 0) {
            CloseSocket((int)md->md_Ifs[i].mi_Socket);
        }
        md->md_Ifs[i] = md->md_Ifs[--md->md_IfCount];
        changed = TRUE;
    }
    for (k = 0; k < count; k++) {
        LONG sock = -1;
        BOOL have6;

        if (now[k].index == 0 || md->md_IfCount >= MDNS_IFS_MAX) {
            continue;
        }
        if (now[k].addr != 0) {
            sock = open_socket4(SocketBase, now[k].name, now[k].addr);
        }
        have6 = !IN6_IS_ADDR_UNSPECIFIED(&now[k].addr6);
        if (sock < 0 && !have6) {
            continue;       /* tried again at the next look */
        }
        mdnsTextCopy(md->md_Ifs[md->md_IfCount].mi_Name,
            (LONG)sizeof(md->md_Ifs[0].mi_Name), now[k].name);
        md->md_Ifs[md->md_IfCount].mi_Index = now[k].index;
        md->md_Ifs[md->md_IfCount].mi_Addr = sock >= 0 ? now[k].addr : 0;
        md->md_Ifs[md->md_IfCount].mi_Addr6 = now[k].addr6;
        md->md_Ifs[md->md_IfCount].mi_Socket = sock;
        md->md_IfCount++;
        changed = TRUE;
    }
    if (changed) {
        if (md->md_Socket6 >= 0) {
            CloseSocket((int)md->md_Socket6);
        }
        md->md_Socket6 = open_socket6(SocketBase, md);
        if (md->md_Socket6 < 0) {
            /* Keep IPv4 service. IPv6-only links are retried at the next
             * scan; dual-stack links temporarily stop publishing AAAA. */
            for (i = 0; i < md->md_IfCount; ) {
                if (md->md_Ifs[i].mi_Addr == 0) {
                    md->md_Ifs[i] = md->md_Ifs[--md->md_IfCount];
                } else {
                    memset(&md->md_Ifs[i].mi_Addr6, 0,
                        sizeof(md->md_Ifs[i].mi_Addr6));
                    i++;
                }
            }
        }
        set_primary(SocketBase, md);
    }
    return changed;
}


static void
close_sockets(struct Library *SocketBase, struct MDNSDaemon *md)
{
    while (md->md_IfCount > 0) {
        struct MDNSInterface *mi = &md->md_Ifs[--md->md_IfCount];

        if (mi->mi_Socket >= 0) {
            CloseSocket((int)mi->mi_Socket);
        }
    }
    if (md->md_Socket6 >= 0) {
        CloseSocket((int)md->md_Socket6);
        md->md_Socket6 = -1;
    }
}

void
mdnsNetClose(struct MDNSDaemon *md)
{
    if (md == NULL || md->md_SocketBase == NULL) {
        return;
    }
    close_sockets(md->md_SocketBase, md);
    md->md_Socket = -1;
    md->md_HostAddr = 0;
}

/* Is this one of the host's own addresses? */
BOOL
mdnsNetIsOwn(struct MDNSDaemon *md, ULONG addr)
{
    LONG i;

    if (md->md_SendIf >= 0 && md->md_SendIf < md->md_IfCount) {
        return md->md_Ifs[md->md_SendIf].mi_Addr == addr;
    }
    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Addr == addr) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
mdnsNetIsOwn6(struct MDNSDaemon *md, const struct in6_addr *addr)
{
    LONG i;

    if (md->md_SendIf >= 0 && md->md_SendIf < md->md_IfCount) {
        return !IN6_IS_ADDR_UNSPECIFIED(
                &md->md_Ifs[md->md_SendIf].mi_Addr6) &&
            memcmp(&md->md_Ifs[md->md_SendIf].mi_Addr6, addr,
                sizeof(*addr)) == 0;
    }
    for (i = 0; i < md->md_IfCount; i++) {
        if (!IN6_IS_ADDR_UNSPECIFIED(&md->md_Ifs[i].mi_Addr6) &&
            memcmp(&md->md_Ifs[i].mi_Addr6, addr, sizeof(*addr)) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Sends on one interface: to the mDNS groups, or to the one who asked. */
static BOOL
send_on4(struct Library *SocketBase, struct MDNSDaemon *md, LONG index,
    const UBYTE *pkt, LONG pktlen)
{
    struct sockaddr_in sin;

    if (md->md_Ifs[index].mi_Socket < 0 ||
        (!mdnsReplyIsUnicast(md) && md->md_SendFamily != 0 &&
         md->md_SendFamily != AF_INET) ||
        (mdnsReplyIsUnicast(md) && md->md_ReplyFamily != 0 &&
         md->md_ReplyFamily != AF_INET)) {
        return FALSE;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    if (mdnsReplyIsUnicast(md)) {
        /* an answer for the one who asked */
        sin.sin_port = htons(md->md_ReplyPort);
        sin.sin_addr.s_addr = htonl(md->md_ReplyAddr);
    } else {
        sin.sin_port = htons(MDNS_DNS_PORT);
        sin.sin_addr.s_addr = inet_addr(MDNS_MCAST_ADDR);
    }
    if (sendto((int)md->md_Ifs[index].mi_Socket, (char *)pkt, (int)pktlen, 0,
            (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        return FALSE;
    }
    md->md_TxPackets++;
    return TRUE;
}

static BOOL
send_on6(struct Library *SocketBase, struct MDNSDaemon *md, LONG index,
    const UBYTE *pkt, LONG pktlen)
{
    static const UBYTE group_bytes[16] = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb
    };
    struct sockaddr_in6 sin6;

    if (md->md_Socket6 < 0 ||
        IN6_IS_ADDR_UNSPECIFIED(&md->md_Ifs[index].mi_Addr6) ||
        (!mdnsReplyIsUnicast(md) && md->md_SendFamily != 0 &&
         md->md_SendFamily != AF_INET6) ||
        (mdnsReplyIsUnicast(md) && md->md_ReplyFamily != AF_INET6)) {
        return FALSE;
    }
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_len = sizeof(sin6);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_scope_id = md->md_Ifs[index].mi_Index;
    if (mdnsReplyIsUnicast(md)) {
        sin6.sin6_port = htons(md->md_ReplyPort);
        sin6.sin6_addr = md->md_ReplyAddr6;
    } else {
        sin6.sin6_port = htons(MDNS_DNS_PORT);
        memcpy(&sin6.sin6_addr, group_bytes, sizeof(group_bytes));
        if (setsockopt((int)md->md_Socket6, IPPROTO_IPV6,
                IPV6_MULTICAST_IF, (char *)&md->md_Ifs[index].mi_Index,
                sizeof(md->md_Ifs[index].mi_Index)) < 0) {
            return FALSE;
        }
    }
    if (sendto((int)md->md_Socket6, (char *)pkt,
            (int)pktlen, 0, (struct sockaddr *)&sin6, sizeof(sin6)) < 0) {
        return FALSE;
    }
    md->md_TxPackets++;
    return TRUE;
}

BOOL
mdnsNetSendOn(struct MDNSDaemon *md, LONG index, const UBYTE *pkt,
    LONG pktlen)
{
    if (md == NULL || md->md_SocketBase == NULL || pkt == NULL ||
        pktlen <= 0 || index < 0 || index >= md->md_IfCount) {
        return FALSE;
    }
    {
        BOOL sent4 = send_on4(md->md_SocketBase, md, index, pkt, pktlen);
        BOOL sent6 = send_on6(md->md_SocketBase, md, index, pkt, pktlen);

        return sent4 || sent6;
    }
}

/* Sends a packet that is the same on every link: to the interface a
 * packet being answered came in on, or else to all of them. */
void
mdnsNetSendAll(struct MDNSDaemon *md, const UBYTE *pkt, LONG pktlen)
{
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_SendIf < 0 || i == md->md_SendIf) {
            mdnsNetSendOn(md, i, pkt, pktlen);
        }
    }
}
