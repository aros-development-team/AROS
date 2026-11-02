/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the localhost names (RFC 6761 section 6.3): "localhost"
    and every name ending in ".localhost", in any case and with or without
    the final dot, are the loopback addresses, whatever the host database
    and the resolver say - 127.0.0.1 and ::1 from getaddrinfo() (both for
    AF_UNSPEC, one each for AF_INET and AF_INET6) and 127.0.0.1 from
    gethostbyname() and gethostbyname_r(). Address literals are still
    taken as they are. Usage:  cunit-net-localhost
*/

#define __CONFIG_ROADSHOW__     /* getaddrinfo() */
#include "socktest.h"

#include <netdb.h>

struct Library *SocketBase = NULL;

static const struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;

/* What getaddrinfo(name, "7", family) gives: how many results, how many
 * are 127.0.0.1, how many ::1; -1 if it fails. */
static int lookup(const char *name, int family, int *n4, int *n6)
{
    struct addrinfo hints, *res = NULL, *ai;
    int n = 0;

    *n4 = *n6 = 0;
    bzero(&hints, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(name, "7", &hints, &res) != 0)
        return -1;
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        n++;
        if (ai->ai_family == AF_INET &&
                ((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr == htonl(INADDR_LOOPBACK) &&
                ((struct sockaddr_in *)ai->ai_addr)->sin_port == htons(7))
            (*n4)++;
        else if (ai->ai_family == AF_INET6 &&
                IN6_ARE_ADDR_EQUAL(&((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr, &lo6) &&
                ((struct sockaddr_in6 *)ai->ai_addr)->sin6_port == htons(7))
            (*n6)++;
    }
    freeaddrinfo(res);
    return n;
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* AF_UNSPEC: the two loopback addresses, and nothing else. */
void testLOCALHOST_UNSPEC(void)
{
    int n4, n6;

    CU_ASSERT_EQUAL(lookup("localhost", AF_UNSPEC, &n4, &n6), 2);
    CU_ASSERT_EQUAL(n4, 1);
    CU_ASSERT_EQUAL(n6, 1);
}

/* One family: its loopback address only. */
void testLOCALHOST_FAMILY(void)
{
    int n4, n6;

    CU_ASSERT_EQUAL(lookup("localhost", AF_INET, &n4, &n6), 1);
    CU_ASSERT_EQUAL(n4, 1);
    CU_ASSERT_EQUAL(lookup("localhost", AF_INET6, &n4, &n6), 1);
    CU_ASSERT_EQUAL(n6, 1);
}

/* Any case, the final dot, and names within .localhost. */
void testLOCALHOST_NAMES(void)
{
    static const char *names[] = { "LocalHost", "localhost.", "app.localhost",
                                   "a.b.LOCALHOST." };
    int i, n4, n6;

    for (i = 0; i < 4; i++) {
        CU_ASSERT_EQUAL(lookup(names[i], AF_UNSPEC, &n4, &n6), 2);
        CU_ASSERT_EQUAL(n4, 1);
        CU_ASSERT_EQUAL(n6, 1);
    }
}

/* gethostbyname(): 127.0.0.1. */
void testLOCALHOST_GETHOSTBYNAME(void)
{
    struct hostent *h = gethostbyname("localhost");

    CU_ASSERT_PTR_NOT_NULL_FATAL(h);
    CU_ASSERT_EQUAL(h->h_addrtype, AF_INET);
    CU_ASSERT_EQUAL(h->h_length, 4);
    CU_ASSERT_PTR_NOT_NULL_FATAL(h->h_addr_list[0]);
    CU_ASSERT_EQUAL(((struct in_addr *)h->h_addr_list[0])->s_addr, htonl(INADDR_LOOPBACK));
    CU_ASSERT_PTR_NULL(h->h_addr_list[1]);
}

/* gethostbyname_r(): 127.0.0.1, in the caller's buffer. */
void testLOCALHOST_GETHOSTBYNAME_R(void)
{
    struct hostent h, *r;
    char buf[256];
    LONG herr = -1;

    r = gethostbyname_r("localhost", &h, buf, sizeof(buf), &herr);
    CU_ASSERT_PTR_EQUAL_FATAL(r, &h);
    CU_ASSERT_EQUAL(herr, 0);
    CU_ASSERT_EQUAL(h.h_addrtype, AF_INET);
    CU_ASSERT_PTR_NOT_NULL_FATAL(h.h_addr_list[0]);
    CU_ASSERT_EQUAL(((struct in_addr *)h.h_addr_list[0])->s_addr, htonl(INADDR_LOOPBACK));
    CU_ASSERT_PTR_NULL(h.h_addr_list[1]);
    CU_ASSERT_STRING_EQUAL(h.h_name, "localhost");
}

/* Address literals are what they say, in each call. */
void testLOCALHOST_LITERALS(void)
{
    struct addrinfo hints, *res = NULL;
    struct in6_addr a6;
    struct hostent *h = gethostbyname("10.1.2.3");

    CU_ASSERT_PTR_NOT_NULL_FATAL(h);
    CU_ASSERT_EQUAL(h->h_addrtype, AF_INET);
    CU_ASSERT_EQUAL(((struct in_addr *)h->h_addr_list[0])->s_addr, htonl(0x0a010203));
    CU_ASSERT_STRING_EQUAL(h->h_name, "10.1.2.3");
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    CU_ASSERT_EQUAL_FATAL(getaddrinfo("2001:db8::7", NULL, &hints, &res), 0);
    inet_pton(AF_INET6, "2001:db8::7", &a6);
    CU_ASSERT(IN6_ARE_ADDR_EQUAL(&((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, &a6));
    CU_ASSERT_PTR_NULL(res->ai_next);
    freeaddrinfo(res);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping LOCALHOST tests\n");
        return 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("LOCALHOST_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "AF_UNSPEC gives 127.0.0.1 and ::1", testLOCALHOST_UNSPEC) ||
        !CU_add_test(p, "one family gives its loopback address", testLOCALHOST_FAMILY) ||
        !CU_add_test(p, "any case, the final dot, names within .localhost", testLOCALHOST_NAMES) ||
        !CU_add_test(p, "gethostbyname() gives 127.0.0.1", testLOCALHOST_GETHOSTBYNAME) ||
        !CU_add_test(p, "gethostbyname_r() gives 127.0.0.1", testLOCALHOST_GETHOSTBYNAME_R) ||
        !CU_add_test(p, "address literals are taken as they are", testLOCALHOST_LITERALS))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-LOCALHOST");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
