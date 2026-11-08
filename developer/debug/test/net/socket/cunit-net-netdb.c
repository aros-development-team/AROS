/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the networks database: a network's number is in host
    order (getnetent(3), and this library's autodoc), read as
    inet_network() reads it - so getnetbyaddr(127, AF_INET) finds the
    "loop 127 loopback" line AROS ships in db/networks, and
    getnetbyname("loopback") gives 127. And inet_network() itself: each
    part a byte, the last rightmost; five parts refused, without writing
    past the four it has room for. Usage:  cunit-net-netdb
*/

#include "socktest.h"

#include <netdb.h>

struct Library *SocketBase = NULL;

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* inet_network(): host order, parts right-justified. */
void testNETDB_INET_NETWORK(void)
{
    CU_ASSERT_EQUAL(inet_network("127"), 127);
    CU_ASSERT_EQUAL(inet_network("10.1"), 0x0a01);
    CU_ASSERT_EQUAL(inet_network("192.168.166"), 0xc0a8a6);
    CU_ASSERT_EQUAL(inet_network("1.2.3.4"), 0x01020304);
    CU_ASSERT_EQUAL(inet_network("0x7f"), 127);
    CU_ASSERT_EQUAL(inet_network("1.2.3.4.5"), INADDR_NONE);
    CU_ASSERT_EQUAL(inet_network("net"), INADDR_NONE);
}

/* getnetbyname(): the number in host order. */
void testNETDB_BYNAME(void)
{
    struct netent *n = getnetbyname("loopback");

    if (n == NULL) {
        CU_PASS("no loopback network in db/networks - skipping");
        return;
    }
    CU_ASSERT_EQUAL(n->n_addrtype, AF_INET);
    CU_ASSERT_EQUAL(n->n_net, 127);
}

/* getnetbyaddr(): found by the number in host order. */
void testNETDB_BYADDR(void)
{
    struct netent *n;

    if (getnetbyname("loopback") == NULL) {
        CU_PASS("no loopback network in db/networks - skipping");
        return;
    }
    n = getnetbyaddr(127, AF_INET);
    CU_ASSERT_PTR_NOT_NULL_FATAL(n);
    CU_ASSERT_STRING_EQUAL(n->n_name, "loop");
    CU_ASSERT_PTR_NULL(getnetbyaddr(127, AF_INET6));
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping NETDB tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NETDB_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "getnetbyname() gives the number in host order", testNETDB_BYNAME) ||
        !CU_add_test(p, "getnetbyaddr(127, AF_INET) finds the loopback network", testNETDB_BYADDR) ||
        !CU_add_test(p, "inet_network(): host order, parts right-justified", testNETDB_INET_NETWORK))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-NETDB");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
