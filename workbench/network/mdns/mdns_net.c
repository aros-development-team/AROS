/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_net.c - one UDP 5353 socket for each interface (bsdsocket.library)
 *
 * mDNS is a protocol of the link: what is heard on an interface is answered
 * there, and an address record carries the address of the interface it is
 * sent on. Each interface that is up, can multicast and has an IPv4 address
 * gets a socket bound to it with SO_BINDTODEVICE.
 */

#include <exec/types.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mdnsd.h"

struct found
{
    char    name[IFNAMSIZ + 1];
    ULONG   addr;               /* 0xAABBCCDD == A.B.C.D */
};

/* The interfaces mDNS can use now. Returns how many were written. */
static LONG
find_interfaces(struct Library *SocketBase, struct found *out, LONG max)
{
    char buf[2048];
    struct ifconf ifc;
    struct ifreq *ifr, *end, flags;
    LONG count = 0;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return 0;
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (IoctlSocket(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
        CloseSocket(sock);
        return 0;
    }

    ifr = ifc.ifc_req;
    end = (struct ifreq *)&ifc.ifc_buf[ifc.ifc_len];
    while (ifr < end && count < max) {
        struct sockaddr *sa = &ifr->ifr_addr;

        if (sa->sa_family == AF_INET) {
            ULONG addr = (ULONG)ntohl((unsigned long)
                ((struct sockaddr_in *)sa)->sin_addr.s_addr);

            memset(&flags, 0, sizeof(flags));
            memcpy(flags.ifr_name, ifr->ifr_name, IFNAMSIZ);
            if (addr != 0 &&
                IoctlSocket(sock, SIOCGIFFLAGS, (char *)&flags) == 0 &&
                (flags.ifr_flags & (IFF_UP | IFF_MULTICAST | IFF_LOOPBACK)) ==
                    (IFF_UP | IFF_MULTICAST)) {
                /* ifr_name need not end with a NUL; name[] is one longer */
                memset(out[count].name, 0, sizeof(out[count].name));
                memcpy(out[count].name, ifr->ifr_name, IFNAMSIZ);
                out[count].addr = addr;
                count++;
            }
        }
        /* an entry is the name and an address of at least sockaddr's size */
        if (sa->sa_len < sizeof(*sa)) {
            ifr = (struct ifreq *)((char *)sa + sizeof(*sa));
        } else {
            ifr = (struct ifreq *)((char *)sa + sa->sa_len);
        }
    }
    CloseSocket(sock);
    return count;
}

static LONG
open_socket(struct Library *SocketBase, const char *name, ULONG addr)
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
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
#ifdef SO_REUSEPORT
    on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));
#endif
    /* Only what arrives on this interface; what is sent leaves there. */
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)name,
        (int)mdnsTextLength(name, IFNAMSIZ));

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
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
        (char *)&mreq.imr_interface, sizeof(mreq.imr_interface));
    {
        /*
         * BSD takes these two as one byte. No copy of what mdnsd sends comes
         * back to it: a copy sent just after the interface changed its
         * address would look like another host using this host's names.
         */
        unsigned char value;

        value = 255;
        setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&value,
            sizeof(value));
        value = 0;
        setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&value,
            sizeof(value));
    }
    return (LONG)sock;
}

static void
set_primary(struct Library *SocketBase, struct MDNSDaemon *md)
{
    md->md_Socket = md->md_IfCount > 0 ? md->md_Ifs[0].mi_Socket : -1;
    md->md_HostAddr = md->md_IfCount > 0 ? md->md_Ifs[0].mi_Addr : 0;

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
            if (now[k].addr == md->md_Ifs[i].mi_Addr &&
                strcmp(now[k].name, md->md_Ifs[i].mi_Name) == 0) {
                break;
            }
        }
        if (k < count) {
            now[k].addr = 0;        /* kept */
            i++;
            continue;
        }
        CloseSocket((int)md->md_Ifs[i].mi_Socket);
        md->md_Ifs[i] = md->md_Ifs[--md->md_IfCount];
        changed = TRUE;
    }
    for (k = 0; k < count; k++) {
        LONG sock;

        if (now[k].addr == 0 || md->md_IfCount >= MDNS_IFS_MAX) {
            continue;
        }
        sock = open_socket(SocketBase, now[k].name, now[k].addr);
        if (sock < 0) {
            continue;       /* tried again at the next look */
        }
        mdnsTextCopy(md->md_Ifs[md->md_IfCount].mi_Name,
            (LONG)sizeof(md->md_Ifs[0].mi_Name), now[k].name);
        md->md_Ifs[md->md_IfCount].mi_Addr = now[k].addr;
        md->md_Ifs[md->md_IfCount].mi_Socket = sock;
        md->md_IfCount++;
        changed = TRUE;
    }
    if (changed) {
        set_primary(SocketBase, md);
    }
    return changed;
}


static void
close_sockets(struct Library *SocketBase, struct MDNSDaemon *md)
{
    while (md->md_IfCount > 0) {
        CloseSocket((int)md->md_Ifs[--md->md_IfCount].mi_Socket);
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

    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Addr == addr) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Sends on one interface: to the mDNS group, or to the one who asked when
 * md_ReplyAddr names one. */
static BOOL
send_on(struct Library *SocketBase, struct MDNSDaemon *md, LONG index,
    const UBYTE *pkt, LONG pktlen)
{
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    if (md->md_ReplyAddr != 0) {
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

BOOL
mdnsNetSendOn(struct MDNSDaemon *md, LONG index, const UBYTE *pkt,
    LONG pktlen)
{
    if (md == NULL || md->md_SocketBase == NULL || pkt == NULL ||
        pktlen <= 0 || index < 0 || index >= md->md_IfCount) {
        return FALSE;
    }
    return send_on(md->md_SocketBase, md, index, pkt, pktlen);
}

