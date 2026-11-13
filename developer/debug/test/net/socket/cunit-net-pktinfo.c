/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IP_PKTINFO (Linux's struct in_pktinfo, with AROS's
    option number). Received: the interface a datagram came in by, the
    header's destination, and the local address a reply would come from -
    the same facts IP_RECVIF and IP_RECVDSTADDR give. Sent, as ancillary
    data: the interface the datagram leaves by, whatever the routing table
    and IP_MULTICAST_IF say, and its source - what a responder with one
    socket for every link (mDNS, RFC 6762 section 15.2) needs to answer on
    the link a question came in by, from the address asked about.
    Usage:  cunit-net-pktinfo [IFNAME]  (default eth0; the tests that send
    by an interface are skipped without an IPv4 address on it, those that
    need a second link without one on eth1 - or eth0 when IFNAME is eth1).
    A host route through IFNAME's .1 (the tap's) leads to .77 of the second
    link's network, an address nobody has: what ARP asks for on that link
    tells which way a datagram went.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/route.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static struct in_addr ifaddr, ifaddr2;  /* IFNAME's address, the other link's */
static unsigned ifindex, ifindex2;

#define GROUP "239.255.77.1"

#define NEED_IF() do { if (ifaddr.s_addr == INADDR_ANY) { CU_PASS("no IPv4 address on the interface - skipping"); return; } } while (0)
#define NEED_TWO() do { if (ifaddr.s_addr == INADDR_ANY || ifaddr2.s_addr == INADDR_ANY) { CU_PASS("no second interface with an IPv4 address - skipping"); return; } } while (0)

/* A control buffer aligned for struct cmsghdr. */
typedef union { struct cmsghdr h; char b[256]; } ctlbuf;

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

static void sin_set(struct sockaddr_in *sin, struct in_addr a, unsigned short nport)
{
    bzero(sin, sizeof(*sin));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr = a;
    sin->sin_port = nport;
}

static struct in_addr addr_of(const char *s)
{
    struct in_addr a;

    a.s_addr = inet_addr((char *)s);
    return a;
}

/* A UDP socket bound to any address; its port in *nport. */
static int receiver(unsigned short *nport)
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int on = 1, s = socket(AF_INET, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    sin_set(&sin, addr_of("0.0.0.0"), 0);
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) != 0 ||
        getsockname(s, (struct sockaddr *)&sin, &len) != 0 ||
        setsockopt(s, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) != 0) {
        CloseSocket(s);
        return -1;
    }
    *nport = sin.sin_port;
    return s;
}

/* recvmsg() one datagram: its sender in *from, its in_pktinfo in *pi
 * (0 if none came). The length, or -1 if nothing came in 3 s. */
static int recv_pi(int fd, struct sockaddr_in *from, struct in_pktinfo *pi, int *got)
{
    static char data[32];
    struct iovec iov;
    struct msghdr mh;
    struct cmsghdr *c;
    ctlbuf ctl;
    int n;

    *got = 0;
    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    bzero(pi, sizeof(*pi));
    iov.iov_base = data;
    iov.iov_len = sizeof(data);
    mh.msg_name = from;
    mh.msg_namelen = sizeof(*from);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = ctl.b;
    mh.msg_controllen = sizeof(ctl);
    if (fd < 0 || sock_wait(fd, 0, 3) <= 0)
        return -1;
    n = recvmsg(fd, &mh, 0);
    for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c))
        if (c->cmsg_level == IPPROTO_IP && c->cmsg_type == IP_PKTINFO &&
            c->cmsg_len == CMSG_LEN(sizeof(*pi))) {
            memcpy(pi, CMSG_DATA(c), sizeof(*pi));
            *got = 1;
        }
    return n;
}

/* sendmsg() "pi" to dst with an IP_PKTINFO of oif and spec_dst; with
 * oif -1, no ancillary data. 2, or -1 and Errno(). */
