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
