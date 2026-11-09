/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the path MTU options of datagram sockets: RFC 3542
    section 11 for IPv6 (IPV6_USE_MIN_MTU, IPV6_DONTFRAG, IPV6_RECVPATHMTU,
    IPV6_PATHMTU), and IP_DONTFRAG for IPv4 - what an application that does
    its own path MTU discovery needs (QUIC, RFC 9000 section 14; RFC 8899).

    Routes to 2001:db8:4d54:43::/64 and 10.77.43.0/24 through routers on
    the link are added with a routing socket (nobody answers for them, so
    nothing sent there leaves); a Packet Too Big or "fragmentation needed"
    quoting a datagram to a destination behind them is sent to the loopback
    from a raw socket. What leaves the interface is read with the
    bsdsocket BPF calls. Usage:  cunit-net-udppmtu [IFNAME]  (default eth0).
    The tests that need the interface are skipped without one.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/route.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static int ready6, ready4;
static struct in_addr ours, gw4, neighbour;
static long bpf = -1;

#define GW6     "fe80::4d54:b"          /* a router on the link; nobody answers */
#define NET6    "2001:db8:4d54:43::"
#define DST6_A  "2001:db8:4d54:43::1"
#define DST6_B  "2001:db8:4d54:43::2"
#define DST6_C  "2001:db8:4d54:43::3"

#define NET4    "10.77.43.0"
#define DST4_A  "10.77.43.1"

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#define NEED6() do { if (!ready6) { CU_PASS("no IPv6 interface - skipping"); return; } } while (0)
#define NEED4() do { if (!ready4) { CU_PASS("no IPv4 interface - skipping"); return; } } while (0)

static void sa6(struct sockaddr_in6 *a, const char *addr, unsigned short port)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_port = htons(port);
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
}

static void sa4(struct sockaddr_in *a, struct in_addr addr, unsigned short port)
{
    bzero(a, sizeof(*a));
    a->sin_len = sizeof(*a);
    a->sin_family = AF_INET;
    a->sin_port = htons(port);
    a->sin_addr = addr;
}

static struct in_addr addr4(const char *s)
{
    struct in_addr a;

    inet_pton(AF_INET, (char *)s, &a);
    return a;
}

/* Add or delete a network route through a gateway with a routing socket. */
static int route(int type, int family, const char *dst, int plen)
{
    long buf[512 / sizeof(long)];       /* aligned for the sockaddrs */
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    char *p;
    int s, r;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = type;
    h->rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    h->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    h->rtm_seq = 1;
    p = (char *)(h + 1);
    if (family == AF_INET6) {
        struct sockaddr_in6 *sa;
        int i;

        sa = (struct sockaddr_in6 *)p; sa6(sa, dst, 0); p += ROUNDUP(sizeof(*sa));
        sa = (struct sockaddr_in6 *)p; sa6(sa, GW6, 0); p += ROUNDUP(sizeof(*sa));
        sa = (struct sockaddr_in6 *)p; sa6(sa, NULL, 0);
        for (i = 0; i < plen / 8; i++)
            sa->sin6_addr.s6_addr[i] = 0xff;
        p += ROUNDUP(sizeof(*sa));
    } else {
        struct sockaddr_in *sa;

        sa = (struct sockaddr_in *)p; sa4(sa, addr4(dst), 0); p += ROUNDUP(sizeof(*sa));
        sa = (struct sockaddr_in *)p; sa4(sa, gw4, 0); p += ROUNDUP(sizeof(*sa));
        sa = (struct sockaddr_in *)p; sa4(sa, addr4("255.255.255.0"), 0);
        p += ROUNDUP(sizeof(*sa));
    }
    h->rtm_msglen = p - (char *)buf;
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* A Packet Too Big reporting mtu, quoting a UDP datagram from ::1 port
 * sport to dst, sent to ::1. */
static int send_ptb(const char *dst, unsigned short sport, unsigned mtu)
{
    unsigned char pkt[8 + 40 + 8];
    struct icmp6_hdr *h = (struct icmp6_hdr *)pkt;
    struct ip6_hdr *ip6 = (struct ip6_hdr *)(pkt + 8);
    unsigned char *uh = pkt + 8 + 40;
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), r;

    if (s < 0)
        return -1;
    bzero(pkt, sizeof(pkt));
    h->icmp6_type = ICMP6_PACKET_TOO_BIG;
    h->icmp6_mtu = htonl(mtu);
    ip6->ip6_vfc = IPV6_VERSION;
    ip6->ip6_plen = htons(8);
    ip6->ip6_nxt = IPPROTO_UDP;
    ip6->ip6_hlim = 64;
    ip6->ip6_src = lo;
    inet_pton(AF_INET6, (char *)dst, &ip6->ip6_dst);
    uh[0] = sport >> 8; uh[1] = sport & 0xff;
    uh[3] = 9;                          /* to the discard port */
    sa6(&d, NULL, 0);
    d.sin6_addr = lo;
    r = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(s);
    Delay(10);                          /* let the stack take it in */
    return r;
}

