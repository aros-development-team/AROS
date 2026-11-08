/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the lifetimes of addresses made by stateless address
    autoconfiguration (RFC 4862 sections 5.5.3 and 5.5.4): an address made
    from a prefix lives as long as the prefix's valid lifetime, and each
    Router Advertisement for the prefix renews it - but one that would cut
    it below two hours can only bring it down to two hours, so that a forged
    advertisement cannot take a host's address away (section 5.5.3 (e)).
    An address deleted by hand comes back with the next advertisement.

    As cunit-net-slaac6, the test sends the Router Advertisements itself to
    the interface's own link-local address, router lifetime 0, one prefix
    per case. Usage:  cunit-net-slaaclife [IFNAME]  (default eth0). Without
    an interface with a link-local address the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

static int index_of(const char *name, unsigned *index)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    r = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr);
    CloseSocket(s);
    if (r != 0)
        return -1;
    *index = ifr.ifr_index;
    return 0;
}

/* Whether the interface has addr; with addr NULL, its link-local into *ll. */
static int have_addr(const struct in6_addr *addr, struct in6_addr *ll)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), found = 0;

    if (s < 0)
        return 0;
    bzero(req, sizeof(req));
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        int i, n = ifc.ifc6_len / sizeof(req[0]);

        for (i = 0; i < n && !found; i++) {
            const struct in6_addr *a = &req[i].ifr_ifru.ifru_addr.sin6_addr;

            if (strcmp(req[i].ifr_name, ifname) != 0)
                continue;
            if (addr == NULL && IN6_IS_ADDR_LINKLOCAL(a)) {
                *ll = *a;
                found = 1;
            } else if (addr != NULL && memcmp(a, addr, sizeof(*a)) == 0)
                found = 1;
        }
    }
    CloseSocket(s);
    return found;
}

/* The address SLAAC forms from prefix and the link-local's interface id. */
static void slaac_addr(const char *prefix, struct in6_addr *a)
{
    inet_pton(AF_INET6, (char *)prefix, a);
    memcpy(&a->s6_addr[8], &linklocal.s6_addr[8], 8);
}

/* A Router Advertisement, router lifetime 0, with one Prefix Information
 * Option for prefix/64 (on-link, autonomous), to our own link-local. */
static int send_ra(const char *prefix, u_int32_t valid, u_int32_t preferred)
{
    u_char pkt[sizeof(struct nd_router_advert) + sizeof(struct nd_opt_prefix_info)];
    struct nd_router_advert *ra = (struct nd_router_advert *)pkt;
    struct nd_opt_prefix_info *pi = (struct nd_opt_prefix_info *)(ra + 1);
    struct sockaddr_in6 a;
    int s, hops = 255, n;

    bzero(pkt, sizeof(pkt));
    ra->nd_ra_type = ND_ROUTER_ADVERT;
    pi->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
    pi->nd_opt_pi_len = 4;
    pi->nd_opt_pi_prefix_len = 64;
    pi->nd_opt_pi_flags_reserved = ND_OPT_PI_FLAG_ONLINK | ND_OPT_PI_FLAG_AUTO;
    pi->nd_opt_pi_valid_time = htonl(valid);
    pi->nd_opt_pi_preferred_time = htonl(preferred);
    inet_pton(AF_INET6, (char *)prefix, &pi->nd_opt_pi_prefix);

    s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (s < 0)
        return -1;
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = linklocal;
    a.sin6_scope_id = ifindex;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) != 0) {
        CloseSocket(s);
        return -1;
    }
    n = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(10);
    return n == (int)sizeof(pkt) ? 0 : -1;
}

/* Wait up to secs for addr to be there (want 1) or gone (want 0). */
static int wait_for(const struct in6_addr *addr, int want, int secs)
{
    int i;

    for (i = 0; i < secs * 5; i++) {
        if (have_addr(addr, NULL) == want)
            return 1;
        Delay(10);
    }
    return 0;
}

/* Delete an address of the interface. */
static int del_addr(const struct in6_addr *addr)
{
    struct in6_ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
    ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
    ifr.ifr_ifru.ifru_addr.sin6_addr = *addr;
    r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    CloseSocket(s);
    return r;
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* Deleted by hand, the address comes back with the next advertisement. */
void testSLAACLIFE_DELETED_RETURNS(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:30::", &a);
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:30::", 600, 600), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT_EQUAL(del_addr(&a), 0);
    CU_ASSERT(wait_for(&a, 0, 2));
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:30::", 600, 600), 0);
    CU_ASSERT(wait_for(&a, 1, 5));
    del_addr(&a);
}

/* An address lives as long as its prefix's valid lifetime: 4 s. */
void testSLAACLIFE_EXPIRES(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:31::", &a);
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:31::", 4, 2), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT(wait_for(&a, 0, 8));
    del_addr(&a);
}

/* An advertisement before the end renews it. */
void testSLAACLIFE_RENEWED(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:32::", &a);
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:32::", 4, 2), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:32::", 3600, 3600), 0);
    Delay(7 * TICKS_PER_SECOND);
    CU_ASSERT(have_addr(&a, NULL));
    del_addr(&a);
}

/* One that would cut it below two hours does not end it (5.5.3 (e)). */
void testSLAACLIFE_TWO_HOURS(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:33::", &a);
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:33::", 86400, 86400), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT_EQUAL(send_ra("2001:db8:4d54:33::", 3, 1), 0);
    Delay(6 * TICKS_PER_SECOND);
    CU_ASSERT(have_addr(&a, NULL));
    del_addr(&a);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SLAACLIFE tests\n");
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && have_addr(NULL, &linklocal);
    if (!ready)
        printf("%s: no interface with a link-local address - skipping SLAACLIFE tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SLAACLIFE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an address deleted by hand comes back", testSLAACLIFE_DELETED_RETURNS) ||
        !CU_add_test(p, "an address ends with its valid lifetime", testSLAACLIFE_EXPIRES) ||
        !CU_add_test(p, "an advertisement before the end renews it", testSLAACLIFE_RENEWED) ||
        !CU_add_test(p, "one below two hours does not end it", testSLAACLIFE_TWO_HOURS))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SLAACLIFE");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