static int send_pi(int s, struct sockaddr_in *dst, int oif, struct in_addr spec_dst)
{
    struct iovec iov;
    struct msghdr mh;
    ctlbuf ctl;

    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    iov.iov_base = "pi";
    iov.iov_len = 2;
    mh.msg_name = dst;
    mh.msg_namelen = dst ? sizeof(*dst) : 0;
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    if (oif >= 0) {
        struct cmsghdr *c;
        struct in_pktinfo pi;

        mh.msg_control = ctl.b;
        mh.msg_controllen = CMSG_SPACE(sizeof(pi));
        c = CMSG_FIRSTHDR(&mh);
        c->cmsg_level = IPPROTO_IP;
        c->cmsg_type = IP_PKTINFO;
        c->cmsg_len = CMSG_LEN(sizeof(pi));
        bzero(&pi, sizeof(pi));
        pi.ipi_ifindex = oif;
        pi.ipi_spec_dst = spec_dst;
        memcpy(CMSG_DATA(c), &pi, sizeof(pi));
    }
    return sendmsg(s, &mh, 0);
}

static unsigned index_of(const char *name)
{
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    unsigned idx = 0;

    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (s >= 0 && IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
        idx = ifr.ifr_index;
    if (s >= 0)
        CloseSocket(s);
    return idx;
}

static struct in_addr ifaddr_of(const char *name)
{
    struct ifreq ifr;
    struct in_addr a;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    a.s_addr = INADDR_ANY;
    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (s >= 0 && IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0)
        a = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    if (s >= 0)
        CloseSocket(s);
    return a;
}

/* Join (1) or leave GROUP on the interface whose address is a. */
static int group(int s, int join, struct in_addr a)
{
    struct ip_mreq mr;

    mr.imr_multiaddr = addr_of(GROUP);
    mr.imr_interface = a;
    return setsockopt(s, IPPROTO_IP, join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
                      &mr, sizeof(mr));
}

/* Delete (del 1) or look up the ARP entry for a; 1 if there is one,
 * resolved or not. */
static int arpent(int del, struct in_addr a)
{
    struct arpreq ar;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return 0;
    bzero(&ar, sizeof(ar));
    sin_set((struct sockaddr_in *)&ar.arp_pa, a, 0);
    r = IoctlSocket(s, del ? SIOCDARP : SIOCGARP, (char *)&ar);
    CloseSocket(s);
    return r == 0 && !del;
}

/* SIOCADDRT (add 1) or SIOCDELRT: a host route to dst through gw */
static int hostroute(int add, struct in_addr dst, struct in_addr gw)
{
    struct ortentry rt;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&rt, sizeof(rt));
    sin_set((struct sockaddr_in *)&rt.rt_dst, dst, 0);
    sin_set((struct sockaddr_in *)&rt.rt_gateway, gw, 0);
    rt.rt_flags = RTF_UP | RTF_HOST | RTF_GATEWAY;
    r = IoctlSocket(s, add ? SIOCADDRT : SIOCDELRT, (char *)&rt);
    CloseSocket(s);
    return r;
}

/* The option: off by default, an int, IP_RECVPKTINFO its other name. */
void testPKTINFO_OPTION(void)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), on = 1, off = 0, v = -1;
    socklen_t l = sizeof(v);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(IP_RECVPKTINFO, IP_PKTINFO);
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IP, IP_PKTINFO, &v, &l), 0);
    CU_ASSERT_EQUAL(v, 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_RECVPKTINFO, &on, sizeof(on)), 0);
    l = sizeof(v);
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IP, IP_PKTINFO, &v, &l), 0);
    CU_ASSERT_EQUAL(v, 1);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_PKTINFO, &off, sizeof(off)), 0);
    l = sizeof(v);
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IP, IP_PKTINFO, &v, &l), 0);
    CU_ASSERT_EQUAL(v, 0);
    CloseSocket(s);
}

/* Over the loopback: lo0, and 127.0.0.1 as destination and reply address;
 * IP_RECVIF and IP_RECVDSTADDR, asked for too, say the same. */