static unsigned short cksum(const unsigned char *p, int len)
{
    unsigned long sum = 0;

    for (; len > 1; p += 2, len -= 2)
        sum += (p[0] << 8) | p[1];
    if (len)
        sum += p[0] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~sum & 0xffff;
}

/* An ICMP "fragmentation needed" reporting mtu, quoting a datagram from us
 * to dst, sent to 127.0.0.1. */
static int send_needfrag(const char *dst, unsigned mtu)
{
    unsigned char pkt[8 + 20 + 8];
    unsigned char *q = pkt + 8;
    struct in_addr d = addr4(dst);
    struct sockaddr_in to;
    unsigned short c;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP), r;

    if (s < 0)
        return -1;
    bzero(pkt, sizeof(pkt));
    pkt[0] = ICMP_UNREACH;
    pkt[1] = ICMP_UNREACH_NEEDFRAG;
    pkt[6] = mtu >> 8;
    pkt[7] = mtu & 0xff;
    q[0] = 0x45;                        /* the quoted IPv4 header */
    q[2] = 1500 >> 8; q[3] = 1500 & 0xff;
    q[6] = 0x40;                        /* DF */
    q[8] = 64;
    q[9] = IPPROTO_UDP;
    memcpy(q + 12, &ours, 4);
    memcpy(q + 16, &d, 4);
    c = cksum(q, 20);
    q[10] = c >> 8; q[11] = c & 0xff;
    c = cksum(pkt, sizeof(pkt));
    pkt[2] = c >> 8; pkt[3] = c & 0xff;
    sa4(&to, addr4("127.0.0.1"), 0);
    r = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&to, sizeof(to));
    CloseSocket(s);
    Delay(10);
    return r;
}

static int geti(int s, int level, int opt)
{
    int v = -99;
    socklen_t l = sizeof(v);

    if (getsockopt(s, level, opt, &v, &l) != 0)
        return -98;
    return v;
}

static int seti(int s, int level, int opt, int v)
{
    return setsockopt(s, level, opt, &v, sizeof(v));
}

/* A UDP socket bound to the wildcard address; *port is the one it got. */
static int bound6(unsigned short *port)
{
    struct sockaddr_in6 a;
    socklen_t l = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    sa6(&a, "::", 0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            getsockname(s, (struct sockaddr *)&a, &l) != 0) {
        CloseSocket(s);
        return -1;
    }
    *port = ntohs(a.sin6_port);
    return s;
}

/* An IPV6_PATHMTU message waiting on s, within secs: 1 and *mi if so. */
static int pathmtu_msg(int s, int secs, struct ip6_mtuinfo *mi)
{
    char data[16], cbuf[128];
    struct sockaddr_in6 from;
    struct iovec iov;
    struct msghdr mh;
    struct cmsghdr *c;

    if (sock_wait(s, 0, secs) <= 0)
        return 0;
    bzero(&mh, sizeof(mh));
    iov.iov_base = data;
    iov.iov_len = sizeof(data);
    mh.msg_name = &from;
    mh.msg_namelen = sizeof(from);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = cbuf;
    mh.msg_controllen = sizeof(cbuf);
    if (recvmsg(s, &mh, 0) != 0)        /* the message has no data */
        return 0;
    for (c = CMSG_FIRSTHDR(&mh); c; c = CMSG_NXTHDR(&mh, c))
        if (c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_PATHMTU &&
                c->cmsg_len >= CMSG_LEN(sizeof(*mi))) {
            memcpy(mi, CMSG_DATA(c), sizeof(*mi));
            return 1;
        }
    return 0;
}

/* ----------------------------------------------------------------- tests */

int init_suite(void)
{
    if (SocketBase == NULL)
        return -1;
    if (ready6 && route(RTM_ADD, AF_INET6, NET6, 64) != 0)
        ready6 = 0;
    if (ready4 && route(RTM_ADD, AF_INET, NET4, 24) != 0)
        ready4 = 0;
    return 0;
}

int clean_suite(void)
{
    if (ready6)
        route(RTM_DELETE, AF_INET6, NET6, 64);
    if (ready4)
        route(RTM_DELETE, AF_INET, NET4, 24);
    return 0;
}

