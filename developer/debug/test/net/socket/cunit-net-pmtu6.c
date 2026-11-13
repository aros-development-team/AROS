/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the path MTU an ICMPv6 Packet Too Big reports (RFC
    8201): for a destination reached through a router, it goes on a host
    route of the destination's own - not on the router's route, which other
    destinations share - and is kept for a while, then given back (section
    4); it is never below the IPv6 minimum, 1280, and only ever lowered.
    For a destination on the link, whose neighbour is no route, a host route
    on the link's interface is added for it the same way; for a link-local
    one, on the interface the report came in by, and used only there.

    A route to 2001:db8:4d54:42::/64 through a router on the link is added
    with a routing socket; the Packet Too Big messages, quoting packets to
    destinations behind it, are sent to ::1 from a raw ICMPv6 socket; the
    routes are read back with RTM_GET. The test needs an interface with an
    IPv6 link-local address (on hosted AROS, tap.device attached to a host
    tap). Usage:  cunit-net-pmtu6 [IFNAME]  (default eth0). Without one the
    tests are skipped. The on-link prefix 2001:db8:4d54:44::/64 is added on
    the interface; the link-local report is sent to the interface's own
    link-local address, so that it comes in by that interface.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <time.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in_var.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static int ready;
static unsigned ifindex, ifindex2;     /* ifname's, and eth1's if there is one */
static struct in6_addr linklocal, linklocal2;   /* their link-local addresses */
static int route_index;                 /* rtm_index of the last route_get() */

#define GW      "fe80::4d54:a"          /* a router on the link; nobody answers */
#define NET     "2001:db8:4d54:42::"    /* reached through it */
#define DST_A   "2001:db8:4d54:42::1"   /* the Packet Too Big is about this one */
#define DST_B   "2001:db8:4d54:42::2"   /* and not about this one */
#define DST_C   "2001:db8:4d54:42::3"
#define ONNET   "2001:db8:4d54:44::"    /* on the link */
#define ON_A    "2001:db8:4d54:44::1"
#define ON_B    "2001:db8:4d54:44::2"
#define LL_A    "fe80::4d54:c"          /* a link-local neighbour; nobody has it */

#define EXPIRE  600                     /* seconds the stack keeps a path MTU */

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

static void sa6(struct sockaddr_in6 *a, const char *addr)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
}

/* Add or delete dst/plen through GW with a routing socket (plen 128: a
 * host route). */
static int route(int type, const char *dst, int plen)
{
    long buf[512 / sizeof(long)];       /* aligned for the sockaddrs */
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    char *p;
    int s, r;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = type;
    h->rtm_flags = RTF_UP | RTF_GATEWAY | (plen == 128 ? RTF_HOST : RTF_STATIC);
    h->rtm_addrs = RTA_DST | RTA_GATEWAY | (plen == 128 ? 0 : RTA_NETMASK);
    h->rtm_seq = 1;
    p = (char *)(h + 1);
    sa = (struct sockaddr_in6 *)p; sa6(sa, dst); p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p; sa6(sa, GW);  p += ROUNDUP(sizeof(*sa));
    if (plen < 128) {
        int i;

        sa = (struct sockaddr_in6 *)p; sa6(sa, NULL);
        for (i = 0; i < plen / 8; i++)
            sa->sin6_addr.s6_addr[i] = 0xff;
        p += ROUNDUP(sizeof(*sa));
    }
    h->rtm_msglen = p - (char *)buf;
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* Add or delete ONNET/64 on the interface: an interface route, our
 * link-local address for gateway. */
static int onlink(int type)
{
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    char *p;
    int s, r, i;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = type;
    h->rtm_flags = RTF_UP | RTF_STATIC;
    h->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    h->rtm_seq = 3;
    p = (char *)(h + 1);
    sa = (struct sockaddr_in6 *)p; sa6(sa, ONNET); p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p; sa6(sa, NULL); sa->sin6_addr = linklocal; p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p; sa6(sa, NULL);
    for (i = 0; i < 8; i++)
        sa->sin6_addr.s6_addr[i] = 0xff;
    p += ROUNDUP(sizeof(*sa));
    h->rtm_msglen = p - (char *)buf;
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* The route the table uses for dst, via RTM_GET: its flags and metrics,
 * and in route_index the index of its interface (asked as RTA_IFP: the
 * interface's link-level address). 0 if there is none. */
static int route_get(const char *dst, int *flags, struct rt_metrics *rmx)
{
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    int s, n = 0;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return 0;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_GET;
    h->rtm_addrs = RTA_DST | RTA_IFP;
    h->rtm_seq = 2;
    sa = (struct sockaddr_in6 *)(h + 1);
    sa6(sa, dst);
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)buf, h->rtm_msglen, 0) >= 0)
        while (sock_wait(s, 0, 2) > 0) {
            n = recv(s, (char *)buf, sizeof(buf), 0);
            if (n <= 0 || (h->rtm_type == RTM_GET && h->rtm_seq == 2))
                break;
        }
    CloseSocket(s);
    if (n <= 0 || h->rtm_errno != 0)
        return 0;
    *flags = h->rtm_flags;
    *rmx = h->rtm_rmx;
    route_index = 0;
    {
        char *p = (char *)(h + 1);
        const char *end = (char *)buf + n;
        int bit;

        for (bit = 1; bit <= RTA_IFA && p < end; bit <<= 1) {
            struct sockaddr *x = (struct sockaddr *)p;

            if (!(h->rtm_addrs & bit))
                continue;
            if (bit == RTA_IFP && x->sa_family == AF_LINK)
                route_index = ((struct sockaddr_dl *)x)->sdl_index;
            p += x->sa_len ? ROUNDUP(x->sa_len) : sizeof(long);
        }
    }
    return 1;
}

