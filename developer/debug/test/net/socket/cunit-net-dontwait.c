/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for MSG_DONTWAIT (4.4BSD; sys/socket.h): a send or receive
    that would block returns EWOULDBLOCK at once, for that call only - the
    socket stays blocking. UDP and TCP, over 127.0.0.1 and ::1. Each socket
    has a 2 s SO_RCVTIMEO/SO_SNDTIMEO, so a stack that ignores the flag
    returns (late) instead of hanging the test; "at once" is under half a
    second. Usage:  cunit-net-dontwait
*/

#include "socktest.h"

#include <proto/dos.h>

struct Library *SocketBase = NULL;

static char dbuf[8192];

static long ticks(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ds.ds_Days * 1440L + ds.ds_Minute) * 60L * TICKS_PER_SECOND + ds.ds_Tick;
}

static void guard(int s)
{
    struct timeval tv = { 2, 0 };

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* UDP: an empty socket, recv() and recvfrom() with MSG_DONTWAIT; then a
 * plain recv() still waits for the datagram. */
static void udp_recv(int family)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    unsigned short port;
    long t;
    int r, s, n;

    r = st_bound(family, SOCK_DGRAM, &port);
    s = socket(family, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && s >= 0);
    guard(r);
    t = ticks();
    n = recv(r, dbuf, sizeof(dbuf), MSG_DONTWAIT);
    t = ticks() - t;
    CU_ASSERT_EQUAL(n, -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT(t < TICKS_PER_SECOND / 2);
    t = ticks();
    n = recvfrom(r, dbuf, sizeof(dbuf), MSG_DONTWAIT, NULL, NULL);
    t = ticks() - t;
    CU_ASSERT_EQUAL(n, -1);
    CU_ASSERT(t < TICKS_PER_SECOND / 2);
    dlen = st_loopback(family, &dst, port);
    CU_ASSERT_EQUAL(sendto(s, "u", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT_EQUAL(recv(r, dbuf, sizeof(dbuf), 0), 1);     /* still blocking */
    CloseSocket(s);
    CloseSocket(r);
}

void testDONTWAIT_UDP4(void) { udp_recv(AF_INET); }
void testDONTWAIT_UDP6(void) { udp_recv(AF_INET6); }

/* TCP: recv() with MSG_DONTWAIT on an empty connection; then a plain
 * recv() waits for the data. */
static void tcp_recv(int family)
{
    long t;
    int c = -1, s = -1, n;

    CU_ASSERT_FATAL(st_tcp_pair(family, &c, &s) == 0);
    guard(s);
    t = ticks();
    n = recv(s, dbuf, sizeof(dbuf), MSG_DONTWAIT);
    t = ticks() - t;
    CU_ASSERT_EQUAL(n, -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT(t < TICKS_PER_SECOND / 2);
    CU_ASSERT_EQUAL(send(c, "t", 1, 0), 1);
    CU_ASSERT_EQUAL(recv(s, dbuf, sizeof(dbuf), 0), 1);
    CloseSocket(c);
    CloseSocket(s);
}

void testDONTWAIT_TCP_RECV4(void) { tcp_recv(AF_INET); }
void testDONTWAIT_TCP_RECV6(void) { tcp_recv(AF_INET6); }

/* TCP: send() with MSG_DONTWAIT to a peer that does not read, until the
 * buffers are full: EWOULDBLOCK at once, not a wait. */
void testDONTWAIT_TCP_SEND(void)
{
    long t = 0, total = 0;
    int c = -1, s = -1, n, i;

    CU_ASSERT_FATAL(st_tcp_pair(AF_INET6, &c, &s) == 0);
    guard(c);
    for (i = 0; i < 1000; i++) {
        long t0 = ticks();

        n = send(c, dbuf, sizeof(dbuf), MSG_DONTWAIT);
        t = ticks() - t0;
        if (n <= 0)
            break;
        total += n;
    }
    CU_ASSERT(total > 0);
    CU_ASSERT_EQUAL(n, -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT(t < TICKS_PER_SECOND / 2);
    CloseSocket(c);
    CloseSocket(s);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping DONTWAIT tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("DONTWAIT_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "UDP recv: EWOULDBLOCK at once, socket still blocking (IPv4)", testDONTWAIT_UDP4) ||
        !CU_add_test(p, "UDP recv: EWOULDBLOCK at once, socket still blocking (IPv6)", testDONTWAIT_UDP6) ||
        !CU_add_test(p, "TCP recv: EWOULDBLOCK at once (IPv4)", testDONTWAIT_TCP_RECV4) ||
        !CU_add_test(p, "TCP recv: EWOULDBLOCK at once (IPv6)", testDONTWAIT_TCP_RECV6) ||
        !CU_add_test(p, "TCP send to a full peer: EWOULDBLOCK at once", testDONTWAIT_TCP_SEND))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-DONTWAIT");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