/* The options and their defaults (RFC 3542 sections 11.1-11.4). */
void testUDPPMTU_V6_OPTIONS(void)
{
    struct ip6_mtuinfo mi;
    socklen_t l = sizeof(mi);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU), -1);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_DONTFRAG), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_RECVPATHMTU), 0);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, 1), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU), 1);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, 0), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU), 0);
    CU_ASSERT_NOT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, 2), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU), 0);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_DONTFRAG, 1), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_DONTFRAG), 1);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_RECVPATHMTU, 1), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IPV6, IPV6_RECVPATHMTU), 1);
    /* IPV6_PATHMTU needs a peer */
    CU_ASSERT_NOT_EQUAL(getsockopt(s, IPPROTO_IPV6, IPV6_PATHMTU, &mi, &l), 0);
    CU_ASSERT_EQUAL(Errno(), ENOTCONN);
    CloseSocket(s);
}

/* IPV6_PATHMTU: the path MTU a Packet Too Big left, for that peer only. */
void testUDPPMTU_V6_PATHMTU(void)
{
    struct sockaddr_in6 d;
    struct ip6_mtuinfo mi;
    struct in6_addr a;
    socklen_t l = sizeof(mi);
    int s, t;
    NEED6();
    CU_ASSERT(send_ptb(DST6_A, 0, 1400) > 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    t = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0 && t >= 0);
    sa6(&d, DST6_A, 9);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    sa6(&d, DST6_B, 9);
    CU_ASSERT_EQUAL(connect(t, (struct sockaddr *)&d, sizeof(d)), 0);
    bzero(&mi, sizeof(mi));
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IPV6, IPV6_PATHMTU, &mi, &l), 0);
    CU_ASSERT_EQUAL(mi.ip6m_mtu, 1400);
    inet_pton(AF_INET6, DST6_A, &a);
    CU_ASSERT(IN6_ARE_ADDR_EQUAL(&mi.ip6m_addr.sin6_addr, &a));
    l = sizeof(mi);
    CU_ASSERT_EQUAL(getsockopt(t, IPPROTO_IPV6, IPV6_PATHMTU, &mi, &l), 0);
    CU_ASSERT(mi.ip6m_mtu != 1400 && mi.ip6m_mtu >= IPV6_MMTU);
    CloseSocket(s);
    CloseSocket(t);
}

/* IPV6_RECVPATHMTU: the socket that sent the datagram a Packet Too Big
 * quotes, and asked, gets an IPV6_PATHMTU message; the others do not. */
void testUDPPMTU_V6_RECVPATHMTU(void)
{
    struct ip6_mtuinfo mi;
    struct in6_addr a;
    unsigned short p1, p2, p3;
    int s1 = bound6(&p1), s2 = bound6(&p2), s3 = bound6(&p3);

    CU_ASSERT_FATAL(s1 >= 0 && s2 >= 0 && s3 >= 0);
    CU_ASSERT_EQUAL(seti(s1, IPPROTO_IPV6, IPV6_RECVPATHMTU, 1), 0);
    CU_ASSERT_EQUAL(seti(s2, IPPROTO_IPV6, IPV6_RECVPATHMTU, 1), 0);
    CU_ASSERT(send_ptb(DST6_C, p1, 1300) > 0);
    CU_ASSERT(send_ptb(DST6_C, p3, 1290) > 0);     /* s3 did not ask */
    bzero(&mi, sizeof(mi));
    CU_ASSERT(pathmtu_msg(s1, 2, &mi));
    CU_ASSERT_EQUAL(mi.ip6m_mtu, 1300);
    inet_pton(AF_INET6, DST6_C, &a);
    CU_ASSERT(IN6_ARE_ADDR_EQUAL(&mi.ip6m_addr.sin6_addr, &a));
    CU_ASSERT(!pathmtu_msg(s2, 1, &mi));
    CU_ASSERT(!pathmtu_msg(s3, 1, &mi));
    CloseSocket(s1);
    CloseSocket(s2);
    CloseSocket(s3);
}

/* IPV6_DONTFRAG, as an option or ancillary data: a datagram above the
 * link MTU is not fragmented but refused with EMSGSIZE, and the socket
 * gets an IPV6_PATHMTU message if it asked (section 11.2). */
