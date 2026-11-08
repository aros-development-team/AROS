/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for TCP_USER_TIMEOUT (RFC 5482, Linux's option): how long,
    in milliseconds, data may wait - sent and unacknowledged, or held back
    by a zero window - before the connection is dropped with ETIMEDOUT.
    The option reads back what was set, 0 by default, refuses a negative
    value and is passed from a listening socket to the connections it
    accepts. Over the loopback a peer that never reads closes its window:
    with a user timeout the sender gives up at about that time; without
    one, the connection stays up. The retransmission and keepalive cases
    need a peer that goes silent and are tested on the wire (tests/tcp in
    aros-matter). Usage:  cunit-net-uto
*/

#include "socktest.h"

#include <proto/dos.h>

struct Library *SocketBase = NULL;

static int geti(int s, int opt)
{
    int v = -99;
    socklen_t l = sizeof(v);

    if (getsockopt(s, IPPROTO_TCP, opt, &v, &l) != 0)
        return -98;
    return v;
}

static int seti(int s, int opt, int v)
{
    return setsockopt(s, IPPROTO_TCP, opt, &v, sizeof(v));
}

/* Ticks (1/50 s) since some fixed time. */
static long ticks(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ds.ds_Days * 1440L + ds.ds_Minute) * 60L * TICKS_PER_SECOND + ds.ds_Tick;
}

/* The connection's pending error, 0 if none. */
static int soerror(int s)
{
    int e = 0;
    socklen_t l = sizeof(e);

    getsockopt(s, SOL_SOCKET, SO_ERROR, &e, &l);
    return e;
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* 0 until set; what was set; a negative value refused. */
void testUTO_OPTION(void)
{
    int s = socket(AF_INET6, SOCK_STREAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(geti(s, TCP_USER_TIMEOUT), 0);
    CU_ASSERT_EQUAL(seti(s, TCP_USER_TIMEOUT, 2500), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_USER_TIMEOUT), 2500);
    CU_ASSERT_NOT_EQUAL(seti(s, TCP_USER_TIMEOUT, -1), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_USER_TIMEOUT), 2500);
    CU_ASSERT_EQUAL(seti(s, TCP_USER_TIMEOUT, 0), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_USER_TIMEOUT), 0);
    CloseSocket(s);
}

