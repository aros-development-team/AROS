/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv6 zones (RFC 4007, Readme section 6.6): a link-scope
    address names a link only together with its zone, the interface index
    in sin6_scope_id. bind() accepts only our own addresses, and a
    link-local one only in its own zone; connect() takes the zone from the
    peer's sin6_scope_id; getsockname(), getpeername(), accept() and
    recvfrom() report the zone; and a datagram to our own link-local
    address arrives on the link that address belongs to, not on lo0.

    It runs over the link-local address of an interface (default eth0, as
    the first argument); without it the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex, loindex;
static struct in6_addr linklocal;
static int ready;

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr,
                unsigned short port, unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_port = port;
    a->sin6_scope_id = scope;
}

static void sa6s(struct sockaddr_in6 *a, const char *addr, unsigned short port,
                 unsigned scope)
{
    struct in6_addr in;

    inet_pton(AF_INET6, (char *)addr, &in);
    sa6(a, &in, port, scope);
}

/* A socket of type bound to [::]:0; *port gets its port. */
static int bound_any(int type, unsigned short *port)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, type, 0);

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

#define NEED_IF() do { if (!ready) { CU_PASS("no link-local - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* bind() refuses an address that is not ours. */
void testSCOPE6_BIND_NOT_OURS(void)
{
    struct sockaddr_in6 a;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    sa6s(&a, "fd00:4d54:62::1", 0, 0);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&a, sizeof(a)), -1);
    CU_ASSERT_EQUAL(Errno(), EADDRNOTAVAIL);
    CloseSocket(s);
}

/* A link-local address binds in its zone, and getsockname() reports it. */
void testSCOPE6_BIND_LINKLOCAL(void)
{
    struct sockaddr_in6 a, l;
    socklen_t len = sizeof(l);
    int s;
    NEED_IF();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    sa6(&a, &linklocal, 0, ifindex);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&a, sizeof(a)), 0);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&l, &len), 0);
    CU_ASSERT(memcmp(&l.sin6_addr, &linklocal, sizeof(linklocal)) == 0);
    CU_ASSERT_EQUAL(l.sin6_scope_id, ifindex);
    CloseSocket(s);
}

/* ...but not in another zone. */
void testSCOPE6_BIND_WRONG_ZONE(void)
{
    struct sockaddr_in6 a;
    int s;
    NEED_IF();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    sa6(&a, &linklocal, 0, loindex);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&a, sizeof(a)), -1);
    CU_ASSERT_EQUAL(Errno(), EADDRNOTAVAIL);
    CloseSocket(s);
}

/* connect() to a link-local peer takes its zone; both ends report it. */
void testSCOPE6_CONNECT_ZONE(void)
{
    struct sockaddr_in6 d, a;
    socklen_t len = sizeof(a);
    int s;
    NEED_IF();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    sa6s(&d, "fe80::4d54:99", htons(9), ifindex);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    CU_ASSERT_EQUAL(getpeername(s, (struct sockaddr *)&a, &len), 0);
    CU_ASSERT_EQUAL(a.sin6_scope_id, ifindex);
    len = sizeof(a);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&a, &len), 0);
    CU_ASSERT(memcmp(&a.sin6_addr, &linklocal, sizeof(linklocal)) == 0);
    CU_ASSERT_EQUAL(a.sin6_scope_id, ifindex);
    CloseSocket(s);
}

/* connect() refuses a zone no interface has. */
void testSCOPE6_CONNECT_BAD_ZONE(void)
{
    struct sockaddr_in6 d;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    sa6s(&d, "fe80::4d54:99", htons(9), 999);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&d, sizeof(d)), -1);
    CU_ASSERT_EQUAL(Errno(), ENXIO);
    CloseSocket(s);
}

