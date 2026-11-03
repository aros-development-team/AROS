/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv6 multicast on two interfaces (RFC 3493 section
    5.2, RFC 3542 section 6): a group datagram leaves by the interface its
    destination's zone names, from that interface's link-local address,
    and its looped-back copy is received there once - with IPV6_PKTINFO
    naming that interface - by a socket that joined the group there, not by
    one that joined it only on the other interface. A socket connected to
    a group with no zone sends by its IPV6_MULTICAST_IF interface, from
    that interface's address.

    The datagrams are the stack's own looped-back copies, so nothing is
    needed on the links but two interfaces with link-local addresses (on
    hosted AROS: AROS_TAP2_IP, a second tap.device unit).
    Usage:  cunit-net-mcast6if [IFNAME1 IFNAME2]  (default eth0 eth1).
    Without two such interfaces the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname[2] = { "eth0", "eth1" };
static unsigned ifindex[2];
static struct in6_addr linklocal[2];
static int ready;

#define GROUP   "ff02::4d54:64"

#define NEED_IF() do { if (!ready) { CU_PASS("no two suitable interfaces - skipping"); return; } } while (0)

static int index_of(const char *name, unsigned *index)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    r = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr);
    CloseSocket(s);
    if (r != 0)
        return -1;
    *index = ifr.ifr_index;
    return 0;
}

/* The link-local address of interface i into *ll. */
static int get_linklocal(int i, struct in6_addr *ll)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), found = 0;

    if (s < 0)
        return 0;
    bzero(req, sizeof(req));
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        int k, n = ifc.ifc6_len / sizeof(req[0]);

        for (k = 0; k < n && !found; k++)
            if (strcmp(req[k].ifr_name, ifname[i]) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[k].ifr_ifru.ifru_addr.sin6_addr)) {
                *ll = req[k].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

static void sa6(struct sockaddr_in6 *a, const char *addr, unsigned short port, unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
    a->sin6_port = port;
    a->sin6_scope_id = scope;
}

/* A UDP socket on [::]:0 with IPV6_RECVPKTINFO, that joined GROUP on the
 * interfaces in mask (bit i: interface i); *port gets its port. */
static int member(int mask, unsigned short *port)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0), on = 1, i;

    if (s < 0)
        return -1;
    sa6(&a, NULL, 0, 0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            getsockname(s, (struct sockaddr *)&a, &len) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) != 0) {
        CloseSocket(s);
        return -1;
    }
    for (i = 0; i < 2; i++)
        if (mask & (1 << i)) {
            struct ipv6_mreq mr;

            bzero(&mr, sizeof(mr));
            inet_pton(AF_INET6, GROUP, &mr.ipv6mr_multiaddr);
            mr.ipv6mr_interface = ifindex[i];
            if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mr, sizeof(mr)) != 0) {
                CloseSocket(s);
                return -1;
            }
        }
    *port = a.sin6_port;
    return s;
}

/* One datagram on s within secs: its source into *from and the interface
 * IPV6_PKTINFO names into *index. 1 if one came, 0 if not. */
