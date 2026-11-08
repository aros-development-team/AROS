/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for a router that says it is no longer one (RFC 4861
    section 7.2.5): a Neighbor Advertisement from a default router with the
    Router flag clear takes it off the default router list, with the routes
    through it - a Route Information Option's among them - and the default
    route goes through another router. One with the flag set changes
    nothing.

    As cunit-net-rio6, the test sends the Router Advertisements and the
    Neighbor Advertisements itself, from a raw ICMPv6 socket with hop limit
    255 to the interface's own link-local address (router 1); a second
    link-local address on the interface stands for router 2, the preferred
    one. The routing table is read back with RTM_GET.

    It needs an interface with no IPv6 router of its own (on hosted AROS,
    tap.device attached to a host tap that sends no Router Advertisements).
    Usage:  cunit-net-narouter6 [IFNAME]  (default eth0). Without such an
    interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() in bsdsocket.library */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr r1;      /* the interface's link-local: router 1 */
static struct in6_addr r2;      /* fe80::4d54:2, added: router 2 */
static int ready;

#define PRF_HIGH    0x08
#define PRF_MEDIUM  0x00
#define PRF_LOW     0x18
#define PRF_RSV     0x10

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/* ------------------------------------------------------- advertisements */

/* A Route Information Option: prefix/plen, with the given option length
 * in units of 8 bytes, preference and lifetime. Returns its size. */
static int rio(u_char *p, const char *prefix, int plen, int len, int prf,
    u_int32_t lifetime)
{
    struct in6_addr a;
    u_int32_t lt = htonl(lifetime);

    bzero(p, len * 8);
    p[0] = ND_OPT_ROUTE_INFO;
    p[1] = len;
    p[2] = plen;
    p[3] = prf;
    memcpy(p + 4, &lt, 4);
    inet_pton(AF_INET6, (char *)prefix, &a);
    memcpy(p + 8, &a, (len - 1) * 8);
    return len * 8;
}

/* Send a Router Advertisement from router src with this router lifetime
 * and preference, followed by the options in opts. */
static int send_ra(const struct in6_addr *src, u_int16_t lifetime, int prf,
    const u_char *opts, int optlen)
{
    u_char pkt[256];
    struct nd_router_advert *ra = (struct nd_router_advert *)pkt;
    struct sockaddr_in6 a;
    int s, hops = 255, n;

    bzero(pkt, sizeof(pkt));
    ra->nd_ra_type = ND_ROUTER_ADVERT;
    ra->nd_ra_flags_reserved = prf;
    ra->nd_ra_router_lifetime = htons(lifetime);
    memcpy(pkt + sizeof(*ra), opts, optlen);

    s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (s < 0)
        return -1;
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = *src;
    a.sin6_scope_id = ifindex;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) != 0) {
        CloseSocket(s);
        return -1;
    }
    /* to our own link-local address: it loops back, never onto the link */
    a.sin6_addr = r1;
    a.sin6_scope_id = ifindex;
    n = sendto(s, (char *)pkt, sizeof(*ra) + optlen, 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(25);      /* let the stack process it */
    return n == (int)(sizeof(*ra) + optlen) ? 0 : -1;
}

/* One Route Information Option in an RA with router lifetime 0. */
static int send_rio(const struct in6_addr *src, const char *prefix, int plen,
    int len, int prf, u_int32_t lifetime)
{
    u_char o[24];
    int n = rio(o, prefix, plen, len, prf, lifetime);
    return send_ra(src, 0, PRF_MEDIUM, o, n);
}

/* ---------------------------------------------------------------- routes */

/* The gateway of the route the table uses for dst, via RTM_GET. Returns 1
 * with *gw set for a gateway route, 0 when there is no route. */
static int route_get(const char *dst, struct in6_addr *gw)
{
    static int seq;
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    int s, n, want = ++seq;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_GET;
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = want;
    sa = (struct sockaddr_in6 *)(h + 1);
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)dst, &sa->sin6_addr);
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)buf, h->rtm_msglen, 0) < 0) {
        CloseSocket(s);
        return 0;       /* ESRCH: no route */
    }
    for (;;) {
        if (sock_wait(s, 0, 2) <= 0) {
            CloseSocket(s);
            return -1;
        }
        n = recv(s, (char *)buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        if (h->rtm_type == RTM_GET && h->rtm_seq == want)
            break;
    }
    CloseSocket(s);
    if (n <= 0 || h->rtm_errno != 0 || !(h->rtm_flags & RTF_GATEWAY) ||
            !(h->rtm_addrs & RTA_GATEWAY))
        return 0;
    {
        char *p = (char *)(h + 1);
        const struct sockaddr *a = (const struct sockaddr *)p;      /* RTA_DST */
        p += ROUNDUP(a->sa_len);
        sa = (struct sockaddr_in6 *)p;                   /* RTA_GATEWAY */
        if (sa->sin6_family != AF_INET6)
            return 0;
        *gw = sa->sin6_addr;
    }
    return 1;
}