/* A Packet Too Big reporting mtu, quoting a UDP datagram from ::1 to dst,
 * sent to ::1 - or with to_ll 1, to the interface's link-local address, so
 * that it comes in by the interface; with 2, by the second interface. */
static int send_ptb_to(const char *dst, unsigned mtu, int to_ll)
{
    unsigned char pkt[8 + 40 + 8];
    struct icmp6_hdr *h = (struct icmp6_hdr *)pkt;
    struct ip6_hdr *ip6 = (struct ip6_hdr *)(pkt + 8);
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), r;

    if (s < 0)
        return -1;
    bzero(pkt, sizeof(pkt));
    h->icmp6_type = ICMP6_PACKET_TOO_BIG;
    h->icmp6_mtu = htonl(mtu);
    ip6->ip6_vfc = IPV6_VERSION;
    ip6->ip6_plen = htons(8);
    ip6->ip6_nxt = IPPROTO_UDP;
    ip6->ip6_hlim = 64;
    ip6->ip6_src = lo;
    inet_pton(AF_INET6, (char *)dst, &ip6->ip6_dst);
    sa6(&d, NULL);
    d.sin6_addr = lo;
    if (to_ll == 1) {
        d.sin6_addr = linklocal;
        d.sin6_scope_id = ifindex;
    } else if (to_ll == 2) {
        d.sin6_addr = linklocal2;
        d.sin6_scope_id = ifindex2;
    }
    r = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(s);
    Delay(10);                          /* let the stack take it in */
    return r;
}

static int send_ptb(const char *dst, unsigned mtu)
{
    return send_ptb_to(dst, mtu, 0);
}

/* IPV6_PATHMTU of a UDP socket connected to dst on the interface index
 * scope: the path MTU, or 0. */
static unsigned pathmtu(const char *dst, unsigned scope)
{
    struct sockaddr_in6 d;
    struct ip6_mtuinfo mi;
    socklen_t l = sizeof(mi);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    unsigned mtu = 0;

    if (s < 0)
        return 0;
    sa6(&d, dst);
    d.sin6_port = htons(9);
    d.sin6_scope_id = scope;
    bzero(&mi, sizeof(mi));
    if (connect(s, (struct sockaddr *)&d, sizeof(d)) == 0 &&
            getsockopt(s, IPPROTO_IPV6, IPV6_PATHMTU, &mi, &l) == 0)
        mtu = mi.ip6m_mtu;
    CloseSocket(s);
    return mtu;
}

/* Seconds since 1970 in UTC: route lifetimes are given as times of day. */
static unsigned long now(void)
{
    return (unsigned long)time(NULL);
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void)
{
    if (SocketBase == NULL)
        return -1;
    if (ready && (route(RTM_ADD, NET, 64) != 0 || onlink(RTM_ADD) != 0))
        ready = 0;
    return 0;
}

int clean_suite(void)
{
    if (ready) {
        route(RTM_DELETE, DST_A, 128);
        route(RTM_DELETE, DST_C, 128);
        route(RTM_DELETE, NET, 64);
        route(RTM_DELETE, ON_A, 128);
        route(RTM_DELETE, LL_A, 128);
        onlink(RTM_DELETE);
    }
    return 0;
}