void testPKTINFO_RECV(void)
{
    struct sockaddr_in to, from;
    struct in_pktinfo pi;
    unsigned short port;
    int r = receiver(&port), s = socket(AF_INET, SOCK_DGRAM, 0), got;

    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    sin_set(&to, addr_of("127.0.0.1"), port);
    CU_ASSERT_EQUAL(sendto(s, "pi", 2, 0, (struct sockaddr *)&to, sizeof(to)), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT(got);
    CU_ASSERT_EQUAL(pi.ipi_ifindex, (int)index_of("lo0"));
    CU_ASSERT_EQUAL(pi.ipi_addr.s_addr, to.sin_addr.s_addr);
    CU_ASSERT_EQUAL(pi.ipi_spec_dst.s_addr, to.sin_addr.s_addr);
    CloseSocket(s);
    CloseSocket(r);
}

/* A group datagram, looped back on IFNAME: the group as destination, the
 * interface's address as the one to reply from. */
void testPKTINFO_RECV_GROUP(void)
{
    struct sockaddr_in to, from;
    struct in_pktinfo pi;
    unsigned short port;
    int r, s, got;
    u_char loop = 1;

    NEED_IF();
    r = receiver(&port);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(group(r, 1, ifaddr), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr)), 0);
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    sin_set(&to, addr_of(GROUP), port);
    CU_ASSERT_EQUAL(sendto(s, "pi", 2, 0, (struct sockaddr *)&to, sizeof(to)), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT(got);
    CU_ASSERT_EQUAL(pi.ipi_ifindex, (int)ifindex);
    CU_ASSERT_EQUAL(pi.ipi_addr.s_addr, to.sin_addr.s_addr);
    CU_ASSERT_EQUAL(pi.ipi_spec_dst.s_addr, ifaddr.s_addr);
    group(r, 0, ifaddr);
    CloseSocket(s);
    CloseSocket(r);
}

/* ipi_spec_dst is the source: on an unbound socket, and over the address
 * a socket is bound to; a connected socket keeps its connection's. */
void testPKTINFO_SOURCE(void)
{
    struct sockaddr_in to, from, me;
    struct in_pktinfo pi;
    unsigned short port;
    int r, s, got;

    NEED_IF();
    r = receiver(&port);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    sin_set(&to, addr_of("127.0.0.1"), port);
    CU_ASSERT_EQUAL(send_pi(s, &to, 0, ifaddr), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT_EQUAL(from.sin_addr.s_addr, ifaddr.s_addr);
    CloseSocket(s);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    sin_set(&me, addr_of("127.0.0.1"), 0);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&me, sizeof(me)), 0);
    CU_ASSERT_EQUAL(send_pi(s, &to, 0, ifaddr), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT_EQUAL(from.sin_addr.s_addr, ifaddr.s_addr);
    CloseSocket(s);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&to, sizeof(to)), 0);
    CU_ASSERT_EQUAL(send_pi(s, NULL, 0, ifaddr), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT_EQUAL(from.sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    CloseSocket(s);
    CloseSocket(r);
}

/* What is refused: a source that is not ours, an interface that does not
 * exist, another than the one the socket is bound to, a short message. */
void testPKTINFO_REFUSED(void)
{
    struct sockaddr_in to;
    struct in_addr none;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    none.s_addr = INADDR_ANY;
    sin_set(&to, addr_of("127.0.0.1"), htons(9));
    CU_ASSERT_EQUAL(send_pi(s, &to, 0, addr_of("10.99.99.99")), -1);
    CU_ASSERT_EQUAL(Errno(), EADDRNOTAVAIL);
    CU_ASSERT_EQUAL(send_pi(s, &to, 9999, none), -1);
    CU_ASSERT_EQUAL(Errno(), ENXIO);
    if (ifindex != 0) {
        CU_ASSERT_EQUAL(setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (char *)"lo0", 3), 0);
            CU_ASSERT_EQUAL(send_pi(s, &to, ifindex, none), -1);
        CU_ASSERT_EQUAL(Errno(), EINVAL);
        CU_ASSERT_EQUAL(send_pi(s, &to, index_of("lo0"), none), 2);
    }
    CloseSocket(s);

    /* a struct of another size */
    {
        struct iovec iov;
        struct msghdr mh;
        struct cmsghdr *c;
        ctlbuf ctl;

        s = socket(AF_INET, SOCK_DGRAM, 0);
        bzero(&mh, sizeof(mh));
        bzero(&ctl, sizeof(ctl));
        iov.iov_base = "pi";
        iov.iov_len = 2;
        mh.msg_name = &to;
        mh.msg_namelen = sizeof(to);
        mh.msg_iov = &iov;
        mh.msg_iovlen = 1;
        mh.msg_control = ctl.b;
        mh.msg_controllen = CMSG_SPACE(sizeof(int));
        c = CMSG_FIRSTHDR(&mh);
        c->cmsg_level = IPPROTO_IP;
        c->cmsg_type = IP_PKTINFO;
        c->cmsg_len = CMSG_LEN(sizeof(int));
            CU_ASSERT_EQUAL(sendmsg(s, &mh, 0), -1);
        CU_ASSERT_EQUAL(Errno(), EINVAL);
        CloseSocket(s);
    }
}

