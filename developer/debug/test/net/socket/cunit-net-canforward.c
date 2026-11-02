/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for which IPv4 destinations may be forwarded - the stack's
    in_canforward(), which IP forwarding and ICMP redirects use, through
    Roadshow's In_CanForward(): not network 0, not the loopback network
    127, not multicast or class E (224/3), not link-local 169.254/16 (RFC
    3927 section 2.7); any other unicast address, yes.
    Usage:  cunit-net-canforward
*/

#define __CONFIG_ROADSHOW__     /* In_CanForward() */
#include "socktest.h"

struct Library *SocketBase = NULL;

static LONG fwd(unsigned a, unsigned b, unsigned c, unsigned d)
{
    return In_CanForward(htonl((a << 24) | (b << 16) | (c << 8) | d));
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* Unicast destinations elsewhere may be forwarded. */
void testCANFORWARD_UNICAST(void)
{
    CU_ASSERT(fwd(8, 8, 8, 8));
    CU_ASSERT(fwd(10, 0, 0, 1));
    CU_ASSERT(fwd(192, 168, 1, 1));
    CU_ASSERT(fwd(126, 255, 255, 1));
    CU_ASSERT(fwd(128, 0, 0, 1));
}

/* The loopback network, all of it, may not. */
void testCANFORWARD_LOOPBACK(void)
{
    CU_ASSERT(!fwd(127, 0, 0, 1));
    CU_ASSERT(!fwd(127, 255, 0, 1));
}

/* Network 0, multicast, class E, the limited broadcast, link-local. */
void testCANFORWARD_RESERVED(void)
{
    CU_ASSERT(!fwd(0, 0, 0, 5));
    CU_ASSERT(!fwd(224, 0, 0, 1));
    CU_ASSERT(!fwd(239, 1, 1, 1));
    CU_ASSERT(!fwd(240, 0, 0, 1));
    CU_ASSERT(!fwd(255, 255, 255, 255));
    CU_ASSERT(!fwd(169, 254, 3, 4));
    CU_ASSERT(fwd(169, 253, 3, 4));     /* only 169.254/16 */
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping CANFORWARD tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("CANFORWARD_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "unicast destinations elsewhere may be forwarded", testCANFORWARD_UNICAST) ||
        !CU_add_test(p, "the loopback network may not", testCANFORWARD_LOOPBACK) ||
        !CU_add_test(p, "net 0, multicast, class E, link-local may not", testCANFORWARD_RESERVED))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-CANFORWARD");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