/* The path MTU goes on a host route of the destination's own; another
 * destination through the same router keeps the router route's MTU. */
void testPMTU6_OWN_ROUTE(void)
{
    struct rt_metrics a, b;
    int fa = 0, fb = 0;
    NEED_IF();
    CU_ASSERT(send_ptb(DST_A, 1400) > 0);
    CU_ASSERT_FATAL(route_get(DST_A, &fa, &a));
    CU_ASSERT(fa & RTF_HOST);
    CU_ASSERT_EQUAL(a.rmx_mtu, 1400);
    CU_ASSERT_FATAL(route_get(DST_B, &fb, &b));
    CU_ASSERT(!(fb & RTF_HOST));
    CU_ASSERT(b.rmx_mtu != 1400);
}

/* It is kept for a while, then given back (RFC 8201 section 4). */
void testPMTU6_EXPIRES(void)
{
    struct rt_metrics a;
    unsigned long t = now();
    int fa = 0;
    NEED_IF();
    CU_ASSERT(send_ptb(DST_A, 1400) > 0);
    CU_ASSERT_FATAL(route_get(DST_A, &fa, &a));
    CU_ASSERT(a.rmx_expire >= t + EXPIRE - 10 && a.rmx_expire <= t + EXPIRE + 10);
}

/* A larger MTU than the one known does not raise it. */
void testPMTU6_ONLY_LOWER(void)
{
    struct rt_metrics a;
    int fa = 0;
    NEED_IF();
    CU_ASSERT(send_ptb(DST_A, 1400) > 0);
    CU_ASSERT(send_ptb(DST_A, 1450) > 0);
    CU_ASSERT_FATAL(route_get(DST_A, &fa, &a));
    CU_ASSERT_EQUAL(a.rmx_mtu, 1400);
}

/* Never below the IPv6 minimum. */
void testPMTU6_MINIMUM(void)
{
    struct rt_metrics c;
    int fc = 0;
    NEED_IF();
    CU_ASSERT(send_ptb(DST_C, 1000) > 0);
    CU_ASSERT_FATAL(route_get(DST_C, &fc, &c));
    CU_ASSERT_EQUAL(c.rmx_mtu, IPV6_MMTU);
}

/* On the link: the path MTU goes on a host route of the destination's
 * own on the link's interface (no router); another destination on the
 * link keeps the prefix. */
void testPMTU6_ON_LINK(void)
{
    struct rt_metrics a, b;
    int fa = 0, fb = 0;
    NEED_IF();
    CU_ASSERT(send_ptb(ON_A, 1400) > 0);
    CU_ASSERT_FATAL(route_get(ON_A, &fa, &a));
    CU_ASSERT((fa & (RTF_HOST | RTF_DYNAMIC | RTF_GATEWAY)) == (RTF_HOST | RTF_DYNAMIC));
    CU_ASSERT_EQUAL(a.rmx_mtu, 1400);
    CU_ASSERT_EQUAL(pathmtu(ON_A, 0), 1400);
    CU_ASSERT_FATAL(route_get(ON_B, &fb, &b));
    CU_ASSERT(!(fb & RTF_HOST));
    CU_ASSERT(pathmtu(ON_B, 0) > 1400);
}

/* A link-local destination: the path MTU holds in the zone the report came
 * from, not in another link's. */
void testPMTU6_LINK_LOCAL(void)
{
    NEED_IF();
    CU_ASSERT(send_ptb_to(LL_A, 1400, 1) > 0);
    CU_ASSERT_EQUAL(pathmtu(LL_A, ifindex), 1400);
    if (ifindex2 != 0)
        CU_ASSERT(pathmtu(LL_A, ifindex2) > 1400);
}

/*
 * The same link-local destination on two links, whose neighbour cache keeps
 * them apart while the routing table keys a host route by address alone: a
 * report from link B while link A's route of the destination is there is
 * not recorded, and A's route is left as it was; once A's route is gone -
 * deleted here as its expiry deletes it (pmtu_restore()) - B's report makes
 * a route of B's own, with B's MTU, used on B only. The destination is
 * this run's (fe80::4d54:d:N), so that no path MTU an earlier test or run
 * recorded is there: it starts with no route of its own.
 */