void testUDPPMTU_V6_DONTFRAG(void)
{
    static char big[1600];
    char cbuf[CMSG_SPACE(sizeof(int))];
    struct sockaddr_in6 d;
    struct ip6_mtuinfo mi;
    struct iovec iov;
    struct msghdr mh;
    struct cmsghdr *c;
    int s, t, on = 1;
    NEED6();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    t = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0 && t >= 0);
    sa6(&d, DST6_B, 9);
    /* without it, fragmented */
    CU_ASSERT_EQUAL(sendto(t, big, sizeof(big), 0, (struct sockaddr *)&d, sizeof(d)), sizeof(big));
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_DONTFRAG, 1), 0);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_RECVPATHMTU, 1), 0);
    CU_ASSERT_EQUAL(sendto(s, big, sizeof(big), 0, (struct sockaddr *)&d, sizeof(d)), -1);
    CU_ASSERT_EQUAL(Errno(), EMSGSIZE);
    bzero(&mi, sizeof(mi));
    CU_ASSERT(pathmtu_msg(s, 2, &mi));
    CU_ASSERT(mi.ip6m_mtu >= IPV6_MMTU && mi.ip6m_mtu < sizeof(big));
    /* a datagram that fits leaves */
    CU_ASSERT_EQUAL(sendto(s, big, 1000, 0, (struct sockaddr *)&d, sizeof(d)), 1000);
    /* ancillary data, on a socket without the option */
    bzero(&mh, sizeof(mh));
    iov.iov_base = big;
    iov.iov_len = sizeof(big);
    mh.msg_name = &d;
    mh.msg_namelen = sizeof(d);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = cbuf;
    mh.msg_controllen = sizeof(cbuf);
    c = CMSG_FIRSTHDR(&mh);
    c->cmsg_level = IPPROTO_IPV6;
    c->cmsg_type = IPV6_DONTFRAG;
    c->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c), &on, sizeof(on));
    CU_ASSERT_EQUAL(sendmsg(t, &mh, 0), -1);
    CU_ASSERT_EQUAL(Errno(), EMSGSIZE);
    CloseSocket(s);
    CloseSocket(t);
}

/* How the next 1400-byte datagram to ff02::1 left the interface: 1 whole,
 * 2 fragmented, 0 not seen. */
static int mcast_sent(int s, unsigned short port)
{
    static char data[1400];
    char buf[BPF_DFLTBUFSIZE];
    struct sockaddr_in6 d;
    int t, how = 0;

    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    sa6(&d, "ff02::1", port);
    d.sin6_scope_id = ifindex;
    if (sendto(s, data, sizeof(data), 0, (struct sockaddr *)&d, sizeof(d)) != sizeof(data))
        return 0;
    for (t = 0; t < 20 && !how; t++) {
        long got;

        while (!how && (got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (!how && p < buf + got) {
                const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen;
                const unsigned char *ip = fr + 14;

                if (bh->bh_caplen >= 14 + 40 + 12 && fr[12] == 0x86 && fr[13] == 0xdd &&
                        ip[24] == 0xff && ip[25] == 0x02 && ip[39] == 0x01) {
                    if (ip[6] == IPPROTO_UDP && ((ip[42] << 8) | ip[43]) == port)
                        how = 1;
                    else if (ip[6] == 44 && ip[40] == IPPROTO_UDP &&
                            ((ip[48 + 2] << 8) | ip[48 + 3]) == port)
                        how = 2;            /* the first fragment */
                }
                p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
            }
        }
        Delay(5);
    }
    return how;
}

/* IPV6_USE_MIN_MTU: -1 (the default) sends multicast at 1280, 0 at the
 * link MTU; IPV6_DONTFRAG sends it whole (section 11.1). */
void testUDPPMTU_V6_MINMTU(void)
{
    int s;
    NEED6();
    if (bpf < 0) {
        CU_PASS("no BPF - skipping");
        return;
    }
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)), 0);
    CU_ASSERT_EQUAL(mcast_sent(s, 4441), 2);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, 0), 0);
    CU_ASSERT_EQUAL(mcast_sent(s, 4442), 1);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, -1), 0);
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IPV6, IPV6_DONTFRAG, 1), 0);
    CU_ASSERT_EQUAL(mcast_sent(s, 4443), 1);
    CloseSocket(s);
}

/* IP_DONTFRAG: DF on, and EMSGSIZE above the path MTU instead of
 * fragments. */