/* A connection accepted by a listening socket has its user timeout. */
void testUTO_INHERITED(void)
{
    struct sockaddr_storage ss;
    socklen_t len;
    unsigned short port;
    int l, c, a;

    l = st_bound(AF_INET, SOCK_STREAM, &port);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(seti(l, TCP_USER_TIMEOUT, 7000), 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    c = socket(AF_INET, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    len = st_loopback(AF_INET, &ss, port);
    CU_ASSERT_EQUAL(connect(c, (struct sockaddr *)&ss, len), 0);
    CU_ASSERT(sock_wait(l, 0, 3) > 0);
    a = accept(l, NULL, NULL);
    CU_ASSERT_FATAL(a >= 0);
    CU_ASSERT_EQUAL(geti(a, TCP_USER_TIMEOUT), 7000);
    CU_ASSERT_EQUAL(geti(c, TCP_USER_TIMEOUT), 0);
    CloseSocket(a);
    CloseSocket(c);
    CloseSocket(l);
}

/*
 * Keep sending on a connection whose peer never reads, so that its window
 * closes and data waits; wait up to secs from the last send that went
 * through for the connection to fail. Returns its error (0 if it is still
 * up) and *waited the ticks from that last send.
 */
static int zero_window(int family, int uto, int secs, long *waited)
{
    static char buf[8192];
    long on = 1, t0, t = 0;
    int c = -1, s = -1, e = 0;

    *waited = -1;
    if (st_tcp_pair(family, &c, &s) != 0)
        return -1;
    if (uto)
        seti(c, TCP_USER_TIMEOUT, uto);
    IoctlSocket(c, FIONBIO, (char *)&on);
    t0 = ticks();
    while (!e && (t = ticks() - t0) < secs * TICKS_PER_SECOND) {
        if (send(c, buf, sizeof(buf), 0) > 0)
            t0 = ticks();               /* still moving */
        else
            Delay(TICKS_PER_SECOND / 10);
        e = soerror(c);
    }
    *waited = t;
    CloseSocket(c);
    CloseSocket(s);
    return e;
}

/*
 * An idle connection, nothing waiting, outlives its user timeout: the
 * timeout set before connect() (the SYN once waited for its ACK) or after,
 * on both ends; 3 s idle with 1000 ms, then a send() goes through.
 */
static void idle(int family, int before)
{
    struct sockaddr_storage ss;
    socklen_t len;
    unsigned short port;
    int l, c, a;

    l = st_bound(family, SOCK_STREAM, &port);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    if (before)
        CU_ASSERT_EQUAL(seti(l, TCP_USER_TIMEOUT, 1000), 0);
    c = socket(family, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    if (before)
        CU_ASSERT_EQUAL(seti(c, TCP_USER_TIMEOUT, 1000), 0);
    len = st_loopback(family, &ss, port);
    CU_ASSERT_EQUAL(connect(c, (struct sockaddr *)&ss, len), 0);
    CU_ASSERT(sock_wait(l, 0, 3) > 0);
    a = accept(l, NULL, NULL);
    CU_ASSERT_FATAL(a >= 0);
    if (!before) {
        CU_ASSERT_EQUAL(seti(c, TCP_USER_TIMEOUT, 1000), 0);
        CU_ASSERT_EQUAL(seti(a, TCP_USER_TIMEOUT, 1000), 0);
    }
    Delay(3 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(soerror(c), 0);
    CU_ASSERT_EQUAL(soerror(a), 0);
    CU_ASSERT_EQUAL(send(c, "i", 1, 0), 1);
    CU_ASSERT_EQUAL(send(a, "j", 1, 0), 1);
    CloseSocket(a);
    CloseSocket(c);
    CloseSocket(l);
}

void testUTO_IDLE_BEFORE4(void) { idle(AF_INET, 1); }
void testUTO_IDLE_AFTER6(void) { idle(AF_INET6, 0); }

/* A user timeout of 2 s drops it at about 2 s, with ETIMEDOUT. */
static void zero_window_drops(int family)
{
    long waited;
    int e = zero_window(family, 2000, 8, &waited);

    CU_ASSERT_EQUAL(e, ETIMEDOUT);
    CU_ASSERT(waited >= 1 * TICKS_PER_SECOND && waited <= 5 * TICKS_PER_SECOND);
}

void testUTO_ZERO_WINDOW4(void) { zero_window_drops(AF_INET); }
void testUTO_ZERO_WINDOW6(void) { zero_window_drops(AF_INET6); }

/* Without one, a shut window does not drop the connection. */
void testUTO_ZERO_WINDOW_OFF(void)
{
    long waited;

    CU_ASSERT_EQUAL(zero_window(AF_INET, 0, 5, &waited), 0);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping UTO tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("UTO_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the option: 0, what was set, no negative", testUTO_OPTION) ||
        !CU_add_test(p, "an accepted connection has the listener's", testUTO_INHERITED) ||
        !CU_add_test(p, "an idle connection outlives it, set before connect() (IPv4)", testUTO_IDLE_BEFORE4) ||
        !CU_add_test(p, "an idle connection outlives it, set after connect() (IPv6)", testUTO_IDLE_AFTER6) ||
        !CU_add_test(p, "a shut window drops the connection at the timeout (IPv4)", testUTO_ZERO_WINDOW4) ||
        !CU_add_test(p, "a shut window drops the connection at the timeout (IPv6)", testUTO_ZERO_WINDOW6) ||
        !CU_add_test(p, "without one, a shut window does not", testUTO_ZERO_WINDOW_OFF))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-UTO");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
