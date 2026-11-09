/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the SBTC_SIG_ADDRESS_CHANGE_MASK signal: it comes for
    every change of the interfaces' addresses, so a program can wait for it
    instead of polling - an IPv4 alias added and deleted, an IPv6 address
    added and deleted, and an address stateless autoconfiguration makes
    from a Router Advertisement (the test sends it itself to the
    interface's own link-local address, as cunit-net-slaac6 does).
    Usage:  cunit-net-addrsig [IFNAME]  (default eth0). Without an
    interface with a link-local address the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton(), SocketBaseTags() */
#include "socktest.h"

#include <proto/dos.h>
#include <bsdsocket/socketbasetags.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;
static ULONG sigmask;

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

static int get_linklocal(struct in6_addr *ll)
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
        int k, n = ifc.ifc6_len / sizeof(req[0]);

        for (k = 0; k < n && !found; k++)
            if (strcmp(req[k].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[k].ifr_ifru.ifru_addr.sin6_addr)) {
                *ll = req[k].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

/* Add (add 1) or delete (add 0) addr/64 on the interface. */
static int addr6(int add, const struct in6_addr *addr)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;
        int i;

        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        ifra.ifra_addr.sin6_len = sizeof(ifra.ifra_addr);
        ifra.ifra_addr.sin6_family = AF_INET6;
        ifra.ifra_addr.sin6_addr = *addr;
        ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
        ifra.ifra_prefixmask.sin6_family = AF_INET6;
        for (i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;

        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
        ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
        ifr.ifr_ifru.ifru_addr.sin6_addr = *addr;
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* Wait up to secs for the address change signal; whether it came. */
static int wait_signal(int secs)
{
    int i;

    for (i = 0; i < secs * 10; i++) {
        if (SetSignal(0, sigmask) & sigmask) {
            SetSignal(0, sigmask);
            return 1;
        }
        Delay(5);
    }
    return 0;
}

/* Add (add 1) or delete (add 0) the IPv4 alias addr/24 on the interface. */
static int addr4(int add, const char *addr)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct ifaliasreq ifra;
        struct sockaddr_in *a;

        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        a = (struct sockaddr_in *)&ifra.ifra_addr;
        a->sin_len = sizeof(*a);
        a->sin_family = AF_INET;
        a->sin_addr.s_addr = inet_addr((char *)addr);
        a = (struct sockaddr_in *)&ifra.ifra_mask;
        a->sin_len = sizeof(*a);
        a->sin_family = AF_INET;
        a->sin_addr.s_addr = htonl(0xffffff00);
        r = IoctlSocket(s, SIOCAIFADDR, (char *)&ifra);
    } else {
        struct ifreq ifr;
        struct sockaddr_in *a = (struct sockaddr_in *)&ifr.ifr_addr;

        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        a->sin_len = sizeof(*a);
        a->sin_family = AF_INET;
        a->sin_addr.s_addr = inet_addr((char *)addr);
        r = IoctlSocket(s, SIOCDIFADDR, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* A Router Advertisement, router lifetime 0, with prefix/64 autonomous. */
static int send_ra(const char *prefix, u_int32_t valid)
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
    pi->nd_opt_pi_preferred_time = htonl(valid);
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
    return n == (int)sizeof(pkt) ? 0 : -1;
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* An IPv4 alias added, then deleted: a signal each time. */
void testADDRSIG_IPV4(void)
{
    NEED_IF();
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL_FATAL(addr4(1, "192.168.166.77"), 0);
    CU_ASSERT(wait_signal(2));
    CU_ASSERT_EQUAL(addr4(0, "192.168.166.77"), 0);
    CU_ASSERT(wait_signal(2));
}

/* An IPv6 address added, then deleted: a signal each time. */
void testADDRSIG_IPV6(void)
{
    struct in6_addr a;

    NEED_IF();
    inet_pton(AF_INET6, "2001:db8:4d54:c1::1", &a);
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL_FATAL(addr6(1, &a), 0);
    CU_ASSERT(wait_signal(2));
    Delay(3 * TICKS_PER_SECOND);        /* DAD, and the signal at its end */
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL(addr6(0, &a), 0);
    CU_ASSERT(wait_signal(2));
}

/* An address SLAAC makes from an advertisement: a signal. */
void testADDRSIG_SLAAC(void)
{
    struct in6_addr a;

    NEED_IF();
    inet_pton(AF_INET6, "2001:db8:4d54:c2::", &a);
    memcpy(&a.s6_addr[8], &linklocal.s6_addr[8], 8);
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL_FATAL(send_ra("2001:db8:4d54:c2::", 600), 0);
    CU_ASSERT(wait_signal(2));
    Delay(3 * TICKS_PER_SECOND);
    send_ra("2001:db8:4d54:c2::", 0);
    addr6(0, &a);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    BYTE sig = -1;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping ADDRSIG tests\n");
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && get_linklocal(&linklocal) &&
            (sig = AllocSignal(-1)) != -1;
    if (ready) {
        sigmask = 1UL << sig;
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), sigmask, TAG_END);
    } else
        printf("%s: no interface with a link-local address - skipping ADDRSIG tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("ADDRSIG_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an IPv4 alias added and deleted", testADDRSIG_IPV4) ||
        !CU_add_test(p, "an IPv6 address added and deleted", testADDRSIG_IPV6) ||
        !CU_add_test(p, "an address SLAAC makes", testADDRSIG_SLAAC))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ADDRSIG");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    if (sig != -1) {
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0, TAG_END);
        FreeSignal(sig);
    }
    CloseLibrary(SocketBase);
    return CU_get_error();
}