void testUDPPMTU_V4_DONTFRAG(void)
{
    static char big[1450];
    char buf[BPF_DFLTBUFSIZE];
    struct sockaddr_in d;
    int s, t, seen = 0, df = 0;
    NEED4();
    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IP, IP_DONTFRAG), 0);
    CU_ASSERT(send_needfrag(DST4_A, 1400) > 0);
    sa4(&d, addr4(DST4_A), 9);
    CU_ASSERT_EQUAL(sendto(s, big, sizeof(big), 0, (struct sockaddr *)&d, sizeof(d)), sizeof(big));
    CU_ASSERT_EQUAL(seti(s, IPPROTO_IP, IP_DONTFRAG, 1), 0);
    CU_ASSERT_EQUAL(geti(s, IPPROTO_IP, IP_DONTFRAG), 1);
    CU_ASSERT_EQUAL(sendto(s, big, sizeof(big), 0, (struct sockaddr *)&d, sizeof(d)), -1);
    CU_ASSERT_EQUAL(Errno(), EMSGSIZE);
    CU_ASSERT_EQUAL(sendto(s, big, 1000, 0, (struct sockaddr *)&d, sizeof(d)), 1000);

    /* DF on the wire, to the neighbour */
    if (bpf < 0) {
        CloseSocket(s);
        return;
    }
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    sa4(&d, neighbour, 4444);
    CU_ASSERT_EQUAL(sendto(s, big, 100, 0, (struct sockaddr *)&d, sizeof(d)), 100);
    for (t = 0; t < 30 && !seen; t++) {
        long got;

        while (!seen && (got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (!seen && p < buf + got) {
                const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen;

                if (bh->bh_caplen >= 14 + 20 + 8 && fr[12] == 0x08 && fr[13] == 0x00 &&
                        fr[14 + 9] == IPPROTO_UDP && memcmp(fr + 14 + 16, &neighbour, 4) == 0 &&
                        ((fr[14 + 22] << 8) | fr[14 + 23]) == 4444) {
                    seen = 1;
                    df = (fr[14 + 6] & 0x40) != 0;
                }
                p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
            }
        }
        Delay(5);
    }
    CloseSocket(s);
    if (!seen) {
        CU_PASS("no datagram seen (the neighbour did not answer ARP) - skipping");
        return;
    }
    CU_ASSERT(df);
}

static void setup(void)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    struct ifreq ifr;
    u_int one = 1;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s >= 0) {
        bzero(&ifc, sizeof(ifc));
        ifc.ifc6_len = sizeof(req);
        ifc.ifc6_req = req;
        if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
            int i, n = ifc.ifc6_len / sizeof(req[0]);

            for (i = 0; i < n && !ready6; i++)
                if (strcmp(req[i].ifr_name, ifname) == 0 &&
                        IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr))
                    ready6 = 1;
        }
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        if (ready6 && IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
            ifindex = ifr.ifr_index;
        else
            ready6 = 0;
        CloseSocket(s);
    }
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s >= 0) {
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        if (IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0 &&
                ((struct sockaddr_in *)&ifr.ifr_addr)->sin_family == AF_INET) {
            ours = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
            gw4 = neighbour = ours;
            ((unsigned char *)&gw4)[3] = 77;
            ((unsigned char *)&neighbour)[3] = 1;
            ready4 = ours.s_addr != 0 && gw4.s_addr != ours.s_addr &&
                     neighbour.s_addr != ours.s_addr;
        }
        CloseSocket(s);
    }
    if (!ready6 && !ready4)
        return;
    bpf = bpf_open(0);
    if (bpf >= 0 && (bpf_ioctl(bpf, BIOCSETIF, (char *)ifname) != 0 ||
            bpf_ioctl(bpf, BIOCIMMEDIATE, (char *)&one) != 0)) {
        bpf_close(bpf);
        bpf = -1;
    }
    if (bpf >= 0) {
        char drain[BPF_DFLTBUFSIZE];

        while (bpf_read(bpf, drain, sizeof(drain)) > 0)
            ;
    }
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping UDPPMTU tests\n");
        return 0;
    }
    setup();
    if (!ready6)
        printf("%s: no interface with a link-local address - skipping its IPv6 tests\n", ifname);
    if (!ready4)
        printf("%s: no IPv4 address - skipping its IPv4 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("UDPPMTU_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the IPv6 path MTU options and their defaults", testUDPPMTU_V6_OPTIONS) ||
        !CU_add_test(p, "IPV6_PATHMTU gives the peer's path MTU", testUDPPMTU_V6_PATHMTU) ||
        !CU_add_test(p, "IPV6_RECVPATHMTU tells the sender of a Packet Too Big", testUDPPMTU_V6_RECVPATHMTU) ||
        !CU_add_test(p, "IPV6_DONTFRAG refuses a datagram above the link MTU", testUDPPMTU_V6_DONTFRAG) ||
        !CU_add_test(p, "IPV6_USE_MIN_MTU and IPV6_DONTFRAG for multicast", testUDPPMTU_V6_MINMTU) ||
        !CU_add_test(p, "IP_DONTFRAG sets DF and refuses above the path MTU", testUDPPMTU_V4_DONTFRAG))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-UDPPMTU");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
