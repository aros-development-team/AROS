/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for TCP_INFO (Linux's option, FreeBSD's struct tcp_info):
    what getsockopt() gives of a connection - its state, the options it
    negotiated, its maximum segment size, windows and sequence numbers,
    and its round-trip estimate and retransmission timeout, in
    microseconds. A short buffer gets the start of the structure; a
    listening socket reports LISTEN. Over the loopback. Usage:
    cunit-net-tcpinfo
*/

#include "socktest.h"

#include <proto/dos.h>
#include <netinet/tcp_fsm.h>

struct Library *SocketBase = NULL;

static int info(int s, struct tcp_info *ti, socklen_t *len)
{
    socklen_t l = sizeof(*ti);

    memset(ti, 0xa5, sizeof(*ti));
    if (len == NULL)
        len = &l;
    return getsockopt(s, IPPROTO_TCP, TCP_INFO, ti, len);
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* An established connection: its state, segment size, windows, and a
 * retransmission timeout between the RFC 6298 bounds the stack keeps. */
static void established(int family)
{
    struct tcp_info ti;
    socklen_t len = sizeof(ti);
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(family, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, &len), 0);
    CU_ASSERT_EQUAL(len, sizeof(ti));
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_ESTABLISHED);
    CU_ASSERT(ti.tcpi_snd_mss > 0 && ti.tcpi_snd_mss < 65536);
    CU_ASSERT(ti.tcpi_snd_cwnd >= ti.tcpi_snd_mss);
    CU_ASSERT(ti.tcpi_rcv_space > 0);
    CU_ASSERT(ti.tcpi_snd_wnd > 0);
    CU_ASSERT_EQUAL(ti.tcpi_snd_una, ti.tcpi_snd_nxt);
    CU_ASSERT(ti.tcpi_rto >= 200000 && ti.tcpi_rto <= 64000000);
    printf("\n    rto %lu us, rtt %lu us, rttvar %lu us, mss %lu, options 0x%x\n",
           (unsigned long)ti.tcpi_rto, (unsigned long)ti.tcpi_rtt, (unsigned long)ti.tcpi_rttvar,
           (unsigned long)ti.tcpi_snd_mss, ti.tcpi_options);
    CloseSocket(c);
    CloseSocket(s);
}

void testTCPINFO_ESTABLISHED4(void) { established(AF_INET); }
void testTCPINFO_ESTABLISHED6(void) { established(AF_INET6); }

/* Data sent and read moves the sequence numbers by its length on both
 * sides, and all of it is acknowledged. */
void testTCPINFO_SEQUENCE(void)
{
    struct tcp_info a, b, a2, b2;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &a, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &b, NULL), 0);
    CU_ASSERT_EQUAL(st_oneway(c, s, 10000), 0);
    Delay(25);                  /* the last ACK, delayed */
    CU_ASSERT_EQUAL(info(c, &a2, NULL), 0);
    CU_ASSERT_EQUAL(info(s, &b2, NULL), 0);
    CU_ASSERT_EQUAL(a2.tcpi_snd_nxt - a.tcpi_snd_nxt, 10000);
    CU_ASSERT_EQUAL(a2.tcpi_snd_una, a2.tcpi_snd_max);
    CU_ASSERT_EQUAL(b2.tcpi_rcv_nxt - b.tcpi_rcv_nxt, 10000);
    CU_ASSERT_EQUAL(b2.tcpi_rcv_nxt, a2.tcpi_snd_nxt);
    CloseSocket(c);
    CloseSocket(s);
}

/* A short buffer gets the start of the structure, and its length. */
void testTCPINFO_SHORT(void)
{
    struct tcp_info ti;
    socklen_t len = 8;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET, &c, &s), 0);
    CU_ASSERT_EQUAL(info(c, &ti, &len), 0);
    CU_ASSERT_EQUAL(len, 8);
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_ESTABLISHED);
    CU_ASSERT_EQUAL(((unsigned char *)&ti)[8], 0xa5);    /* nothing past it */
    CloseSocket(c);
    CloseSocket(s);
}

/* A listening socket reports LISTEN. */
void testTCPINFO_LISTEN(void)
{
    struct tcp_info ti;
    unsigned short port;
    int l = st_bound(AF_INET, SOCK_STREAM, &port);

    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 1), 0);
    CU_ASSERT_EQUAL(info(l, &ti, NULL), 0);
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_LISTEN);
    CloseSocket(l);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TCPINFO tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TCPINFO_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an established connection (IPv4)", testTCPINFO_ESTABLISHED4) ||
        !CU_add_test(p, "an established connection (IPv6)", testTCPINFO_ESTABLISHED6) ||
        !CU_add_test(p, "data moves the sequence numbers", testTCPINFO_SEQUENCE) ||
        !CU_add_test(p, "a short buffer gets the start", testTCPINFO_SHORT) ||
        !CU_add_test(p, "a listening socket reports LISTEN", testTCPINFO_LISTEN))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TCPINFO");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
