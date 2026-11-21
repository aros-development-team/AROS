/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_net.c - the UDP 5353 sockets, one per address family
 *
 * mDNS is a protocol of the link: what is heard on an interface is answered
 * there, and an address record carries the address of the interface it is
 * sent on. One socket per family serves every link: it joins the mDNS group
 * on each link that is up, can multicast and has an address of the family,
 * learns the link a datagram came by from IP_PKTINFO or IPV6_PKTINFO, and
 * sends on a link - to its group, or to the one who asked - with the same
 * ancillary data naming the link: the datagram leaves there whatever the
 * routing table says (a host route to the asker through another link; two
 * IPv4 link-local links, 169.254/16 routed by one), and an IPv4 one from
 * the address its records carry.
 * The sockets stay open while the stack is there: links coming, going or
 * changing their addresses change only the memberships (the stack leaves
 * those of an interface that goes away).
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
#include "dnssd_link_aros.h"

/* MDNS_ECHO_DEBUG: the echo journal's growth and evictions to the debug
 * output (tests: aros-matter tests/mdns/samesegment.sh) */
#ifdef MDNS_ECHO_DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

/* Options both sockets take; FALSE if one is refused. */
static BOOL
share_port(struct Library *SocketBase, int sock)
{
    int on = 1;

    /* Every responder of the host shares the port. */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
            sizeof(on)) < 0) {
        return FALSE;
    }
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on));
#endif
    return TRUE;
}

