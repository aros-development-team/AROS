/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the DNS options of Router Advertisements (RFC 8106):
    the servers of an RDNSS option and the domains of a DNSSL option are
    the resolver's for their lifetime - gethostbyname() asks a server it
    learned that way, a name without dots is tried in a domain it learned -
    a lifetime of 0 ends them at once, and they end by themselves when the
    lifetime is up. An option that does not parse is ignored. A DHCP update
    of the dynamic servers (miami.library's ClearDynNameServ()) leaves them.

    The test sends the advertisements itself to the interface's own
    link-local address, router lifetime 0, as cunit-net-slaac6 does; the
    server they name is that link-local address, where the test runs a
    small DNS server of its own (dnstest.h) that knows one.rdnss.test and
    two.rdnss.test.

    A server the resolver already has and asks first would answer "no such
    name" for these names itself; then the tests are skipped.
    Usage:  cunit-net-rdnss [IFNAME]  (default eth0)
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"
#include "dnstest.h"

#include <defines/miami.h>   /* only its calls: clib/miami_protos.h clashes with bsdsocket.h */
#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;

#define LIFETIME    30

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface or server - skipping"); return; } } while (0)

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

/* The interface's link-local address into *ll. */
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
        int i, n = ifc.ifc6_len / sizeof(req[0]);

        for (i = 0; i < n && !found; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                *ll = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

/* A Router Advertisement, router lifetime 0, with the options given. */
static unsigned char ra[256];
static int ralen;

static void ra_begin(void)
{
    struct nd_router_advert *h = (struct nd_router_advert *)ra;

    bzero(ra, sizeof(ra));
    h->nd_ra_type = ND_ROUTER_ADVERT;
    ralen = sizeof(*h);
}

/* An option of type, units of 8 octets long, the lifetime at offset 4. */
static unsigned char *ra_opt(int type, int units, u_int32_t lifetime)
{
    unsigned char *o = ra + ralen;

    lifetime = htonl(lifetime);
    o[0] = type;
    o[1] = units;
    memcpy(o + 4, &lifetime, 4);
    ralen += units * 8;
    return o;
}

/* RDNSS with our link-local address; extra units of padding make it bad. */
static void ra_rdnss(u_int32_t lifetime, int extra)
{
    unsigned char *o = ra_opt(ND_OPT_RDNSS, 3 + extra, lifetime);

    memcpy(o + 8, &linklocal, sizeof(linklocal));
}

/* DNSSL with one domain given already in the DNS's label encoding. */
static void ra_dnssl(const unsigned char *labels, int len, u_int32_t lifetime)
{
    unsigned char *o = ra_opt(ND_OPT_DNSSL, (8 + len + 7) / 8, lifetime);

    memcpy(o + 8, labels, len);
}

static const unsigned char rdnss_test[] = "\005rdnss\004test";   /* and its root label */

static int ra_send(void)
{
    struct sockaddr_in6 a;
    int s, hops = 255, n;

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
    n = sendto(s, (char *)ra, ralen, 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(10);
    return n == ralen ? 0 : -1;
}

static void forget(void)
{
    ra_begin();
    ra_rdnss(0, 0);
    ra_dnssl(rdnss_test, sizeof(rdnss_test), 0);
    ra_send();
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A server learned from an RDNSS option answers gethostbyname(). */
void testRDNSS_LEARNED(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 0);
    ra_begin();
    ra_rdnss(LIFETIME, 0);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    forget();
}

/* A lifetime of 0 ends it at once. */
void testRDNSS_ZERO(void)
{
    NEED_IF();
    ra_begin();
    ra_rdnss(LIFETIME, 0);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    ra_begin();
    ra_rdnss(0, 0);
    CU_ASSERT_EQUAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 0);
}

/* It ends by itself with its lifetime: 3 s. */
void testRDNSS_EXPIRES(void)
{
    NEED_IF();
    ra_begin();
    ra_rdnss(3, 0);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    Delay(5 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 0);
    forget();
}

/* A domain learned from a DNSSL option is searched; until its lifetime 0. */
void testRDNSS_DNSSL(void)
{
    NEED_IF();
    ra_begin();
    ra_rdnss(LIFETIME, 0);
    ra_dnssl(rdnss_test, sizeof(rdnss_test), LIFETIME);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("two"), 78);
    ra_begin();
    ra_dnssl(rdnss_test, sizeof(rdnss_test), 0);
    CU_ASSERT_EQUAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("two"), 0);
    CU_ASSERT_EQUAL(dns_lookup("two.rdnss.test"), 78);
    forget();
}

/* Options that do not parse are ignored: an RDNSS of even length, a
 * DNSSL with a compression pointer. */
void testRDNSS_MALFORMED(void)
{
    static const unsigned char pointer[] = { 3, 't', 'w', 'o', 0xc0, 12 };

    NEED_IF();
    ra_begin();
    ra_rdnss(LIFETIME, 1);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 0);
    ra_begin();
    ra_rdnss(LIFETIME, 0);
    ra_dnssl(pointer, sizeof(pointer), LIFETIME);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    CU_ASSERT_EQUAL(dns_lookup("two"), 0);
    forget();
}

/* A DHCP update of the dynamic servers leaves those from advertisements. */
void testRDNSS_DHCP(void)
{
    NEED_IF();
    if (MiamiBase == NULL) {
        CU_PASS("miami.library not available - skipping");
        return;
    }
    ra_begin();
    ra_rdnss(LIFETIME, 0);
    ra_dnssl(rdnss_test, sizeof(rdnss_test), LIFETIME);
    CU_ASSERT_EQUAL_FATAL(ra_send(), 0);
    ClearDynNameServ();
    EndDynNameServ();
    ClearDynDomain();
    EndDynDomain();
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    CU_ASSERT_EQUAL(dns_lookup("two"), 78);
    forget();
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int server = 0;

    if (argc > 1 && strcmp(argv[1], "--serve") == 0)
        return dns_serve();
    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping RDNSS tests\n");
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    ready = index_of(ifname, &ifindex) == 0 && get_linklocal(&linklocal);
    if (!ready)
        printf("%s: no interface with a link-local address - skipping RDNSS tests\n", ifname);
    else if (dns_other_server()) {
        printf("another DNS server answers first - skipping RDNSS tests\n");
        ready = 0;
    } else if (!(server = dns_start(argv[0]))) {
        printf("the test's DNS server did not start - skipping RDNSS tests\n");
        ready = 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("RDNSS_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a server from an RDNSS option answers", testRDNSS_LEARNED) ||
        !CU_add_test(p, "a lifetime of 0 ends it", testRDNSS_ZERO) ||
        !CU_add_test(p, "it ends with its lifetime", testRDNSS_EXPIRES) ||
        !CU_add_test(p, "a domain from a DNSSL option is searched", testRDNSS_DNSSL) ||
        !CU_add_test(p, "options that do not parse are ignored", testRDNSS_MALFORMED) ||
        !CU_add_test(p, "a DHCP update leaves them", testRDNSS_DHCP))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-RDNSS");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    if (server)
        dns_stop();
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
