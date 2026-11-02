/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the resolver's search list (resolv.conf(5), BIND 4.9's
    res_search()): a name without a dot is tried in each domain of the
    list in turn, not only in the first; a name with a dot is tried as it
    is first, then in each domain. The domains and the server are given
    as a DHCP client gives them: the domains through miami.library's
    AddDynDomain(), which leaves the dynamic search domains cleared at the
    end (a DHCP client gives its own again when it renews); the server,
    the test's own (dnstest.h) at 127.0.0.1, through AddDomainNameServer().

    A server the resolver already has and asks first would answer "no such
    name" for the test's names itself; then the tests are skipped.
    Usage:  cunit-net-resolver
*/

#define __CONFIG_ROADSHOW__     /* AddDomainNameServer() */
#include "socktest.h"
#include "dnstest.h"

#include <defines/miami.h>   /* only its calls: clib/miami_protos.h clashes with bsdsocket.h */

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

static int ready;

#define NEED_SERVER() do { if (!ready) { CU_PASS("no server of our own - skipping"); return; } } while (0)

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A name without a dot: each domain in turn. */
void testRESOLVER_SEARCH(void)
{
    NEED_SERVER();
    CU_ASSERT_EQUAL(dns_lookup("host"), 79);
}

/* A name with a dot: as it is first. */
void testRESOLVER_AS_IS_FIRST(void)
{
    NEED_SERVER();
    CU_ASSERT_EQUAL(dns_lookup("www.search.test"), 80);
}

/* A name with a dot not found as it is: each domain in turn. */
void testRESOLVER_DOTTED_SEARCH(void)
{
    NEED_SERVER();
    CU_ASSERT_EQUAL(dns_lookup("x.y"), 82);
}

/* A name with a trailing dot: only as it is. */
void testRESOLVER_TRAILING_DOT(void)
{
    NEED_SERVER();
    CU_ASSERT_EQUAL(dns_lookup("www.search.test."), 80);
    CU_ASSERT_EQUAL(dns_lookup("x.y."), 0);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int server = 0;

    if (argc > 1 && strcmp(argv[1], "--serve") == 0)
        return dns_serve();
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping RESOLVER tests\n");
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    if (MiamiBase == NULL)
        printf("miami.library not available - skipping RESOLVER tests\n");
    else if (dns_other_server())
        printf("another DNS server answers first - skipping RESOLVER tests\n");
    else if (!(server = dns_start(argv[0])))
        printf("the test's DNS server did not start - skipping RESOLVER tests\n");
    else {
        AddDomainNameServer("127.0.0.1");
        AddDynDomain("a.search.test");
        AddDynDomain("b.search.test");
        EndDynDomain();
        ready = 1;
    }

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("RESOLVER_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a name without a dot is tried in each domain", testRESOLVER_SEARCH) ||
        !CU_add_test(p, "a name with a dot is tried as it is first", testRESOLVER_AS_IS_FIRST) ||
        !CU_add_test(p, "then in each domain", testRESOLVER_DOTTED_SEARCH) ||
        !CU_add_test(p, "a name with a trailing dot only as it is", testRESOLVER_TRAILING_DOT))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-RESOLVER");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    if (ready) {
        RemoveDomainNameServer("127.0.0.1");
        ClearDynDomain();
        EndDynDomain();
    }
    if (server)
        dns_stop();
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