static LONG
open_socket4(struct Library *SocketBase)
{
    struct sockaddr_in sin;
    unsigned char value;
    int on = 1;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    if (!share_port(SocketBase, sock) ||
        setsockopt(sock, IPPROTO_IP, IP_PKTINFO, (char *)&on,
            sizeof(on)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(MDNS_DNS_PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    /*
     * BSD takes these two as one byte. No copy of what mdnsd sends comes
     * back to it: a copy sent just after the interface changed its address
     * would look like another host using this host's names.
     */
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
    return (LONG)sock;
}

static const struct dnssd_link_entry *active_link(
    const struct MDNSDaemon *md, LONG slot)
{
    if (md == NULL || slot < 0 || slot >= md->md_IfCount)
        return NULL;
    return dnssd_link_inventory_find(&md->md_LinkInventory,
                                     md->md_Ifs[slot].mi_Index);
}

const struct dnssd_address_entry *
mdnsNetAddressAt(const struct MDNSDaemon *md, LONG slot, UWORD family,
                 size_t ordinal)
{
    const struct dnssd_link_entry *link = active_link(md, slot);
    size_t i;

    if (link == NULL)
        return NULL;
    for (i = 0; i < md->md_LinkInventory.address_count; ++i) {
        const struct dnssd_address_entry *address =
            &md->md_LinkInventory.addresses[i];

        if (address->value.interface_index == link->value.interface_index &&
            address->value.family == family &&
            dnssd_link_address_usable(&md->md_LinkInventory, address)) {
            if (ordinal == 0u)
                return address;
            --ordinal;
        }
    }
    return NULL;
}

ULONG
mdnsNetAddress4(const struct MDNSDaemon *md, LONG slot)
{
    const struct dnssd_address_entry *address;
    ULONG network;

    address = mdnsNetAddressAt(md, slot, DNSSD_ADDRESS_IPV4, 0u);
    if (address == NULL)
        return 0;
    memcpy(&network, address->value.bytes, sizeof(network));
    return (ULONG)ntohl((unsigned long)network);
}

BOOL
mdnsNetAddress6(const struct MDNSDaemon *md, LONG slot,
                struct in6_addr *result)
{
    const struct dnssd_address_entry *address;

    if (result == NULL)
        return FALSE;
    memset(result, 0, sizeof(*result));
    address = mdnsNetAddressAt(md, slot, DNSSD_ADDRESS_IPV6, 0u);
    if (address == NULL)
        return FALSE;
    memcpy(result, address->value.bytes, sizeof(*result));
    return TRUE;
}

static BOOL
has_address6(const struct MDNSDaemon *md, LONG slot)
{
    struct in6_addr address;

    return mdnsNetAddress6(md, slot, &address);
}

static LONG
open_socket6(struct Library *SocketBase)
{
    struct sockaddr_in6 sin6;
    int on = 1, sock;

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    if (!share_port(SocketBase, sock) ||
        setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on,
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

/*
 * Each link joins the group of each family it has an address of. A
 * membership the socket has already is refused (EADDRINUSE) and kept; one
 * of a link that lost its addresses stays until the interface goes, and
 * hears nothing mdnsd answers: no slot names the link.
 */
static void
join_links(struct Library *SocketBase, struct MDNSDaemon *md)
{
    static const UBYTE group_bytes[16] = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb
    };
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        ULONG address4 = mdnsNetAddress4(md, i);

        if (md->md_Socket4 >= 0 && address4 != 0) {
            struct ip_mreq mreq;

            memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = inet_addr(MDNS_MCAST_ADDR);
            mreq.imr_interface.s_addr = htonl(address4);
            setsockopt((int)md->md_Socket4, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                (char *)&mreq, sizeof(mreq));
        }
        if (md->md_Socket6 >= 0 && has_address6(md, i)) {
            struct ipv6_mreq mreq;

            memset(&mreq, 0, sizeof(mreq));
            memcpy(&mreq.ipv6mr_multiaddr, group_bytes, sizeof(group_bytes));
            mreq.ipv6mr_interface = (unsigned int)md->md_Ifs[i].mi_Index;
            setsockopt((int)md->md_Socket6, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                (char *)&mreq, sizeof(mreq));
        }
    }
}

/*
 * The system's host name as mDNS has it: its first label, "aros" when it has
 * none (base: MDNSD_HOST_MAX + 1 bytes).
 */
void
mdnsNetSystemName(struct Library *SocketBase, char *base)
{
    char *dot;

    base[0] = '\0';
    if (gethostname(base, MDNSD_HOST_MAX) != 0 || base[0] == '\0') {
        mdnsTextCopy(base, MDNSD_HOST_MAX + 1, "aros");
    }
    base[MDNSD_HOST_MAX] = '\0';
    /* over mDNS a host is its first label, in the mDNS domain */
    if ((dot = strchr(base, '.')) != NULL) {
        *dot = '\0';
    }
}

static void
set_primary(struct Library *SocketBase, struct MDNSDaemon *md)
{
    LONG i;

    md->md_Socket = md->md_IfCount == 0 ? -1 :
        md->md_Socket6 >= 0 ? md->md_Socket6 : md->md_Socket4;
    md->md_HostAddr = 0;
    for (i = 0; i < md->md_IfCount && md->md_HostAddr == 0; i++) {
        md->md_HostAddr = mdnsNetAddress4(md, i);
    }

    {
        char base[MDNSD_HOST_MAX + 1];

        mdnsNetSystemName(SocketBase, base);
        /* another name from the system starts without a number again, and
         * what was announced under the old one goes with a goodbye - the
         * links may change in the same turn as the name (mdnsHostRename()) */
        mdnsHostRename(md, base);
    }
}

static void close_sockets(struct Library *SocketBase, struct MDNSDaemon *md);

/* The links after a complete canonical snapshot changed: the sockets are
 * opened the first time, and each link joins the groups. What changed is in
 * changes, whose items the caller frees (FreeVec()). */
BOOL
mdnsNetRescan(struct MDNSDaemon *md, struct dnssd_link_changes *changes)
{
    struct Library *SocketBase;
    uint32_t previous_revision;
    size_t i;
    int result;

    if (md == NULL || md->md_SocketBase == NULL) {
        return FALSE;
    }
    SocketBase = md->md_SocketBase;
    previous_revision = md->md_LinkInventory.revision;
    /* room for whatever this rescan can change; without it, the changes
     * overflow, and everything is taken as changed */
    changes->capacity = dnssd_link_changes_bound(&md->md_LinkInventory);
    changes->items = AllocVec(changes->capacity * sizeof(*changes->items),
        MEMF_ANY);
    if (changes->items == NULL) {
        changes->capacity = 0;
    }
    result = dnssd_link_aros_reconcile(&md->md_LinkInventory, SocketBase,
                                       MDNS_LINK_SNAPSHOT_MAX, changes);
    if (result != DNSSD_LINK_OK ||
        md->md_LinkInventory.revision == previous_revision)
        return FALSE;

    if (md->md_Socket4 < 0) {
        md->md_Socket4 = open_socket4(SocketBase);
    }
    if (md->md_Socket6 < 0) {
        md->md_Socket6 = open_socket6(SocketBase);
    }
    md->md_IfCount = 0;
    for (i = 0; i < md->md_LinkInventory.link_count &&
            md->md_IfCount < MDNS_IFS_MAX; ++i) {
        const struct dnssd_link_entry *link = &md->md_LinkInventory.links[i];
        LONG slot = md->md_IfCount;

        if (dnssd_link_usable_address_count(&md->md_LinkInventory,
                link->value.interface_index) == 0u)
            continue;
        md->md_Ifs[slot].mi_Index = link->value.interface_index;
        ++md->md_IfCount;
        /* a link is served in a family it has an address and a socket of */
        if (!(md->md_Socket4 >= 0 && mdnsNetAddress4(md, slot) != 0u) &&
            !(md->md_Socket6 >= 0 && has_address6(md, slot)))
            --md->md_IfCount;
    }
    join_links(SocketBase, md);
    set_primary(SocketBase, md);
    return TRUE;
}


static void
close_sockets(struct Library *SocketBase, struct MDNSDaemon *md)
{
    md->md_IfCount = 0;
    if (md->md_Socket4 >= 0) {
        CloseSocket((int)md->md_Socket4);
        md->md_Socket4 = -1;
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
    dnssd_link_inventory_reconcile(&md->md_LinkInventory, NULL, 0u,
                                   NULL, 0u);
    mdnsHostLinksClear(md);
    if (md->md_Sent != NULL) {          /* no echo comes without sockets */
        FreeVec(md->md_Sent);
        md->md_Sent = NULL;
    }
    md->md_SentMax = md->md_SentFirst = md->md_SentCount = 0;
    md->md_Socket = -1;
    md->md_HostAddr = 0;
}

/* Is this one of the host's own addresses? */
BOOL
mdnsNetIsOwn(struct MDNSDaemon *md, ULONG addr)
{
    size_t i;
    ULONG link = 0u;

    if (md->md_SendIf >= 0 && md->md_SendIf < md->md_IfCount)
        link = md->md_Ifs[md->md_SendIf].mi_Index;
    for (i = 0; i < md->md_LinkInventory.address_count; ++i) {
        const struct dnssd_address_entry *address =
            &md->md_LinkInventory.addresses[i];
        ULONG network;

        if (address->value.family != DNSSD_ADDRESS_IPV4 ||
            (link != 0u && address->value.interface_index != link))
            continue;
        memcpy(&network, address->value.bytes, sizeof(network));
        if ((ULONG)ntohl((unsigned long)network) == addr)
            return TRUE;
    }
    return FALSE;
}

BOOL
mdnsNetIsOwn6(struct MDNSDaemon *md, const struct in6_addr *addr)
{
    size_t i;
    ULONG link = 0u;

    if (md->md_SendIf >= 0 && md->md_SendIf < md->md_IfCount)
        link = md->md_Ifs[md->md_SendIf].mi_Index;
    for (i = 0; i < md->md_LinkInventory.address_count; ++i) {
        const struct dnssd_address_entry *address =
            &md->md_LinkInventory.addresses[i];

        if (address->value.family == DNSSD_ADDRESS_IPV6 &&
            (link == 0u || address->value.interface_index == link) &&
            memcmp(address->value.bytes, addr, sizeof(*addr)) == 0)
            return TRUE;
    }
    return FALSE;
}

/*
 * mdnsd's own packets, heard by another of its interfaces (RFC 6762 section
 * 14: on a segment that two of them share - two cards on one switch, a
 * bridge - each hears what the other sends). Taken for another host's, the
 * host's probes lose the tie-break to themselves and its announcements are a
 * conflict with itself: it never ends probing, or takes another name at each
 * round (B60).
 *
 * By address such a packet cannot be told from one of a host on a separate
 * network that has an address this host has on another link: that one must
 * still be heard, and still be a conflict if it claims this host's names.
 * So by where it went and what it was: what mdnsd sends to the group is kept
 * with the link it went out by (struct MDNSSent), and a packet is its own
 * echo only if it comes in by another link, from the address the host has
 * on the link it went out by, and is the same packet, byte for byte (its
 * signature). A packet identical to one this host sent carries the same
 * records, which are no conflict whoever sent them; another, from whatever
 * address, goes on to the rest of mdnsd. When in doubt - a signature gone,
 * an echo later than it - a packet is another host's: at worst a conflict
 * with itself, never one of another host's missed.
 *
 * How long a signature is kept is not a count or a time of the clock alone:
 * an echo is on the receiving socket within a second of its sending, so a
 * signature goes once that socket has been read past what was there a
 * second or more after it was sent - seen with nothing to read
 * (mdnsNetIdle()), or the echo of a packet sent a second or more later
 * read from it (the socket gives what came in order) - however slowly
 * mdnsd gets to reading. The ring grows by half when it is full of
 * signatures still waiting, and shrinks when a quarter of it is used;
 * past MDNS_ECHO_KEEP_TICKS a signature goes whatever happens. With one
 * link nothing is kept: nothing can come back by another.
 */
#define MDNS_SENT_MIN           32
#define MDNS_ECHO_TICKS         50      /* 1 s: an echo is on its way by then */
#define MDNS_ECHO_KEEP_TICKS    1500    /* 30 s at most, whatever happens */
#ifndef MDNS_SENT_CAP
#define MDNS_SENT_CAP           0x8000  /* signatures at most: what memory gives */
#endif

static void
sent_sign(const UBYTE *pkt, LONG pktlen, ULONG *hash, ULONG *sum)
{
    ULONG h = 2166136261UL, a = 1, b = 0;
    LONG i;

    for (i = 0; i < pktlen; i++) {
        h = (h ^ pkt[i]) * 16777619UL;
        a += pkt[i];
        b += a;
    }
    *hash = h;
    *sum = (b << 16) ^ a;
}

static struct MDNSSent *
sent_at(const struct MDNSDaemon *md, UWORD k)
{
    return &md->md_Sent[(md->md_SentFirst + k) % md->md_SentMax];
}

static UWORD
sent_length(UWORD af, LONG pktlen)
{
    return (UWORD)(pktlen | (af == AF_INET6 ? MDNS_SENT_IPV6 : 0));
}

/* The ring with room for max signatures, the waiting ones kept in order;
 * FALSE with no memory for it. */
static BOOL
sent_resize(struct MDNSDaemon *md, UWORD max)
{
    struct MDNSSent *ring = AllocVec((ULONG)max * sizeof(*ring), MEMF_ANY);
    UWORD k;

    if (ring == NULL) {
        return FALSE;
    }
    for (k = 0; k < md->md_SentCount; k++) {
        ring[k] = *sent_at(md, k);
    }
    if (md->md_Sent != NULL) {
        FreeVec(md->md_Sent);
    }
    md->md_Sent = ring;
    md->md_SentMax = max;
    md->md_SentFirst = 0;
    D(bug("[mdnsd] echo journal: %u signatures (%u waiting)\n", max,
        md->md_SentCount));
    return TRUE;
}

/* Drops the signatures whose echo cannot come any more. */
static void
sent_purge(struct MDNSDaemon *md, ULONG now)
{
    while (md->md_SentCount > 0) {
        const struct MDNSSent *s = sent_at(md, 0);
        ULONG idle = (s->s_Length & MDNS_SENT_IPV6) ? md->md_Idle6 :
            md->md_Idle4;

        if ((LONG)(idle - (s->s_Tick + MDNS_ECHO_TICKS)) < 0 &&
            (LONG)(now - (s->s_Tick + MDNS_ECHO_KEEP_TICKS)) < 0) {
            break;
        }
        md->md_SentFirst = (UWORD)((md->md_SentFirst + 1) % md->md_SentMax);
        md->md_SentCount--;
    }
    if (md->md_SentCount == 0 && md->md_SentMax > MDNS_SENT_MIN) {
        D(bug("[mdnsd] echo journal: empty, %u freed; echoes %lu\n",
            md->md_SentMax, md->md_RxEchoes));
        FreeVec(md->md_Sent);           /* back to the least, when needed */
        md->md_Sent = NULL;
        md->md_SentMax = 0;
    } else if (md->md_SentMax > MDNS_SENT_MIN &&
               md->md_SentCount <= md->md_SentMax / 4) {
        UWORD half = (UWORD)(md->md_SentMax / 2);

        sent_resize(md, half > MDNS_SENT_MIN ? half : MDNS_SENT_MIN);
    }
}

/* Room for one more signature: a ring half as big again when this one is
 * full of signatures still waiting for their echo; with no memory, the
 * oldest goes. */
static BOOL
sent_room(struct MDNSDaemon *md)
{
    ULONG max = md->md_SentMax != 0 ?
        (ULONG)md->md_SentMax + md->md_SentMax / 2 :
        (MDNS_SENT_MIN < MDNS_SENT_CAP ? MDNS_SENT_MIN : MDNS_SENT_CAP);

    if (md->md_Sent != NULL && md->md_SentCount < md->md_SentMax) {
        return TRUE;
    }
    if (max > MDNS_SENT_CAP) {
        max = MDNS_SENT_CAP;
    }
    if (max > md->md_SentMax && sent_resize(md, (UWORD)max)) {
        return TRUE;
    }
    if (md->md_Sent == NULL) {
        return FALSE;
    }
    D(bug("[mdnsd] echo journal: full at %u, the oldest dropped\n",
        md->md_SentMax));
    md->md_SentFirst = (UWORD)((md->md_SentFirst + 1) % md->md_SentMax);
    md->md_SentCount--;
    return TRUE;
}

/* A packet that went to the group by af: kept until its echo cannot come. */
static void
sent_keep(struct MDNSDaemon *md, LONG slot, UWORD af, const UBYTE *pkt,
    LONG pktlen)
{
    ULONG now = mdnsTicks();
    struct MDNSSent *s;

    if (md->md_IfCount < 2 || mdnsReplyIsUnicast(md)) {
        return;
    }
    sent_purge(md, now);
    if (!sent_room(md)) {
        return;
    }
    s = sent_at(md, md->md_SentCount++);
    s->s_Link = md->md_Ifs[slot].mi_Index;
    s->s_Tick = now;
    sent_sign(pkt, pktlen, &s->s_Hash, &s->s_Sum);
    s->s_Length = sent_length(af, pktlen);
}

/* The socket of af was seen with nothing to read: whatever this host sent
 * a second or more before cannot come back by it any more. */
void
mdnsNetIdle(struct MDNSDaemon *md, UWORD af)
{
    ULONG now = mdnsTicks();

    if (af == AF_INET6) {
        md->md_Idle6 = now;
    } else {
        md->md_Idle4 = now;
    }
    if (md->md_SentCount > 0) {
        sent_purge(md, now);
    }
}

/* Is source (4 or 16 bytes, by af) an address this host has on the link
 * with this interface index - or, with other, on any link but that one? */
static BOOL
own_address(const struct MDNSDaemon *md, ULONG link, BOOL other, UWORD af,
    const UBYTE *source)
{
    UWORD family = af == AF_INET6 ? DNSSD_ADDRESS_IPV6 : DNSSD_ADDRESS_IPV4;
    size_t i, length = af == AF_INET6 ? 16u : 4u;

    for (i = 0; i < md->md_LinkInventory.address_count; ++i) {
        const struct dnssd_address_entry *address =
            &md->md_LinkInventory.addresses[i];

        if (address->value.family == family &&
            (address->value.interface_index == link) != other &&
            memcmp(address->value.bytes, source, length) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * A packet that came in by the link in this slot, by af, from source (the
 * address as on the wire: 4 or 16 bytes): is it this host's own? From an
 * address of the host on that link it is - the group's traffic looped back;
 * from the address of another of its links only if it is a packet the host
 * sent out by that link (above). Anything else is another host's.
 */
BOOL
mdnsNetIsOwnPacket(struct MDNSDaemon *md, LONG slot, UWORD af,
    const UBYTE *source, const UBYTE *pkt, LONG pktlen)
{
    ULONG link = md->md_Ifs[slot].mi_Index, hash, sum;
    UWORD k;

    if (own_address(md, link, FALSE, af, source)) {
        return TRUE;
    }
    if (md->md_SentCount == 0 || !own_address(md, link, TRUE, af, source)) {
        return FALSE;
    }
    sent_sign(pkt, pktlen, &hash, &sum);
    for (k = 0; k < md->md_SentCount; k++) {
        const struct MDNSSent *s = sent_at(md, k);

        if (s->s_Hash == hash && s->s_Sum == sum &&
            s->s_Length == sent_length(af, pktlen) && s->s_Link != link &&
            own_address(md, s->s_Link, FALSE, af, source)) {
            ULONG *mark = af == AF_INET6 ? &md->md_Idle6 : &md->md_Idle4;

            /* read past what came a second before it went (above) */
            if ((LONG)(s->s_Tick - *mark) > 0) {
                *mark = s->s_Tick;
            }
            md->md_RxEchoes++;
            sent_purge(md, mdnsTicks());
            return TRUE;
        }
    }
    return FALSE;
}

/* Sends pkt to sa with one ancillary message of the given level and type. */
static BOOL
send_with(struct Library *SocketBase, LONG sock, struct sockaddr *sa,
    LONG salen, const UBYTE *pkt, LONG pktlen, int level, int type,
    const void *info, LONG infolen)
{
    union {
        struct cmsghdr align;
        UBYTE data[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    } control;
    struct msghdr message;
    struct iovec vector;
    struct cmsghdr *header;

    memset(&message, 0, sizeof(message));
    memset(&control, 0, sizeof(control));
    vector.iov_base = (APTR)pkt;
    vector.iov_len = pktlen;
    message.msg_name = sa;
    message.msg_namelen = salen;
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
    message.msg_control = &control;
    message.msg_controllen = CMSG_SPACE(infolen);
    header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = level;
    header->cmsg_type = type;
    header->cmsg_len = CMSG_LEN(infolen);
    memcpy(CMSG_DATA(header), info, infolen);
    return sendmsg((int)sock, &message, 0) >= 0;
}

/* Sends on one interface: to the mDNS groups, or to the one who asked. */
static BOOL
send_on4(struct Library *SocketBase, struct MDNSDaemon *md, LONG index,
    const UBYTE *pkt, LONG pktlen)
{
    struct sockaddr_in sin;
    struct in_pktinfo info;
    ULONG address4 = mdnsNetAddress4(md, index);

    if (md->md_Socket4 < 0 || address4 == 0 ||
        (!mdnsReplyIsUnicast(md) && md->md_SendFamily != 0 &&
         md->md_SendFamily != AF_INET) ||
        (mdnsReplyIsUnicast(md) && md->md_ReplyFamily != 0 &&
         md->md_ReplyFamily != AF_INET)) {
        return FALSE;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    if (mdnsReplyIsUnicast(md)) {
        /* an answer for the one who asked */
        sin.sin_port = htons(md->md_ReplyPort);
        sin.sin_addr.s_addr = htonl(md->md_ReplyAddr);
    } else {
        sin.sin_port = htons(MDNS_DNS_PORT);
        sin.sin_addr.s_addr = inet_addr(MDNS_MCAST_ADDR);
    }
    /* out by this link, from its address */
    memset(&info, 0, sizeof(info));
    info.ipi_ifindex = (int)md->md_Ifs[index].mi_Index;
    info.ipi_spec_dst.s_addr = htonl(address4);
    if (!send_with(SocketBase, md->md_Socket4, (struct sockaddr *)&sin,
            sizeof(sin), pkt, pktlen, IPPROTO_IP, IP_PKTINFO, &info,
            sizeof(info))) {
        return FALSE;
    }
    md->md_TxPackets++;
    sent_keep(md, index, AF_INET, pkt, pktlen);
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
    struct in6_pktinfo info;

    if (md->md_Socket6 < 0 || !has_address6(md, index) ||
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
    }
    /* out by this link; the stack takes the source from it */
    memset(&info, 0, sizeof(info));
    info.ipi6_ifindex = md->md_Ifs[index].mi_Index;
    if (!send_with(SocketBase, md->md_Socket6, (struct sockaddr *)&sin6,
            sizeof(sin6), pkt, pktlen, IPPROTO_IPV6, IPV6_PKTINFO, &info,
            sizeof(info))) {
        return FALSE;
    }
    md->md_TxPackets++;
    sent_keep(md, index, AF_INET6, pkt, pktlen);
    return TRUE;
}

/* Out by one family alone on this link (MDNS_FAM_*): a name's publication
 * there goes by each family on its own. */
BOOL
mdnsNetSendOnFamily(struct MDNSDaemon *md, LONG index, UWORD fam,
    const UBYTE *pkt, LONG pktlen)
{
    if (md == NULL || md->md_SocketBase == NULL || pkt == NULL ||
        pktlen <= 0 || index < 0 || index >= md->md_IfCount) {
        return FALSE;
    }
    return fam == MDNS_FAM_IPV6 ?
        send_on6(md->md_SocketBase, md, index, pkt, pktlen) :
        send_on4(md->md_SocketBase, md, index, pkt, pktlen);
}

/* The slot (md_Ifs[]) of the link with this interface index, or -1. */
LONG
mdnsNetSlot(const struct MDNSDaemon *md, ULONG link)
{
    LONG i;

    for (i = 0; link != 0 && i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Index == link) {
            return i;
        }
    }
    return -1;
}

/* Does the link in this slot have a usable address of the family? */
BOOL
mdnsNetHasFamily(const struct MDNSDaemon *md, LONG index, UWORD fam)
{
    return mdnsNetAddressAt(md, index, mdnsDnssdFamily(fam), 0) != NULL &&
        (fam == MDNS_FAM_IPV6 ? md->md_Socket6 >= 0 : md->md_Socket4 >= 0);
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