static int via(const char *dst, const struct in6_addr *router)
{
    struct in6_addr gw;
    return route_get(dst, &gw) == 1 && memcmp(&gw, router, sizeof(gw)) == 0;
}

static int no_route(const char *dst)
{
    struct in6_addr gw;
    return route_get(dst, &gw) == 0;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

#define DEST    "2001:db8:ffff::1"
#define RIONET  "2001:db8:4d54:d0::"
#define RIODST  "2001:db8:4d54:d0::1"

/* A Neighbor Advertisement from router 2 for itself, with the Router flag
 * set or not. */
static int send_na(int router)
{
    struct nd_neighbor_advert na;
    struct sockaddr_in6 a;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), hops = 255, n;

    if (s < 0)
        return -1;
    bzero(&na, sizeof(na));
    na.nd_na_type = ND_NEIGHBOR_ADVERT;
    na.nd_na_flags_reserved = ND_NA_FLAG_OVERRIDE | (router ? ND_NA_FLAG_ROUTER : 0);
    na.nd_na_target = r2;
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = r2;
    a.sin6_scope_id = ifindex;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) != 0) {
        CloseSocket(s);
        return -1;
    }
    a.sin6_addr = r1;
    n = sendto(s, (char *)&na, sizeof(na), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(25);
    return n == (int)sizeof(na) ? 0 : -1;
}

/* Both routers advertising: router 2 preferred, with a more-specific route. */
static void routers_up(void)
{
    u_char o[24];
    int n = rio(o, RIONET, 64, 2, PRF_MEDIUM, 600);

    send_ra(&r1, 600, PRF_LOW, o, 0);
    send_ra(&r2, 600, PRF_HIGH, o, n);
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }

int clean_suite(void)
{
    u_char o[24];
    int n;

    if (!ready)
        return 0;
    n = rio(o, RIONET, 64, 2, PRF_MEDIUM, 0);
    send_ra(&r2, 0, PRF_HIGH, o, n);
    send_ra(&r1, 0, PRF_LOW, o, 0);
    return 0;
}

/* The Router flag set: nothing changes. */
void testNAROUTER6_STILL_ROUTER(void)
{
    NEED_IF();
    routers_up();
    CU_ASSERT_FATAL(via(DEST, &r2));
    CU_ASSERT_FATAL(via(RIODST, &r2));
    CU_ASSERT_EQUAL(send_na(1), 0);
    CU_ASSERT(via(DEST, &r2));
    CU_ASSERT(via(RIODST, &r2));
}

/* The Router flag clear: router 2 and its routes go; router 1 takes over. */
void testNAROUTER6_NO_LONGER(void)
{
    NEED_IF();
    routers_up();
    CU_ASSERT_FATAL(via(DEST, &r2));
    CU_ASSERT_EQUAL(send_na(0), 0);
    CU_ASSERT(via(DEST, &r1));
    CU_ASSERT(via(RIODST, &r1));        /* the default, not router 2's route */
}

/* -------------------------------------------------------------------- setup */

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    struct in6_aliasreq ifra;
    struct in6_addr gw;
    int s, i, found = 0;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) != 0 ||
            !(ifr.ifr_flags & IFF_UP) ||
            IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) != 0) {
        CloseSocket(s);
        return 0;
    }
    ifindex = ifr.ifr_index;
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        int n = ifc.ifc6_len / sizeof(req[0]);

        for (i = 0; i < n; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                r1 = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
                break;
            }
    }

    /* router 2: a second link-local address on the interface */
    inet_pton(AF_INET6, "fe80::4d54:2", &r2);
    bzero(&ifra, sizeof(ifra));
    strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
    ifra.ifra_addr.sin6_len = sizeof(ifra.ifra_addr);
    ifra.ifra_addr.sin6_family = AF_INET6;
    ifra.ifra_addr.sin6_addr = r2;
    ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
    ifra.ifra_prefixmask.sin6_family = AF_INET6;
    for (i = 0; i < 8; i++)
        ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
    ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
    if (found && IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra) != 0)
        found = 0;
    CloseSocket(s);
    if (!found)
        return 0;
    Delay(3 * 50);      /* duplicate address detection on fe80::4d54:2 */

    /* an interface whose link already has a default router is not used */
    return route_get("2001:db8:ffff::1", &gw) == 0;
}

static void teardown(void)
{
    struct in6_ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
    ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
    ifr.ifr_ifru.ifru_addr.sin6_addr = r2;
    IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    CloseSocket(s);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping NAROUTER6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no interface without a router - skipping NAROUTER6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NAROUTER6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the Router flag set: it stays a router", testNAROUTER6_STILL_ROUTER) ||
        !CU_add_test(p, "the Router flag clear: it and its routes go", testNAROUTER6_NO_LONGER))
    {
        CU_cleanup_registry();
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-NAROUTER6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    teardown();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