/* A group datagram named for the second link leaves there, from that
 * link's address, though IP_MULTICAST_IF says IFNAME: the receiver,
 * joined on both, gets it once, by the second link. */
void testPKTINFO_GROUP_IF(void)
{
    struct sockaddr_in to, from;
    struct in_pktinfo pi;
    struct in_addr none;
    unsigned short port;
    int r, s, got;
    u_char loop = 1;

    NEED_TWO();
    none.s_addr = INADDR_ANY;
    r = receiver(&port);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(group(r, 1, ifaddr), 0);
    CU_ASSERT_EQUAL(group(r, 1, ifaddr2), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr)), 0);
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    sin_set(&to, addr_of(GROUP), port);

    /* without: IP_MULTICAST_IF's */
    CU_ASSERT_EQUAL(send_pi(s, &to, -1, none), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    CU_ASSERT_EQUAL(pi.ipi_ifindex, (int)ifindex);
    CU_ASSERT_EQUAL(from.sin_addr.s_addr, ifaddr.s_addr);

    CU_ASSERT_EQUAL(send_pi(s, &to, ifindex2, none), 2);
    CU_ASSERT_EQUAL(recv_pi(r, &from, &pi, &got), 2);
    printf("\n    named for index %u: came in by %d from %s\n", ifindex2,
           pi.ipi_ifindex, inet_ntoa(from.sin_addr));
    CU_ASSERT_EQUAL(pi.ipi_ifindex, (int)ifindex2);
    CU_ASSERT_EQUAL(from.sin_addr.s_addr, ifaddr2.s_addr);
    CU_ASSERT_EQUAL(sock_wait(r, 0, 1), 0);    /* no second copy */

    group(r, 0, ifaddr);
    group(r, 0, ifaddr2);
    CloseSocket(s);
    CloseSocket(r);
}

/*
 * The route to an address of the second link's network goes through
 * IFNAME's host (.1), as a host route; a datagram named for the second
 * link goes there directly - ARP asks for the address on that link, and
 * an entry for it is made, unresolved: nobody has .77. The route alone
 * makes none (it resolves the gateway). An address nobody has, so that
 * nothing the host sends makes the entry instead.
 */
