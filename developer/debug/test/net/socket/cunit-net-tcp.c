/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for bsdsocket.library TCP (IPv4) over the loopback interface:
    handshake, names, echo, large transfer (window scaling), multiple
    simultaneous connections (SYN cache), half-close, refused connect and MSS.
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

void testHANDSHAKE(void)
{
    int c = -1, s = -1;
    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    /* Both ends must report a connected peer. */
    CU_ASSERT_EQUAL(getpeername(c, (struct sockaddr *)&ss, &len), 0);
    len = sizeof(ss);
    CU_ASSERT_EQUAL(getpeername(s, (struct sockaddr *)&ss, &len), 0);
    CloseSocket(c);
    CloseSocket(s);
}

void testNAMES(void)
{
    int c = -1, s = -1;
    struct sockaddr_in cl, sp;
    socklen_t len;

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    /* The client's local name must equal the server's peer name. */
    len = sizeof(cl);
    CU_ASSERT_EQUAL(getsockname(c, (struct sockaddr *)&cl, &len), 0);
    len = sizeof(sp);
    CU_ASSERT_EQUAL(getpeername(s, (struct sockaddr *)&sp, &len), 0);
    CU_ASSERT_EQUAL(cl.sin_port, sp.sin_port);
    CU_ASSERT_EQUAL(cl.sin_addr.s_addr, sp.sin_addr.s_addr);
    CloseSocket(c);
    CloseSocket(s);
}

void testECHO(void)
{
    int c = -1, s = -1, r;
    char buf[16];

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    CU_ASSERT_EQUAL(send(c, "hello", 5, 0), 5);
    CU_ASSERT(sock_wait(s, 0, 3) > 0);
    r = recv(s, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 5);
    CU_ASSERT(r == 5 && memcmp(buf, "hello", 5) == 0);
    CU_ASSERT_EQUAL(send(s, buf, r, 0), r);
    CU_ASSERT(sock_wait(c, 0, 3) > 0);
    r = recv(c, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 5);
    CU_ASSERT(r == 5 && memcmp(buf, "hello", 5) == 0);
    CloseSocket(c);
    CloseSocket(s);
}

void testLARGE(void)
{
    int c = -1, s = -1;

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    /* 256 KB exceeds a single unscaled window: exercises RFC 1323 scaling. */
    CU_ASSERT_EQUAL(st_oneway(c, s, 256 * 1024), 0);
    CloseSocket(c);
    CloseSocket(s);
}

void testMULTI(void)
{
    unsigned short nport;
    int lfd, i, naccepted = 0, on = 1;
    int clients[8], accepted[8];
    struct sockaddr_storage ss;
    socklen_t len;

    lfd = st_bound(AF_INET, SOCK_STREAM, &nport);
    CU_ASSERT(lfd >= 0);
    if (lfd < 0)
        return;
    CU_ASSERT_EQUAL(listen(lfd, 8), 0);

    /* Fire 8 simultaneous connects; the half-opens live in the SYN cache. */
    for (i = 0; i < 8; i++) {
        clients[i] = socket(AF_INET, SOCK_STREAM, 0);
        IoctlSocket(clients[i], FIONBIO, (char *)&on);
        len = st_loopback(AF_INET, &ss, nport);
        connect(clients[i], (struct sockaddr *)&ss, len);
    }
    for (i = 0; i < 8; i++) {
        int a;
        if (sock_wait(lfd, 0, 3) <= 0)
            break;
        len = sizeof(ss);
        a = accept(lfd, (struct sockaddr *)&ss, &len);
        if (a >= 0)
            accepted[naccepted++] = a;
    }
    CU_ASSERT_EQUAL(naccepted, 8);

    for (i = 0; i < 8; i++)
        CloseSocket(clients[i]);
    for (i = 0; i < naccepted; i++)
        CloseSocket(accepted[i]);
    CloseSocket(lfd);
}

void testHALFCLOSE(void)
{
    int c = -1, s = -1, r;
    char buf[8];

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    CU_ASSERT_EQUAL(send(c, "AB", 2, 0), 2);
    CU_ASSERT_EQUAL(shutdown(c, SHUT_WR), 0);

    CU_ASSERT(sock_wait(s, 0, 3) > 0);
    r = recv(s, buf, sizeof(buf), 0);
    CU_ASSERT_EQUAL(r, 2);
    /* After the peer's half-close, the server must see EOF. */
    CU_ASSERT(sock_wait(s, 0, 3) > 0);
    CU_ASSERT_EQUAL(recv(s, buf, sizeof(buf), 0), 0);
    /* ...but the reverse direction is still open. */
    CU_ASSERT_EQUAL(send(s, "CD", 2, 0), 2);
    CU_ASSERT(sock_wait(c, 0, 3) > 0);
    CU_ASSERT_EQUAL(recv(c, buf, sizeof(buf), 0), 2);
    CloseSocket(c);
    CloseSocket(s);
}

void testREFUSED(void)
{
    unsigned short nport;
    int tmp, c, on = 1, err = 0, r;
    socklen_t elen = sizeof(err), len;
    struct sockaddr_storage ss;

    /* Bind then close a socket to obtain a port with no listener. */
    tmp = st_bound(AF_INET, SOCK_STREAM, &nport);
    CU_ASSERT(tmp >= 0);
    if (tmp < 0)
        return;
    CloseSocket(tmp);

    c = socket(AF_INET, SOCK_STREAM, 0);
    IoctlSocket(c, FIONBIO, (char *)&on);
    len = st_loopback(AF_INET, &ss, nport);
    r = connect(c, (struct sockaddr *)&ss, len);
    if (r == 0) {
        CU_FAIL("connect to a closed port unexpectedly succeeded");
    } else if (Errno() == EINPROGRESS) {
        sock_wait(c, 1, 3);
        CU_ASSERT_EQUAL(getsockopt(c, SOL_SOCKET, SO_ERROR, &err, &elen), 0);
        CU_ASSERT_EQUAL(err, ECONNREFUSED);
    } else {
        CU_ASSERT_EQUAL(Errno(), ECONNREFUSED);
    }
    CloseSocket(c);
}

void testMAXSEG(void)
{
    int c = -1, s = -1, mss = 0;
    socklen_t len = sizeof(mss);

    CU_ASSERT_EQUAL(st_tcp_pair(AF_INET, &c, &s), 0);
    if (c < 0 || s < 0)
        return;
    CU_ASSERT_EQUAL(getsockopt(c, IPPROTO_TCP, TCP_MAXSEG, &mss, &len), 0);
    CU_ASSERT(mss > 0);
    CloseSocket(c);
    CloseSocket(s);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TCP tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TCP_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "handshake", testHANDSHAKE) ||
        !CU_add_test(p, "getsockname/getpeername", testNAMES) ||
        !CU_add_test(p, "echo", testECHO) ||
        !CU_add_test(p, "large transfer (window scaling)", testLARGE) ||
        !CU_add_test(p, "multiple connections (SYN cache)", testMULTI) ||
        !CU_add_test(p, "half-close (shutdown)", testHALFCLOSE) ||
        !CU_add_test(p, "connect refused", testREFUSED) ||
        !CU_add_test(p, "TCP_MAXSEG", testMAXSEG))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TCP");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
