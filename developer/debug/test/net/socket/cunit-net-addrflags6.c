/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the state of an IPv6 address as applications see it
    (RFC 4862 section 5.4: a tentative address must not be used yet):
    SIOCGIFAFLAG_IN6 (KAME) gives an address's IN6_IFF_* flags, and the
    SBTC_SIG_ADDRESS_CHANGE_MASK signal comes when duplicate address
    detection ends, after the flags have changed - so a program woken by it
    finds a new address no longer tentative, or a duplicated one
    IN6_IFF_DUPLICATED. An address no one has gives EADDRNOTAVAIL.

    The test adds addresses on the interface and deletes them again; the
    duplicate is made by a Neighbor Advertisement for the tentative address,
    sent by the test itself from the interface's link-local address to
    itself. Usage:  cunit-net-addrflags6 [IFNAME]  (default eth0). Without
    an interface with a link-local address the tests are skipped.
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

/* SIOCGIFAFLAG_IN6 for addr: its flags, or -1 (with errno). */
static int aflags(const struct in6_addr *addr)
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
    r = IoctlSocket(s, SIOCGIFAFLAG_IN6, (char *)&ifr);
    CloseSocket(s);
    return r == 0 ? ifr.ifr_ifru.ifru_flags6 : -1;
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

/* A Neighbor Advertisement for target, from our link-local to itself:
 * someone else has the address. */
static int send_na(const struct in6_addr *target)
{
    struct nd_neighbor_advert na;
    struct sockaddr_in6 a;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), hops = 255, n;

    if (s < 0)
        return -1;
    bzero(&na, sizeof(na));
    na.nd_na_type = ND_NEIGHBOR_ADVERT;
    na.nd_na_flags_reserved = ND_NA_FLAG_OVERRIDE;
    na.nd_na_target = *target;
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
    n = sendto(s, (char *)&na, sizeof(na), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    return n == (int)sizeof(na) ? 0 : -1;
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The link-local, settled: no TENTATIVE, no DUPLICATED. */
void testADDRFLAGS6_LINKLOCAL(void)
{
    int f;

    NEED_IF();
    f = aflags(&linklocal);
    CU_ASSERT_FATAL(f >= 0);
    CU_ASSERT_EQUAL(f & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED), 0);
}

/* A new address: TENTATIVE, then the signal, then not TENTATIVE. */
void testADDRFLAGS6_DAD_PASSES(void)
{
    struct in6_addr a;
    int f;

    NEED_IF();
    inet_pton(AF_INET6, "2001:db8:4d54:c0::1", &a);
    CU_ASSERT_EQUAL_FATAL(addr6(1, &a), 0);
    f = aflags(&a);
    CU_ASSERT(f >= 0 && (f & IN6_IFF_TENTATIVE));
    SetSignal(0, sigmask);              /* the one for adding it */
    CU_ASSERT(wait_signal(5));
    f = aflags(&a);
    CU_ASSERT(f >= 0 && !(f & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED)));
    addr6(0, &a);
}

/* Someone else has it: the signal, then DUPLICATED. */
void testADDRFLAGS6_DAD_DUPLICATE(void)
{
    struct in6_addr a;
    int f;

    NEED_IF();
    inet_pton(AF_INET6, "2001:db8:4d54:c0::2", &a);
    CU_ASSERT_EQUAL_FATAL(addr6(1, &a), 0);
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL(send_na(&a), 0);
    CU_ASSERT(wait_signal(5));
    f = aflags(&a);
    CU_ASSERT(f >= 0 && (f & IN6_IFF_DUPLICATED) && !(f & IN6_IFF_TENTATIVE));
    addr6(0, &a);
}

/* An address no one has: EADDRNOTAVAIL. */
void testADDRFLAGS6_UNKNOWN(void)
{
    struct in6_addr a;

    NEED_IF();
    inet_pton(AF_INET6, "2001:db8:4d54:c0::99", &a);
    CU_ASSERT_EQUAL(aflags(&a), -1);
    CU_ASSERT_EQUAL(Errno(), EADDRNOTAVAIL);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    BYTE sig = -1;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping ADDRFLAGS6 tests\n");
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && get_linklocal(&linklocal) &&
            (sig = AllocSignal(-1)) != -1;
    if (ready) {
        sigmask = 1UL << sig;
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), sigmask, TAG_END);
    } else
        printf("%s: no interface with a link-local address - skipping ADDRFLAGS6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("ADDRFLAGS6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the link-local is settled", testADDRFLAGS6_LINKLOCAL) ||
        !CU_add_test(p, "a new address: tentative, the signal, usable", testADDRFLAGS6_DAD_PASSES) ||
        !CU_add_test(p, "a duplicate: the signal, duplicated", testADDRFLAGS6_DAD_DUPLICATE) ||
        !CU_add_test(p, "an address no one has: EADDRNOTAVAIL", testADDRFLAGS6_UNKNOWN))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ADDRFLAGS6");
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
