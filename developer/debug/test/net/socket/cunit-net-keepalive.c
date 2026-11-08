/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the keepalive options of a TCP socket, as FreeBSD has
    them: TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPINIT in seconds and
    TCP_KEEPCNT in probes. Each reads back the system value until set, takes
    a value of its own and gives it back, returns to the system value with
    0, refuses what is negative or too large, and a connection accepted by
    a listening socket has the listener's values. The probes themselves
    are tested on the wire (tests/tcp in aros-matter). Over 127.0.0.1 and
    ::1. Usage:  cunit-net-keepalive
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

/* The system values: 2 hours idle, 75 s between probes, 8 probes, 75 s
 * to connect (netinet/tcp_timer.h). */
#define SYS_IDLE    7200
#define SYS_INTVL   75
#define SYS_CNT     8
#define SYS_INIT    75

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

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* Before anything is set, the system values. */
void testKEEPALIVE_DEFAULTS(void)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPIDLE), SYS_IDLE);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPINTVL), SYS_INTVL);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPCNT), SYS_CNT);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPINIT), SYS_INIT);
    CloseSocket(s);
}

/* A value of the socket's own, and 0 for the system value again. */
static void roundtrip(int family)
{
    int s = socket(family, SOCK_STREAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPIDLE, 30), 0);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPINTVL, 5), 0);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPCNT, 4), 0);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPINIT, 10), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPIDLE), 30);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPINTVL), 5);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPCNT), 4);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPINIT), 10);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPIDLE, 0), 0);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPCNT, 0), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPIDLE), SYS_IDLE);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPCNT), SYS_CNT);
    CloseSocket(s);
}

void testKEEPALIVE_ROUNDTRIP4(void) { roundtrip(AF_INET); }
void testKEEPALIVE_ROUNDTRIP6(void) { roundtrip(AF_INET6); }

/* Negative, or more than the timers hold (16383 s), is refused and leaves
 * the value as it was. */
void testKEEPALIVE_BOUNDS(void)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_NOT_EQUAL(seti(s, TCP_KEEPIDLE, -1), 0);
    CU_ASSERT_NOT_EQUAL(seti(s, TCP_KEEPIDLE, 16384), 0);
    CU_ASSERT_NOT_EQUAL(seti(s, TCP_KEEPINTVL, 100000), 0);
    CU_ASSERT_NOT_EQUAL(seti(s, TCP_KEEPCNT, -5), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPIDLE), SYS_IDLE);
    CU_ASSERT_EQUAL(seti(s, TCP_KEEPIDLE, 16383), 0);
    CU_ASSERT_EQUAL(geti(s, TCP_KEEPIDLE), 16383);
    CloseSocket(s);
}

/* A connection accepted by a listening socket has its values. */
static void inherited(int family)
{
    struct sockaddr_storage ss;
    socklen_t len;
    unsigned short port;
    int l, c, a;

    l = st_bound(family, SOCK_STREAM, &port);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(seti(l, TCP_KEEPIDLE, 40), 0);
    CU_ASSERT_EQUAL(seti(l, TCP_KEEPINTVL, 3), 0);
    CU_ASSERT_EQUAL(seti(l, TCP_KEEPCNT, 6), 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    c = socket(family, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    len = st_loopback(family, &ss, port);
    CU_ASSERT_EQUAL(connect(c, (struct sockaddr *)&ss, len), 0);
    CU_ASSERT(sock_wait(l, 0, 3) > 0);
    a = accept(l, NULL, NULL);
    CU_ASSERT_FATAL(a >= 0);
    CU_ASSERT_EQUAL(geti(a, TCP_KEEPIDLE), 40);
    CU_ASSERT_EQUAL(geti(a, TCP_KEEPINTVL), 3);
    CU_ASSERT_EQUAL(geti(a, TCP_KEEPCNT), 6);
    CU_ASSERT_EQUAL(geti(c, TCP_KEEPIDLE), SYS_IDLE);  /* not the client's */
    CloseSocket(a);
    CloseSocket(c);
    CloseSocket(l);
}

void testKEEPALIVE_INHERITED4(void) { inherited(AF_INET); }
void testKEEPALIVE_INHERITED6(void) { inherited(AF_INET6); }

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping KEEPALIVE tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("KEEPALIVE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the system values until set", testKEEPALIVE_DEFAULTS) ||
        !CU_add_test(p, "set, read back, 0 for the system value (IPv4)", testKEEPALIVE_ROUNDTRIP4) ||
        !CU_add_test(p, "set, read back, 0 for the system value (IPv6)", testKEEPALIVE_ROUNDTRIP6) ||
        !CU_add_test(p, "negative and too large are refused", testKEEPALIVE_BOUNDS) ||
        !CU_add_test(p, "an accepted connection has the listener's values (IPv4)", testKEEPALIVE_INHERITED4) ||
        !CU_add_test(p, "an accepted connection has the listener's values (IPv6)", testKEEPALIVE_INHERITED6))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-KEEPALIVE");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