/* recvfrom() gives a link-local source with its zone: the link it is on. */
void testSCOPE6_RECVFROM_ZONE(void)
{
    struct sockaddr_in6 d, from;
    socklen_t len = sizeof(from);
    unsigned short rp;
    char buf[8];
    int r, s;
    NEED_IF();
    r = bound_any(SOCK_DGRAM, &rp);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    sa6(&d, &linklocal, rp, ifindex);
    CU_ASSERT_EQUAL(sendto(s, "z", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    CU_ASSERT_EQUAL(recvfrom(r, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len), 1);
    CU_ASSERT(memcmp(&from.sin6_addr, &linklocal, sizeof(linklocal)) == 0);
    CU_ASSERT_EQUAL(from.sin6_scope_id, ifindex);
    CloseSocket(s);
    CloseSocket(r);
}

/* IPV6_PKTINFO gives the link the datagram's link-local destination is on. */
void testSCOPE6_PKTINFO_IFINDEX(void)
{
    union { struct cmsghdr c; char b[CMSG_SPACE(sizeof(struct in6_pktinfo))]; } ctl;
    struct sockaddr_in6 d, from;
    struct in6_pktinfo pi;
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    unsigned short rp;
    char buf[8];
    int r, s, on = 1, got = 0;
    NEED_IF();
    r = bound_any(SOCK_DGRAM, &rp);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)), 0);
    sa6(&d, &linklocal, rp, ifindex);
    CU_ASSERT_EQUAL(sendto(s, "z", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    bzero(&mh, sizeof(mh));
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    mh.msg_name = &from;
    mh.msg_namelen = sizeof(from);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = &ctl;
    mh.msg_controllen = sizeof(ctl);
    CU_ASSERT_EQUAL(recvmsg(r, &mh, 0), 1);
    for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c))
        if (c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_PKTINFO) {
            memcpy(&pi, CMSG_DATA(c), sizeof(pi));
            got = 1;
        }
    CU_ASSERT(got);
    CU_ASSERT(got && pi.ipi6_ifindex == ifindex);
    CloseSocket(s);
    CloseSocket(r);
}

/* A TCP connection over the link-local address: accept() and both
 * getsockname()/getpeername() of the accepted socket report the zone. */
void testSCOPE6_TCP_ACCEPT_ZONE(void)
{
    struct sockaddr_in6 d, a;
    socklen_t len = sizeof(a);
    unsigned short lp;
    int l, c, n;
    NEED_IF();
    l = bound_any(SOCK_STREAM, &lp);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    c = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    sa6(&d, &linklocal, lp, ifindex);
    CU_ASSERT_EQUAL(connect(c, (struct sockaddr *)&d, sizeof(d)), 0);
    CU_ASSERT(sock_wait(l, 0, 2) > 0);
    n = accept(l, (struct sockaddr *)&a, &len);
    CU_ASSERT_FATAL(n >= 0);
    CU_ASSERT(memcmp(&a.sin6_addr, &linklocal, sizeof(linklocal)) == 0);
    CU_ASSERT_EQUAL(a.sin6_scope_id, ifindex);
    len = sizeof(a);
    CU_ASSERT_EQUAL(getsockname(n, (struct sockaddr *)&a, &len), 0);
    CU_ASSERT_EQUAL(a.sin6_scope_id, ifindex);
    len = sizeof(a);
    CU_ASSERT_EQUAL(getpeername(c, (struct sockaddr *)&a, &len), 0);
    CU_ASSERT_EQUAL(a.sin6_scope_id, ifindex);
    CloseSocket(n);
    CloseSocket(c);
    CloseSocket(l);
}

static int index_of(int s, const char *name, unsigned *index)
{
    struct ifreq ifr;

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    if (IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) != 0)
        return -1;
    *index = ifr.ifr_index;
    return 0;
}

static int setup(void)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s, i, n, found = 0;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    if (index_of(s, ifname, &ifindex) != 0 || index_of(s, "lo0", &loindex) != 0) {
        CloseSocket(s);
        return 0;
    }
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        n = ifc.ifc6_len / sizeof(req[0]);
        for (i = 0; i < n; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                linklocal = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SCOPE6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no link-local address - skipping the SCOPE6 tests that need one\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SCOPE6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "bind() refuses an address that is not ours", testSCOPE6_BIND_NOT_OURS) ||
        !CU_add_test(p, "a link-local address binds in its zone", testSCOPE6_BIND_LINKLOCAL) ||
        !CU_add_test(p, "a link-local address does not bind in another zone", testSCOPE6_BIND_WRONG_ZONE) ||
        !CU_add_test(p, "connect() takes the peer's zone", testSCOPE6_CONNECT_ZONE) ||
        !CU_add_test(p, "connect() refuses an unknown zone", testSCOPE6_CONNECT_BAD_ZONE) ||
        !CU_add_test(p, "recvfrom() reports the source's zone", testSCOPE6_RECVFROM_ZONE) ||
        !CU_add_test(p, "IPV6_PKTINFO gives the link-local's own link", testSCOPE6_PKTINFO_IFINDEX) ||
        !CU_add_test(p, "TCP over a link-local address reports the zone", testSCOPE6_TCP_ACCEPT_ZONE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SCOPE6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