static int receive(int s, int secs, struct sockaddr_in6 *from, unsigned *index)
{
    long ctl[(CMSG_SPACE(sizeof(struct in6_pktinfo)) + sizeof(long) - 1) / sizeof(long)];
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    char buf[16];

    if (sock_wait(s, 0, secs) <= 0)
        return 0;
    bzero(&mh, sizeof(mh));
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    mh.msg_name = from;
    mh.msg_namelen = sizeof(*from);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = ctl;
    mh.msg_controllen = sizeof(ctl);
    if (recvmsg(s, &mh, 0) < 0)
        return 0;
    *index = 0;
    for (c = CMSG_FIRSTHDR(&mh); c; c = CMSG_NXTHDR(&mh, c))
        if (c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_PKTINFO) {
            struct in6_pktinfo pi;

            memcpy(&pi, CMSG_DATA(c), sizeof(pi));
            *index = pi.ipi6_ifindex;
        }
    return 1;
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* Sent to the group on each link: received there, from that link's
 * address, once. */
void testMCAST6IF_EACH_LINK(void)
{
    struct sockaddr_in6 d, from;
    unsigned short port;
    unsigned index;
    int r, s, i;

    NEED_IF();
    CU_ASSERT_FATAL((r = member(3, &port)) >= 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    for (i = 0; i < 2; i++) {
        sa6(&d, GROUP, port, ifindex[i]);
        CU_ASSERT_EQUAL(sendto(s, "m", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
        CU_ASSERT_FATAL(receive(r, 2, &from, &index));
        CU_ASSERT_EQUAL(index, ifindex[i]);
        CU_ASSERT(memcmp(&from.sin6_addr, &linklocal[i], sizeof(linklocal[i])) == 0);
        CU_ASSERT_EQUAL(from.sin6_scope_id, ifindex[i]);
        CU_ASSERT(!receive(r, 1, &from, &index));       /* once */
    }
    CloseSocket(s);
    CloseSocket(r);
}

/* Joined only on the first link: what leaves by the second is not
 * received. */
void testMCAST6IF_JOINED_LINK_ONLY(void)
{
    struct sockaddr_in6 d, from;
    unsigned short port;
    unsigned index;
    int r, s;

    NEED_IF();
    CU_ASSERT_FATAL((r = member(1, &port)) >= 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    sa6(&d, GROUP, port, ifindex[1]);
    CU_ASSERT_EQUAL(sendto(s, "m", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT(!receive(r, 1, &from, &index));
    sa6(&d, GROUP, port, ifindex[0]);
    CU_ASSERT_EQUAL(sendto(s, "m", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT(receive(r, 2, &from, &index));
    CloseSocket(s);
    CloseSocket(r);
}

/* Connected to the group with no zone: it sends by the IPV6_MULTICAST_IF
 * interface, from that interface's address. */
void testMCAST6IF_CONNECT_MULTICAST_IF(void)
{
    struct sockaddr_in6 d, from;
    unsigned short port;
    unsigned index;
    int r, s;

    NEED_IF();
    CU_ASSERT_FATAL((r = member(3, &port)) >= 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex[1], sizeof(ifindex[1])), 0);
    sa6(&d, GROUP, port, 0);
    CU_ASSERT_EQUAL_FATAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    CU_ASSERT_EQUAL(send(s, "m", 1, 0), 1);
    CU_ASSERT_FATAL(receive(r, 2, &from, &index));
    CU_ASSERT_EQUAL(index, ifindex[1]);
    CU_ASSERT(memcmp(&from.sin6_addr, &linklocal[1], sizeof(linklocal[1])) == 0);
    CloseSocket(s);
    CloseSocket(r);
}

/* Connected to the group in a zone: it sends there, from that address. */
void testMCAST6IF_CONNECT_ZONE(void)
{
    struct sockaddr_in6 d, from;
    unsigned short port;
    unsigned index;
    int r, s;

    NEED_IF();
    CU_ASSERT_FATAL((r = member(3, &port)) >= 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    sa6(&d, GROUP, port, ifindex[1]);
    CU_ASSERT_EQUAL_FATAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    CU_ASSERT_EQUAL(send(s, "m", 1, 0), 1);
    CU_ASSERT_FATAL(receive(r, 2, &from, &index));
    CU_ASSERT_EQUAL(index, ifindex[1]);
    CU_ASSERT(memcmp(&from.sin6_addr, &linklocal[1], sizeof(linklocal[1])) == 0);
    CloseSocket(s);
    CloseSocket(r);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int i;

    if (argc > 2) {
        ifname[0] = argv[1];
        ifname[1] = argv[2];
    }
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping MCAST6IF tests\n");
        return 0;
    }
    ready = 1;
    for (i = 0; i < 2; i++)
        if (index_of(ifname[i], &ifindex[i]) != 0 || !get_linklocal(i, &linklocal[i]))
            ready = 0;
    if (!ready)
        printf("%s, %s: not two interfaces with link-local addresses - skipping MCAST6IF tests\n",
               ifname[0], ifname[1]);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MCAST6IF_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "sent on each link, received there once", testMCAST6IF_EACH_LINK) ||
        !CU_add_test(p, "not where the group was not joined", testMCAST6IF_JOINED_LINK_ONLY) ||
        !CU_add_test(p, "connected with no zone: by IPV6_MULTICAST_IF", testMCAST6IF_CONNECT_MULTICAST_IF) ||
        !CU_add_test(p, "connected in a zone: there", testMCAST6IF_CONNECT_ZONE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-MCAST6IF");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
