/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the net.inet.tcp knobs MiamiSysCtl() reads and sets
    (the 4.4BSD sysctl() MIB): each has its default, takes a new value and
    gives it back, and an unknown leaf is refused. Usage:  cunit-net-sysctl
*/

#include "socktest.h"

#include <proto/miami.h>
#include <sys/sysctl.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

/* Read net.inet.tcp.<leaf> into *val, setting it to *nval first if nval;
 * 0 or the error. */
static int tcpctl(int leaf, LONG *val, LONG *nval)
{
    LONG name[4] = { CTL_NET, PF_INET, IPPROTO_TCP, 0 };
    LONG len = sizeof(*val);

    name[3] = leaf;
    return MiamiSysCtl(name, 4, val, &len, nval, nval ? sizeof(*nval) : 0, 0);
}

/* The knob's default is dflt; it takes 0 and dflt back. */
static void knob(int leaf, LONG dflt)
{
    LONG v = -1, n = 0;

    CU_ASSERT_EQUAL(tcpctl(leaf, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, dflt);
    CU_ASSERT_EQUAL(tcpctl(leaf, &v, &n), 0);
    CU_ASSERT_EQUAL(v, dflt);           /* the old value */
    CU_ASSERT_EQUAL(tcpctl(leaf, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 0);
    n = dflt;
    CU_ASSERT_EQUAL(tcpctl(leaf, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(leaf, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, dflt);
}

int init_suite(void)
{
    return MiamiBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* IPv4 path MTU discovery: DF on TCP segments (RFC 1191). */
void testSYSCTL_PMTUD(void)
{
    knob(TCPCTL_PMTUD, 1);
}

/* Path MTU black-hole detection (RFC 2923 section 2.1). */
void testSYSCTL_PMTUD_BLACKHOLE(void)
{
    knob(TCPCTL_PMTUD_BLACKHOLE, 1);
}

/* ECN (RFC 3168): 2, accept it when the peer asks; 0 to 2 only. */
void testSYSCTL_ECN(void)
{
    LONG v = -1, n = 3;

    knob(TCPCTL_ECN, 2);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ECN, &v, &n), EINVAL);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ECN, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 2);
}

/* Proportional rate reduction (RFC 6937) in loss recovery with SACK: off;
 * it takes 1 and gives it back. */
void testSYSCTL_PRR(void)
{
    LONG v = -1, n = 1;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_PRR, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_PRR, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_PRR, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1);
    n = 0;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_PRR, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_PRR, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 0);
}

/* The least retransmission timeout, ms: 200 (as FreeBSD and Linux, below
 * RFC 6298's 1 s); it takes 1000 and gives it back; not 0. */
void testSYSCTL_REXMIT_MIN(void)
{
    LONG v = -1, n = 1000;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 200);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1000);
    n = 0;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, &n), EINVAL);
    n = 200;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_REXMIT_MIN, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 200);
}

/* Slow start counts the bytes an ACK acknowledges, at most L segments
 * (RFC 3465): 1 by default (RFC 5681's min(N, SMSS)); 1 to 16. */
void testSYSCTL_ABC_L(void)
{
    LONG v = -1, n = 0;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, &n), EINVAL);
    n = 17;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, &n), EINVAL);
    n = 2;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, &n), 0);
    n = 1;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_ABC_L, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1);
}

/* HyStart++ (RFC 9406) in the first slow start: on; 0 or 1. */
void testSYSCTL_HYSTART(void)
{
    LONG v = -1, n = 2;

    knob(TCPCTL_HYSTART, 1);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_HYSTART, &v, &n), EINVAL);
}

/* The least MSS a connection uses, bytes: 216 (TCP_MINMSS, FreeBSD's);
 * 0 (the floor of 64) to 1460. */
void testSYSCTL_MINMSS(void)
{
    LONG v = -1, n = 1461;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 216);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &v, &n), EINVAL);
    n = 0;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &v, &n), 0);
    n = 216;
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &v, &n), 0);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MINMSS, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 216);
}

/* The tail loss probe (RFC 8985 section 7): on; 0 or 1. */
void testSYSCTL_TLP(void)
{
    LONG v = -1, n = 2;

    knob(TCPCTL_TLP, 1);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_TLP, &v, &n), EINVAL);
}

/* RACK loss detection (RFC 8985 section 6): on; 0 or 1. */
void testSYSCTL_RACK(void)
{
    LONG v = -1, n = 2;

    knob(TCPCTL_RACK, 1);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_RACK, &v, &n), EINVAL);
}

/* The initial retransmission timeout, s: 1 (RFC 6298 section 2.1). */
void testSYSCTL_RTTDFLT(void)
{
    LONG v = -1, n = 0;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_RTTDFLT, &v, NULL), 0);
    CU_ASSERT_EQUAL(v, 1);
    CU_ASSERT_EQUAL(tcpctl(TCPCTL_RTTDFLT, &v, &n), EINVAL);
}

/* A leaf that does not exist. */
void testSYSCTL_UNKNOWN(void)
{
    LONG v = 0;

    CU_ASSERT_EQUAL(tcpctl(TCPCTL_MAXID + 20, &v, NULL), ENOPROTOOPT);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    MiamiBase = OpenLibrary("miami.library", 0);
    if (SocketBase == NULL || MiamiBase == NULL) {
        printf("bsdsocket.library or miami.library not available - skipping SYSCTL tests\n");
        if (MiamiBase)
            CloseLibrary(MiamiBase);
        if (SocketBase)
            CloseLibrary(SocketBase);
        return 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SYSCTL_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "net.inet.tcp.pmtud", testSYSCTL_PMTUD) ||
        !CU_add_test(p, "net.inet.tcp.pmtud_blackhole", testSYSCTL_PMTUD_BLACKHOLE) ||
        !CU_add_test(p, "net.inet.tcp.ecn", testSYSCTL_ECN) ||
        !CU_add_test(p, "net.inet.tcp.prr", testSYSCTL_PRR) ||
        !CU_add_test(p, "net.inet.tcp.rexmit_min", testSYSCTL_REXMIT_MIN) ||
        !CU_add_test(p, "net.inet.tcp.rttdflt", testSYSCTL_RTTDFLT) ||
        !CU_add_test(p, "net.inet.tcp.abc_l", testSYSCTL_ABC_L) ||
        !CU_add_test(p, "net.inet.tcp.hystart", testSYSCTL_HYSTART) ||
        !CU_add_test(p, "net.inet.tcp.minmss", testSYSCTL_MINMSS) ||
        !CU_add_test(p, "net.inet.tcp.tlp", testSYSCTL_TLP) ||
        !CU_add_test(p, "net.inet.tcp.rack", testSYSCTL_RACK) ||
        !CU_add_test(p, "an unknown leaf is refused", testSYSCTL_UNKNOWN))
    {
        CU_cleanup_registry();
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SYSCTL");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
