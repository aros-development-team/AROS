/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for SO_BINDTODEVICE on IPv6 sockets. The interface a socket
    is bound to is its zone (RFC 4007, Readme section 6.6), as for Linux's
    sk_bound_dev_if: a link-scope peer given with no zone is on that link,
    for sendto() as for connect(), and one given with another zone is an
    error (EINVAL). It only hears what arrives on that interface, as for
    IPv4 - a TCP listener of either family included: a connection that
    arrives by another interface (the loopback) is refused. An interface no one has is refused with ENXIO, and an empty name
    removes the binding.

    The datagrams go to the interfaces' own link-local addresses and come
    back through the stack, so the source the stack chose shows. Usage:
    cunit-net-bindtodev6 [IFNAME1 IFNAME2]  (default eth0 eth1). The tests
    that need the second interface (on hosted AROS: AROS_TAP2_IP, a second
    tap.device unit) are skipped without it.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname[2] = { "eth0", "eth1" };
static unsigned ifindex[2];
static struct in6_addr linklocal[2];
static int have[2];

#define NEED_IF(n) do { if (!have[0] || ((n) > 1 && !have[1])) { CU_PASS("no suitable interfaces - skipping"); return; } } while (0)

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

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, unsigned short port,
                unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_port = port;
    a->sin6_scope_id = scope;
}

static int bindtodevice(int s, const char *name)
{
    return setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (char *)name, strnlen(name, IFNAMSIZ));
}

/* A UDP socket on [::]:0; *port gets its port. */
static int udp_any(unsigned short *port)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    sa6(&a, NULL, 0, 0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            getsockname(s, (struct sockaddr *)&a, &len) != 0) {
        CloseSocket(s);
        return -1;
    }
    *port = a.sin6_port;
    return s;
}

static int recv_from(int s, struct sockaddr_in6 *from)
{
    char buf[16];
    socklen_t len = sizeof(*from);

    if (sock_wait(s, 0, 2) <= 0)
        return -1;
    return recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)from, &len);
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The option on an IPv6 socket: an interface, an empty name, one no one has. */
void testBINDTODEV6_OPTION(void)
{
    int s;

    NEED_IF(1);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(bindtodevice(s, ifname[0]), 0);
    CU_ASSERT_EQUAL(setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (char *)"", 0), 0);
    CU_ASSERT_EQUAL(bindtodevice(s, "nosuch9"), -1);
    CU_ASSERT_EQUAL(Errno(), ENXIO);
    CloseSocket(s);
}

/* Bound to the second interface, a link-local peer with no zone is on its
 * link: the datagram comes from that interface's address. */
void testBINDTODEV6_UNZONED_PEER(void)
{
    struct sockaddr_in6 d, from;
    unsigned short sp, rp;
    int s, r;

    NEED_IF(2);
    s = udp_any(&sp);
    r = udp_any(&rp);
    CU_ASSERT_FATAL(s >= 0 && r >= 0);
    CU_ASSERT_EQUAL_FATAL(bindtodevice(s, ifname[1]), 0);
    sa6(&d, &linklocal[1], rp, 0);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL_FATAL(recv_from(r, &from), 1);
    CU_ASSERT(memcmp(&from.sin6_addr, &linklocal[1], sizeof(linklocal[1])) == 0);
    CU_ASSERT_EQUAL(from.sin6_scope_id, ifindex[1]);
    CloseSocket(r);
    CloseSocket(s);
}

/* ...and so for connect(): the connection is in that zone. */
void testBINDTODEV6_UNZONED_CONNECT(void)
{
    struct sockaddr_in6 d, l;
    socklen_t len = sizeof(l);
    unsigned short sp, rp;
    int s, r;

    NEED_IF(2);
    s = udp_any(&sp);
    r = udp_any(&rp);
    CU_ASSERT_FATAL(s >= 0 && r >= 0);
    CU_ASSERT_EQUAL_FATAL(bindtodevice(s, ifname[1]), 0);
    sa6(&d, &linklocal[1], rp, 0);
    CU_ASSERT_EQUAL_FATAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&l, &len), 0);
    CU_ASSERT(memcmp(&l.sin6_addr, &linklocal[1], sizeof(linklocal[1])) == 0);
    CU_ASSERT_EQUAL(l.sin6_scope_id, ifindex[1]);
    CloseSocket(r);
    CloseSocket(s);
}

/* A peer in another zone than the bound interface: EINVAL, for sendto()
 * and connect(). */
void testBINDTODEV6_OTHER_ZONE(void)
{
    struct sockaddr_in6 d;
    unsigned short sp, rp;
    int s, r;

    NEED_IF(2);
    s = udp_any(&sp);
    r = udp_any(&rp);
    CU_ASSERT_FATAL(s >= 0 && r >= 0);
    CU_ASSERT_EQUAL_FATAL(bindtodevice(s, ifname[0]), 0);
    sa6(&d, &linklocal[1], rp, ifindex[1]);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), -1);
    CU_ASSERT_EQUAL(Errno(), EINVAL);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&d, sizeof(d)), -1);
    CU_ASSERT_EQUAL(Errno(), EINVAL);
    CloseSocket(r);
    CloseSocket(s);
}