void testPKTINFO_ROUTE_IF(void)
{
    struct sockaddr_in to, me;
    struct in_addr host2, gw, none;
    int s, f0, f1;

    NEED_TWO();
    none.s_addr = INADDR_ANY;
    host2.s_addr = (ifaddr2.s_addr & htonl(0xffffff00)) | htonl(77);
    gw.s_addr = (ifaddr.s_addr & htonl(0xffffff00)) | htonl(1);
    CU_ASSERT_EQUAL_FATAL(hostroute(1, host2, gw), 0);
    sin_set(&to, host2, htons(9));

    s = socket(AF_INET, SOCK_DGRAM, 0);
    sin_set(&me, none, 0);
    bind(s, (struct sockaddr *)&me, sizeof(me));
    arpent(1, host2);
    CU_ASSERT_EQUAL(send_pi(s, &to, -1, none), 2);
    Delay(TICKS_PER_SECOND);
    f0 = arpent(0, host2);
    arpent(1, host2);
    CU_ASSERT_EQUAL(send_pi(s, &to, ifindex2, none), 2);
    Delay(TICKS_PER_SECOND);
    f1 = arpent(0, host2);
    printf("\n    to %s: by the route %s, named for index %u %s\n", inet_ntoa(host2),
           f0 ? "asked for on its link" : "through the gateway", ifindex2,
           f1 ? "asked for on its link" : "nowhere");
    CU_ASSERT(!f0);
    CU_ASSERT(f1);
    CloseSocket(s);
    hostroute(0, host2, gw);
}

/*
 * IP_PKTINFO is per datagram: one socket alternates, named for the second
 * link, then not, then named again. Each datagram goes its own way (on the
 * link, through the gateway, on the link), and the socket keeps no address.
 */
void testPKTINFO_ALTERNATE(void)
{
    struct sockaddr_in to, me;
    struct in_addr host2, gw, none;
    socklen_t len;
    int s, f[3], i;

    NEED_TWO();
    none.s_addr = INADDR_ANY;
    host2.s_addr = (ifaddr2.s_addr & htonl(0xffffff00)) | htonl(77);
    gw.s_addr = (ifaddr.s_addr & htonl(0xffffff00)) | htonl(1);
    CU_ASSERT_EQUAL_FATAL(hostroute(1, host2, gw), 0);
    sin_set(&to, host2, htons(9));
    s = socket(AF_INET, SOCK_DGRAM, 0);
    for (i = 0; i < 3; i++) {
        arpent(1, host2);
        CU_ASSERT_EQUAL(send_pi(s, &to, i == 1 ? -1 : (int)ifindex2, none), 2);
        Delay(TICKS_PER_SECOND);
        f[i] = arpent(0, host2);
    }
    printf("\n    named, not, named: %s, %s, %s\n", f[0] ? "asked for on its link" : "not on it",
           f[1] ? "asked for on its link" : "through the gateway", f[2] ? "asked for on its link" : "not on it");
    CU_ASSERT(f[0] && !f[1] && f[2]);
    len = sizeof(me);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&me, &len), 0);
    CU_ASSERT_EQUAL(me.sin_addr.s_addr, INADDR_ANY);
    CloseSocket(s);
    hostroute(0, host2, gw);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    const char *other;

    if (argc > 1)
        ifname = argv[1];
    other = strcmp(ifname, "eth1") ? "eth1" : "eth0";
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping IP_PKTINFO tests\n");
        return 0;
    }
    if (st_skip_program("IP_PKTINFO", ST_V4)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    ifaddr = ifaddr_of(ifname);
    ifindex = index_of(ifname);
    ifaddr2 = ifaddr_of(other);
    ifindex2 = index_of(other);
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PKTINFO_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the option", testPKTINFO_OPTION) ||
        !CU_add_test(p, "received over the loopback", testPKTINFO_RECV) ||
        !CU_add_test(p, "received for a group", testPKTINFO_RECV_GROUP) ||
        !CU_add_test(p, "the source sent with", testPKTINFO_SOURCE) ||
        !CU_add_test(p, "what is refused", testPKTINFO_REFUSED) ||
        !CU_add_test(p, "a group datagram leaves by the interface named", testPKTINFO_GROUP_IF) ||
        !CU_add_test(p, "a datagram leaves by the interface named, not the route's", testPKTINFO_ROUTE_IF) ||
        !CU_add_test(p, "per datagram: named, not, named again", testPKTINFO_ALTERNATE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-PKTINFO");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