void testPMTU6_TWO_LINKS(void)
{
    static unsigned n;
    struct rt_metrics m;
    char ll[40];
    int f = 0, tick;

    NEED_IF();
    if (ifindex2 == 0 || IN6_IS_ADDR_UNSPECIFIED(&linklocal2)) {
        CU_PASS("no second interface with a link-local address - skipping");
        return;
    }
    snprintf(ll, sizeof(ll), "fe80::4d54:d:%x", (unsigned)((now() + n++) & 0xffff));
    CU_ASSERT(!route_get(ll, &f, &m) || !(f & RTF_HOST));
    CU_ASSERT(send_ptb_to(ll, 1400, 1) > 0);
    CU_ASSERT_FATAL(route_get(ll, &f, &m));
    printf("\n    %s A: flags 0x%x mtu %lu index %d", ll, f, (unsigned long)m.rmx_mtu, route_index);
    CU_ASSERT((f & RTF_HOST) && m.rmx_mtu == 1400 && route_index == (int)ifindex);

    /* B reports while A's route is there: not recorded, A unchanged */
    CU_ASSERT(send_ptb_to(ll, 1300, 2) > 0);
    CU_ASSERT_FATAL(route_get(ll, &f, &m));
    CU_ASSERT((f & RTF_HOST) && m.rmx_mtu == 1400 && route_index == (int)ifindex);
    CU_ASSERT_EQUAL(pathmtu(ll, ifindex), 1400);
    CU_ASSERT(pathmtu(ll, ifindex2) > 1300);

    /* A's route goes, as at its expiry; its slot lets it go at the next
     * slow tick (rt_pmtu_timer()): the deletion is seen to take effect */
    CU_ASSERT_EQUAL(route(RTM_DELETE, ll, 128), 0);
    for (tick = 0; tick < 20; tick++) {
        Delay(25);
        f = 0;
        if (!route_get(ll, &f, &m) || !(f & RTF_HOST))
            break;
    }
    CU_ASSERT(!(f & RTF_HOST));

    /* now B's report is B's route */
    CU_ASSERT(send_ptb_to(ll, 1300, 2) > 0);
    CU_ASSERT_FATAL(route_get(ll, &f, &m));
    printf("\n    %s B: flags 0x%x mtu %lu index %d\n", ll, f, (unsigned long)m.rmx_mtu, route_index);
    CU_ASSERT((f & RTF_HOST) && m.rmx_mtu == 1300 && route_index == (int)ifindex2);
    CU_ASSERT_EQUAL(pathmtu(ll, ifindex2), 1300);
    CU_ASSERT(pathmtu(ll, ifindex) > 1300);
    route(RTM_DELETE, ll, 128);
}

static int setup(void)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), found = 0;

    if (s < 0)
        return 0;
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        int i, n = ifc.ifc6_len / sizeof(req[0]);

        for (i = 0; i < n; i++) {
            const char *other = strcmp(ifname, "eth1") ? "eth1" : "eth0";

            if (!IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr))
                continue;
            if (strcmp(req[i].ifr_name, ifname) == 0 && !found) {
                linklocal = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            } else if (strcmp(req[i].ifr_name, other) == 0)
                linklocal2 = req[i].ifr_ifru.ifru_addr.sin6_addr;
        }
    }
    if (found) {
        struct ifreq ifr;

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        found = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0;
        ifindex = ifr.ifr_index;
        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", strcmp(ifname, "eth1") ? "eth1" : "eth0");
        if (IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
            ifindex2 = ifr.ifr_index;
    }
    CloseSocket(s);
    return found;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping PMTU6 tests\n");
        return 0;
    }
    if (st_skip_program("cunit-net-pmtu6", ST_V6)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no interface with a link-local address - skipping PMTU6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PMTU6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the path MTU goes on a route of the destination's own", testPMTU6_OWN_ROUTE) ||
        !CU_add_test(p, "the path MTU is kept for a while, then given back", testPMTU6_EXPIRES) ||
        !CU_add_test(p, "a larger MTU does not raise the path MTU", testPMTU6_ONLY_LOWER) ||
        !CU_add_test(p, "the path MTU is never below 1280", testPMTU6_MINIMUM) ||
        !CU_add_test(p, "on the link, a route of the destination's own", testPMTU6_ON_LINK) ||
        !CU_add_test(p, "a link-local destination's, in its zone only", testPMTU6_LINK_LOCAL) ||
        !CU_add_test(p, "the same link-local destination on two links", testPMTU6_TWO_LINKS))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-PMTU6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
