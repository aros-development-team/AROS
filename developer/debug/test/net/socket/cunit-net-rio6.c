/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for Router Advertisement processing of RFC 4191: default
    router preferences and Route Information Options (a type C host). The
    test sends the Router Advertisements itself, from a raw ICMPv6 socket
    with hop limit 255 to the interface's own link-local address: they loop
    back and are received on that interface as if a router on the link had
    sent them, and none goes out on the link, where other hosts would take
    this one for a router. A second link-local address on the interface
    stands for a second router. The routing table is read back with RTM_GET
    on a routing socket.

    It needs an interface with no IPv6 router of its own (on hosted AROS,
    tap.device attached to a host tap that sends no Router Advertisements).
    Usage:  cunit-net-rio6 [IFNAME]  (default eth0). Without such an
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
    union { struct rt_msghdr h; char b[512]; } msg;
    struct sockaddr_in6 *sa;
    int s, n, want = ++seq;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.h.rtm_version = RTM_VERSION;
    msg.h.rtm_type = RTM_GET;
    msg.h.rtm_addrs = RTA_DST;
    msg.h.rtm_seq = want;
    sa = (struct sockaddr_in6 *)(&msg.h + 1);
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)dst, &sa->sin6_addr);
    msg.h.rtm_msglen = sizeof(msg.h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)&msg, msg.h.rtm_msglen, 0) < 0) {
        CloseSocket(s);
        return 0;       /* ESRCH: no route */
    }
    for (;;) {
        if (sock_wait(s, 0, 2) <= 0) {
            CloseSocket(s);
            return -1;
        }
        n = recv(s, (char *)&msg, sizeof(msg), 0);
        if (n <= 0)
            break;
        if (msg.h.rtm_type == RTM_GET && msg.h.rtm_seq == want)
            break;
    }
    CloseSocket(s);
    if (n <= 0 || msg.h.rtm_errno != 0 || !(msg.h.rtm_flags & RTF_GATEWAY) ||
            !(msg.h.rtm_addrs & RTA_GATEWAY))
        return 0;
    {
        char *p = (char *)(&msg.h + 1);
        struct sockaddr *a = (struct sockaddr *)p;      /* RTA_DST */
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

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A Route Information Option installs the route through its router, even
 * from a router that is not a default router (router lifetime 0). */
void testRIO6_INSTALLS_ROUTE(void)
{
    NEED_IF();
    CU_ASSERT(no_route("2001:db8:4d54:10::1"));
    CU_ASSERT_EQUAL(send_rio(&r1, "2001:db8:4d54:10::", 64, 2, PRF_MEDIUM, 600), 0);
    CU_ASSERT(via("2001:db8:4d54:10::1", &r1));
    send_rio(&r1, "2001:db8:4d54:10::", 64, 2, PRF_MEDIUM, 0);
}

/* A lifetime of 0 removes it. */
void testRIO6_LIFETIME_ZERO_REMOVES(void)
{
    NEED_IF();
    send_rio(&r1, "2001:db8:4d54:11::", 64, 2, PRF_MEDIUM, 600);
    CU_ASSERT(via("2001:db8:4d54:11::1", &r1));
    CU_ASSERT_EQUAL(send_rio(&r1, "2001:db8:4d54:11::", 64, 2, PRF_MEDIUM, 0), 0);
    CU_ASSERT(no_route("2001:db8:4d54:11::1"));
}

/* Of two routers advertising a prefix, the more preferred one is used; when
 * it withdraws, the other takes over. */
void testRIO6_PREFERENCE_WINS(void)
{
    NEED_IF();
    send_rio(&r1, "2001:db8:4d54:12::", 64, 2, PRF_LOW, 600);
    send_rio(&r2, "2001:db8:4d54:12::", 64, 2, PRF_HIGH, 600);
    CU_ASSERT(via("2001:db8:4d54:12::1", &r2));
    send_rio(&r2, "2001:db8:4d54:12::", 64, 2, PRF_HIGH, 0);
    CU_ASSERT(via("2001:db8:4d54:12::1", &r1));
    send_rio(&r1, "2001:db8:4d54:12::", 64, 2, PRF_LOW, 0);
    CU_ASSERT(no_route("2001:db8:4d54:12::1"));
}

/* A longer prefix from the same router is used for what it covers. */
void testRIO6_LONGEST_PREFIX(void)
{
    NEED_IF();
    send_rio(&r1, "2001:db8:4d54::", 48, 2, PRF_MEDIUM, 600);
    send_rio(&r2, "2001:db8:4d54:13::", 64, 2, PRF_LOW, 600);
    CU_ASSERT(via("2001:db8:4d54:13::1", &r2));
    CU_ASSERT(via("2001:db8:4d54:99::1", &r1));
    send_rio(&r2, "2001:db8:4d54:13::", 64, 2, PRF_LOW, 0);
    send_rio(&r1, "2001:db8:4d54::", 48, 2, PRF_MEDIUM, 0);
}

/* An option with the reserved preference, or whose length does not fit its
 * prefix length, is ignored. */
void testRIO6_INVALID_IGNORED(void)
{
    NEED_IF();
    send_rio(&r1, "2001:db8:4d54:14::", 64, 2, PRF_RSV, 600);
    CU_ASSERT(no_route("2001:db8:4d54:14::1"));
    send_rio(&r1, "2001:db8:4d54:15::", 80, 2, PRF_MEDIUM, 600);
    CU_ASSERT(no_route("2001:db8:4d54:15::1"));
}

/* Of two default routers, the more preferred one is the default route. */
void testRIO6_DEFAULT_ROUTER_PREFERENCE(void)
{
    NEED_IF();
    CU_ASSERT(no_route("2001:db8:ffff::1"));
    send_ra(&r1, 600, PRF_LOW, NULL, 0);
    send_ra(&r2, 600, PRF_HIGH, NULL, 0);
    CU_ASSERT(via("2001:db8:ffff::1", &r2));
    send_ra(&r2, 0, PRF_MEDIUM, NULL, 0);
    CU_ASSERT(via("2001:db8:ffff::1", &r1));
    send_ra(&r1, 0, PRF_MEDIUM, NULL, 0);
    CU_ASSERT(no_route("2001:db8:ffff::1"));
}

/* A ::/0 option overrides the router lifetime and preference of its RA. */
void testRIO6_DEFAULT_OPTION_OVERRIDES(void)
{
    u_char o[24];
    NEED_IF();
    rio(o, "::", 0, 1, PRF_MEDIUM, 600);
    send_ra(&r1, 0, PRF_MEDIUM, o, 8);
    CU_ASSERT(via("2001:db8:ffff::1", &r1));
    rio(o, "::", 0, 1, PRF_MEDIUM, 0);
    send_ra(&r1, 0, PRF_MEDIUM, o, 8);
    CU_ASSERT(no_route("2001:db8:ffff::1"));
}

/* -------------------------------------------------------------------- setup */

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    struct in6_aliasreq ifra;
    struct in6_addr gw;
    int s, i, n, found = 0;

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
        n = ifc.ifc6_len / sizeof(req[0]);
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
        printf("bsdsocket.library not available - skipping RIO6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no interface without a router - skipping RIO6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("RIO6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a Route Information Option installs its route", testRIO6_INSTALLS_ROUTE) ||
        !CU_add_test(p, "lifetime 0 removes the route", testRIO6_LIFETIME_ZERO_REMOVES) ||
        !CU_add_test(p, "the more preferred router wins", testRIO6_PREFERENCE_WINS) ||
        !CU_add_test(p, "the longest prefix is used", testRIO6_LONGEST_PREFIX) ||
        !CU_add_test(p, "an invalid option is ignored", testRIO6_INVALID_IGNORED) ||
        !CU_add_test(p, "the more preferred default router wins", testRIO6_DEFAULT_ROUTER_PREFERENCE) ||
        !CU_add_test(p, "a ::/0 option overrides the router lifetime", testRIO6_DEFAULT_OPTION_OVERRIDES))
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
    CU_set_output_filename("Net-RIO6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    teardown();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
