/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for bsdsocket.library IPv6 (AF_INET6) over the ::1 loopback:
    socket creation, bind/getsockname, TCP handshake+echo+large transfer,
    UDP sendto/recvfrom, and the IPV6_V6ONLY / IPV6_UNICAST_HOPS options.
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

void testV6_SOCKET(void)
{
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CloseSocket(s);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CloseSocket(s);
}

void testV6_BIND_NAME(void)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    struct sockaddr_in6 sa;
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_storage ss;
    socklen_t len;

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    len = st_loopback(AF_INET6, &ss, 0);
    CU_ASSERT_EQUAL(bind(s, (struct sockaddr *)&ss, len), 0);

    bzero(&sa, sizeof(sa));
    len = sizeof(sa);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&sa, &len), 0);
    CU_ASSERT_EQUAL(sa.sin6_family, AF_INET6);
    CU_ASSERT(memcmp(&sa.sin6_addr, &lo, sizeof(lo)) == 0);
    CU_ASSERT(sa.sin6_port != 0);
    CloseSocket(s);
}

void testV6_TCP_ECHO(void)
{
    int c = -1, s = -1, r;
    char buf[16];

    if (st_tcp_pair(AF_INET6, &c, &s) != 0) {
        CU_FAIL("TCP over IPv6 handshake (st_tcp_pair) failed");
        return;
    }
    CU_ASSERT_EQUAL(send(c, "ipv6!", 5, 0), 5);
    CU_ASSERT(sock_wait(s, 0, 3) > 0);
    r = recv(s, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 5);
    CU_ASSERT(r == 5 && memcmp(buf, "ipv6!", 5) == 0);
    CU_ASSERT_EQUAL(send(s, buf, r, 0), r);
    CU_ASSERT(sock_wait(c, 0, 3) > 0);
    CU_ASSERT_EQUAL(recv(c, buf, sizeof(buf), 0), 5);
    CloseSocket(c);
    CloseSocket(s);
}

void testV6_LARGE(void)
{
    int c = -1, s = -1;

    if (st_tcp_pair(AF_INET6, &c, &s) != 0) {
        CU_FAIL("TCP over IPv6 handshake (st_tcp_pair) failed");
        return;
    }
    CU_ASSERT_EQUAL(st_oneway(c, s, 128 * 1024), 0);
    CloseSocket(c);
    CloseSocket(s);
}

void testV6_UDP(void)
{
    unsigned short nport;
    int rfd, cfd, r;
    struct sockaddr_storage dst, src;
    socklen_t len;
    char buf[32];

    rfd = st_bound(AF_INET6, SOCK_DGRAM, &nport);
    CU_ASSERT(rfd >= 0);
    if (rfd < 0)
        return;
    cfd = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT(cfd >= 0);
    if (cfd < 0) {
        CloseSocket(rfd);
        return;
    }
    len = st_loopback(AF_INET6, &dst, nport);
    CU_ASSERT_EQUAL(sendto(cfd, "v6dg", 4, 0, (struct sockaddr *)&dst, len), 4);
    CU_ASSERT(sock_wait(rfd, 0, 3) > 0);
    len = sizeof(src);
    r = recvfrom(rfd, buf, sizeof(buf), 0, (struct sockaddr *)&src, &len);
    CU_ASSERT_EQUAL(r, 4);
    CU_ASSERT(r == 4 && memcmp(buf, "v6dg", 4) == 0);
    CU_ASSERT_EQUAL(src.ss_family, AF_INET6);
    CloseSocket(cfd);
    CloseSocket(rfd);
}

void testV6_V6ONLY(void)
{
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    int on = 1, val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == 0) {
        CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
            &val, &len), 0);
        CU_ASSERT(val != 0);
    } else {
        CU_PASS("IPV6_V6ONLY not supported - skipping");
    }
    CloseSocket(s);
}

void testV6_HOPS(void)
{
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    int hops = 47, val = 0;
    socklen_t len = sizeof(val);

    CU_ASSERT(s >= 0);
    if (s < 0)
        return;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
            &hops, sizeof(hops)) == 0) {
        CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
            &val, &len), 0);
        CU_ASSERT_EQUAL(val, 47);
    } else {
        CU_PASS("IPV6_UNICAST_HOPS not supported - skipping");
    }
    CloseSocket(s);
}

/* Connecting to a closed IPv6 port must be refused (exercises the IPv6
 * dropwithreset / tcp_respond RST path). */
void testV6_REFUSED(void)
{
    unsigned short nport;
    int tmp, c, on = 1, err = 0, r;
    socklen_t elen = sizeof(err), len;
    struct sockaddr_storage ss;

    tmp = st_bound(AF_INET6, SOCK_STREAM, &nport);
    CU_ASSERT(tmp >= 0);
    if (tmp < 0)
        return;
    CloseSocket(tmp);

    c = socket(AF_INET6, SOCK_STREAM, 0);
    IoctlSocket(c, FIONBIO, (char *)&on);
    len = st_loopback(AF_INET6, &ss, nport);
    r = connect(c, (struct sockaddr *)&ss, len);
    if (r == 0) {
        CU_FAIL("connect to a closed IPv6 port unexpectedly succeeded");
    } else if (Errno() == EINPROGRESS) {
        sock_wait(c, 1, 3);
        CU_ASSERT_EQUAL(getsockopt(c, SOL_SOCKET, SO_ERROR, &err, &elen), 0);
        CU_ASSERT_EQUAL(err, ECONNREFUSED);
    } else {
        CU_ASSERT_EQUAL(Errno(), ECONNREFUSED);
    }
    CloseSocket(c);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping IPv6 tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("INET6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "socket(AF_INET6)", testV6_SOCKET) ||
        !CU_add_test(p, "bind ::1 / getsockname", testV6_BIND_NAME) ||
        !CU_add_test(p, "TCP echo over ::1", testV6_TCP_ECHO) ||
        !CU_add_test(p, "large transfer over ::1", testV6_LARGE) ||
        !CU_add_test(p, "UDP over ::1", testV6_UDP) ||
        !CU_add_test(p, "IPV6_V6ONLY", testV6_V6ONLY) ||
        !CU_add_test(p, "IPV6_UNICAST_HOPS", testV6_HOPS) ||
        !CU_add_test(p, "connect refused over ::1", testV6_REFUSED))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-INET6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