/* Bound to the second interface, it hears what arrives there, not what
 * arrives on the first. */
void testBINDTODEV6_HEARS_ITS_LINK(void)
{
    struct sockaddr_in6 d, from;
    unsigned short sp, rp;
    int s, r;

    NEED_IF(2);
    s = udp_any(&sp);
    r = udp_any(&rp);
    CU_ASSERT_FATAL(s >= 0 && r >= 0);
    CU_ASSERT_EQUAL_FATAL(bindtodevice(r, ifname[1]), 0);
    sa6(&d, &linklocal[0], rp, ifindex[0]);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL(recv_from(r, &from), -1);
    sa6(&d, &linklocal[1], rp, ifindex[1]);
    CU_ASSERT_EQUAL(sendto(s, "y", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL(recv_from(r, &from), 1);
    CloseSocket(r);
    CloseSocket(s);
}

/* A TCP listener on port of family fam bound to interface i (0 on error). */
static int tcp_listener(int fam, int i, unsigned short *port)
{
    struct sockaddr_storage a;
    socklen_t len = sizeof(a);
    int s = socket(fam, SOCK_STREAM, 0);

    if (s < 0)
        return -1;
    bzero(&a, sizeof(a));
    a.ss_family = fam;
    a.ss_len = fam == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    if (bindtodevice(s, ifname[i]) != 0 || bind(s, (struct sockaddr *)&a, a.ss_len) != 0 ||
            listen(s, 2) != 0 || getsockname(s, (struct sockaddr *)&a, &len) != 0) {
        CloseSocket(s);
        return -1;
    }
    *port = fam == AF_INET6 ? ((struct sockaddr_in6 *)&a)->sin6_port
                            : ((struct sockaddr_in *)&a)->sin_port;
    return s;
}

/* connect() to d: 0, or the errno. */
static int tcp_connect(const struct sockaddr *d, socklen_t len)
{
    int c = socket(d->sa_family, SOCK_STREAM, 0), r;

    if (c < 0)
        return -1;
    r = connect(c, d, len) == 0 ? 0 : Errno();
    CloseSocket(c);
    return r;
}

/* A TCP/IPv6 listener bound to the interface: refused over ::1, accepted
 * over the interface's link-local address (it arrives there). */
void testBINDTODEV6_TCP6_LISTENER(void)
{
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d;
    unsigned short port;
    int l;

    NEED_IF(1);
    CU_ASSERT_FATAL((l = tcp_listener(AF_INET6, 0, &port)) >= 0);
    sa6(&d, &lo, port, 0);
    CU_ASSERT_EQUAL(tcp_connect((struct sockaddr *)&d, sizeof(d)), ECONNREFUSED);
    sa6(&d, &linklocal[0], port, ifindex[0]);
    CU_ASSERT_EQUAL(tcp_connect((struct sockaddr *)&d, sizeof(d)), 0);
    CloseSocket(l);
}

/* A TCP/IPv4 listener bound to the interface: refused over 127.0.0.1. */
void testBINDTODEV6_TCP4_LISTENER(void)
{
    struct sockaddr_in d;
    unsigned short port;
    int l;

    NEED_IF(1);
    CU_ASSERT_FATAL((l = tcp_listener(AF_INET, 0, &port)) >= 0);
    bzero(&d, sizeof(d));
    d.sin_len = sizeof(d);
    d.sin_family = AF_INET;
    d.sin_port = port;
    d.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    CU_ASSERT_EQUAL(tcp_connect((struct sockaddr *)&d, sizeof(d)), ECONNREFUSED);
    CloseSocket(l);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int i;

    if (argc > 2) {
        ifname[0] = argv[1];
        ifname[1] = argv[2];
    }
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping BINDTODEV6 tests\n");
        return 0;
    }
    for (i = 0; i < 2; i++)
        have[i] = index_of(ifname[i], &ifindex[i]) == 0 && get_linklocal(i, &linklocal[i]);
    if (!have[0] || !have[1])
        printf("%s, %s: not both with link-local addresses - skipping what needs them\n",
               ifname[0], ifname[1]);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("BINDTODEV6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the option on an IPv6 socket", testBINDTODEV6_OPTION) ||
        !CU_add_test(p, "a peer with no zone is on the bound link", testBINDTODEV6_UNZONED_PEER) ||
        !CU_add_test(p, "and so for connect()", testBINDTODEV6_UNZONED_CONNECT) ||
        !CU_add_test(p, "a peer in another zone is refused", testBINDTODEV6_OTHER_ZONE) ||
        !CU_add_test(p, "it hears only its interface", testBINDTODEV6_HEARS_ITS_LINK) ||
        !CU_add_test(p, "a TCP/IPv6 listener only its interface", testBINDTODEV6_TCP6_LISTENER) ||
        !CU_add_test(p, "a TCP/IPv4 listener only its interface", testBINDTODEV6_TCP4_LISTENER))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-BINDTODEV6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
