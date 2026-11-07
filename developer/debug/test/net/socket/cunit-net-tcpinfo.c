/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for TCP_INFO (Linux's option, FreeBSD's struct tcp_info):
    what getsockopt() gives of a connection - its state, the options it
    negotiated, its maximum segment size, windows and sequence numbers,
    and its round-trip estimate and retransmission timeout, in
    microseconds. A short buffer gets the start of the structure; a
    listening socket reports LISTEN. Over the loopback. Usage:
    cunit-net-tcpinfo [IFNAME]  (default eth0; the SYN backoff test needs
    an interface, the others the loopback)
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>

#include <proto/dos.h>
#include <netinet/tcp_fsm.h>

struct Library *SocketBase = NULL;
static const char *ifname = "eth0";

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

/* Over the loopback, after some exchanges, the RTT is measured in
 * milliseconds (well under 100 ms) and the RTO is the floor,
 * net.inet.tcp.rexmit_min (200 ms by default), not whole seconds. */
void testTCPINFO_RTO_FLOOR(void)
{
    struct tcp_info ti;
    char b[100];
    int c, s, i;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(AF_INET6, &c, &s), 0);
    for (i = 0; i < 20; i++) {
        CU_ASSERT_EQUAL(send(c, b, sizeof(b), 0), sizeof(b));
        CU_ASSERT_EQUAL(recv(s, b, sizeof(b), MSG_WAITALL), sizeof(b));
        CU_ASSERT_EQUAL(send(s, b, sizeof(b), 0), sizeof(b));
        CU_ASSERT_EQUAL(recv(c, b, sizeof(b), MSG_WAITALL), sizeof(b));
    }
    CU_ASSERT_EQUAL(info(c, &ti, NULL), 0);
    printf("\n    after 20 exchanges: rto %lu us, rtt %lu us, rttvar %lu us\n",
           (unsigned long)ti.tcpi_rto, (unsigned long)ti.tcpi_rtt, (unsigned long)ti.tcpi_rttvar);
    CU_ASSERT(ti.tcpi_rtt < 100000);
    CU_ASSERT_EQUAL(ti.tcpi_rto, 200000);
    CloseSocket(c);
    CloseSocket(s);
}

/* A SYN nobody answers is resent after 1 s, then 2 s, then 4 s (RFC 6298
 * sections 2.1 and 5.5): the RTO doubles from the initial one, it does not
 * fall to the floor once the first SYN timed out. The SYN goes to a
 * link-local neighbour that does not exist, on IFNAME (cunit-net-tcpinfo
 * [IFNAME], default eth0); without the interface the test is skipped. */
static unsigned index_of(const char *name)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    r = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr);
    CloseSocket(s);
    return r == 0 ? ifr.ifr_index : 0;
}

void testTCPINFO_SYN_BACKOFF(void)
{
    struct tcp_info ti;
    struct sockaddr_in6 a;
    int s, on = 1;
    unsigned idx = index_of(ifname);
    unsigned long rto1, rto2;

    if (idx == 0) {
        CU_PASS("no interface - skipping");
        return;
    }
    s = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    IoctlSocket(s, FIONBIO, (char *)&on);
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_port = htons(9);
    inet_pton(AF_INET6, "fe80::4d54:dead", &a.sin6_addr);
    a.sin6_scope_id = idx;
    connect(s, (struct sockaddr *)&a, sizeof(a));
    Delay(75);                          /* 1.5 s: one timeout */
    info(s, &ti, NULL);
    rto1 = ti.tcpi_rto;
    CU_ASSERT_EQUAL(ti.tcpi_state, TCPS_SYN_SENT);
    Delay(125);                         /* 4 s: two */
    info(s, &ti, NULL);
    rto2 = ti.tcpi_rto;
    printf("\n    SYN unanswered: RTO %lu us after one timeout, %lu us after two\n", rto1, rto2);
    CU_ASSERT_EQUAL(rto1, 2000000);
    CU_ASSERT_EQUAL(rto2, 4000000);
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

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
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
        !CU_add_test(p, "over the loopback the RTO is the floor", testTCPINFO_RTO_FLOOR) ||
        !CU_add_test(p, "an unanswered SYN backs off from 1 s", testTCPINFO_SYN_BACKOFF) ||
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
