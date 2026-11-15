/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for an IPv4 alias's netmask and broadcast address:
    SIOCGIFNETMASK, SIOCGIFBRDADDR (and SIOCGIFDSTADDR) answer for the
    address named in ifr_addr when the interface has it - as NetBSD's
    in_control() and IPv6's SIOCGIFNETMASK_IN6 do - and for the interface's
    first address otherwise: with nothing there, with what an earlier
    request left (a program asking SIOCGIFADDR, SIOCGIFNETMASK and
    SIOCGIFBRDADDR with one ifreq has the mask in it by the third), or with
    an address of another interface. SIOCGIFADDR is the first address
    whatever ifr_addr holds.

    The test adds 10.77.0.1/16 as an alias of IFNAME and deletes it at the
    end. Usage:  cunit-net-ifalias [IFNAME]  (default eth0; skipped without
    an IPv4 address on it).
*/

#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static struct in_addr first, firstmask, firstbrd, alias;
static int ready;

#define ALIAS      "10.77.0.1"
#define ALIASMASK  "255.255.0.0"
#define ALIASBRD   "10.77.255.255"

/* An ioctl on IFNAME with ifr_addr set to a (family AF_INET), or zeroed
 * when a is NULL: the address it gives, or INADDR_ANY (-1 on error). */
static int ask(int req, const struct in_addr *a, struct in_addr *out)
{
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    out->s_addr = INADDR_ANY;
    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (a != NULL) {
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_addr = *a;
    }
    r = IoctlSocket(s, req, (char *)&ifr);
    if (r == 0)
        *out = req == SIOCGIFBRDADDR ? ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr
                                     : sin->sin_addr;
    CloseSocket(s);
    return r;
}

/* Add (1) or delete (0) the alias, 10.77.0.1/16 with its broadcast. */
static int alias_set(int add)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct in_aliasreq ifra;

        bzero(&ifra, sizeof(ifra));
        snprintf(ifra.ifra_name, sizeof(ifra.ifra_name), "%s", ifname);
        ifra.ifra_addr.sin_len = ifra.ifra_mask.sin_len = ifra.ifra_broadaddr.sin_len =
            sizeof(struct sockaddr_in);
        ifra.ifra_addr.sin_family = ifra.ifra_mask.sin_family = ifra.ifra_broadaddr.sin_family =
            AF_INET;
        ifra.ifra_addr.sin_addr.s_addr = inet_addr(ALIAS);
        ifra.ifra_mask.sin_addr.s_addr = inet_addr(ALIASMASK);
        ifra.ifra_broadaddr.sin_addr.s_addr = inet_addr(ALIASBRD);
        r = IoctlSocket(s, SIOCAIFADDR, (char *)&ifra);
    } else {
        struct ifreq ifr;
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = inet_addr(ALIAS);
        r = IoctlSocket(s, SIOCDIFADDR, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no IPv4 address on the interface - skipping"); return; } } while (0)

int init_suite(void)
{
    if (SocketBase == NULL)
        return -1;
    ready = ask(SIOCGIFADDR, NULL, &first) == 0 && first.s_addr != INADDR_ANY &&
            ask(SIOCGIFNETMASK, NULL, &firstmask) == 0 &&
            ask(SIOCGIFBRDADDR, NULL, &firstbrd) == 0;
    if (ready && alias_set(1) != 0) {
        printf("%s: could not add %s - skipping\n", ifname, ALIAS);
        ready = 0;
    }
    alias.s_addr = inet_addr(ALIAS);
    return 0;
}

int clean_suite(void)
{
    if (ready)
        alias_set(0);
    return 0;
}

/* The alias named: its own mask and broadcast address. */
void testIFALIAS_NAMED(void)
{
    struct in_addr m, b;

    NEED_IF();
    CU_ASSERT_EQUAL(ask(SIOCGIFNETMASK, &alias, &m), 0);
    CU_ASSERT_EQUAL(m.s_addr, inet_addr(ALIASMASK));
    CU_ASSERT_EQUAL(ask(SIOCGIFBRDADDR, &alias, &b), 0);
    CU_ASSERT_EQUAL(b.s_addr, inet_addr(ALIASBRD));
}

/* The first address named, or nothing: the first address's. */
void testIFALIAS_FIRST(void)
{
    struct in_addr m, b;

    NEED_IF();
    CU_ASSERT_EQUAL(ask(SIOCGIFNETMASK, &first, &m), 0);
    CU_ASSERT_EQUAL(m.s_addr, firstmask.s_addr);
    CU_ASSERT_EQUAL(ask(SIOCGIFBRDADDR, &first, &b), 0);
    CU_ASSERT_EQUAL(b.s_addr, firstbrd.s_addr);
    CU_ASSERT_EQUAL(ask(SIOCGIFNETMASK, NULL, &m), 0);
    CU_ASSERT_EQUAL(m.s_addr, firstmask.s_addr);
    CU_ASSERT_EQUAL(ask(SIOCGIFBRDADDR, NULL, &b), 0);
    CU_ASSERT_EQUAL(b.s_addr, firstbrd.s_addr);
}

/* What is not one of the interface's addresses - the mask an earlier
 * request left, another interface's address - names the first, as before:
 * no EADDRNOTAVAIL for programs that reuse one ifreq. */
void testIFALIAS_OTHER(void)
{
    struct in_addr m, b, lo;

    NEED_IF();
    lo.s_addr = htonl(INADDR_LOOPBACK);
    CU_ASSERT_EQUAL(ask(SIOCGIFBRDADDR, &firstmask, &b), 0);
    CU_ASSERT_EQUAL(b.s_addr, firstbrd.s_addr);
    CU_ASSERT_EQUAL(ask(SIOCGIFNETMASK, &lo, &m), 0);
    CU_ASSERT_EQUAL(m.s_addr, firstmask.s_addr);
}

/* SIOCGIFADDR is the first address whatever ifr_addr names. */
void testIFALIAS_ADDR(void)
{
    struct in_addr a;

    NEED_IF();
    CU_ASSERT_EQUAL(ask(SIOCGIFADDR, &alias, &a), 0);
    CU_ASSERT_EQUAL(a.s_addr, first.s_addr);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping IFALIAS tests\n");
        return 0;
    }
    if (st_skip_program("IFALIAS", ST_V4)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("IFALIAS_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an alias named: its own mask and broadcast", testIFALIAS_NAMED) ||
        !CU_add_test(p, "the first address named, or none: the first's", testIFALIAS_FIRST) ||
        !CU_add_test(p, "a mask or another interface's address: the first's", testIFALIAS_OTHER) ||
        !CU_add_test(p, "SIOCGIFADDR: the first address", testIFALIAS_ADDR))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-IFALIAS");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
