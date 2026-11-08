/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests that setting the system time moves no lifetime or timeout
    of the stack: they are kept on the monotonic clock (timer.device's
    GetUpTime()), as FreeBSD keeps them on time_uptime. Each case learns
    something with a lifetime, sets the clock a day forward or back, and
    checks that it still ends when its lifetime says, then sets the clock
    back to where it was:
    - an address made by stateless autoconfiguration (RFC 4862), and its
      prefix;
    - a route from a Route Information Option (RFC 4191);
    - a DNS server from an RDNSS option (RFC 8106);
    - a path MTU an ICMPv6 Packet Too Big lowered (RFC 8201);
    - an answer in the resolver's cache, by its TTL (RFC 1035).
    Applications see and give a route's expiry as a time of day, as in
    BSD; one given reads back as it was.

    The Router Advertisements go to the interface's own link-local address,
    router lifetime 0, as cunit-net-slaac6, cunit-net-rio6 and
    cunit-net-rdnss send them; the DNS server is the test's own
    (dnstest.h), on that link-local address.
    Usage:  cunit-net-clockstep [IFNAME]  (default eth0). Without an
    interface with a link-local address, or with a default router already
    on the link, the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"
#include "dnstest.h"

#include <proto/dos.h>
#include <proto/timer.h>
#include <devices/timer.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <time.h>

struct Library *SocketBase = NULL;
struct Device *TimerBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready, dns_ready;

#define DAY     86400

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/* ----------------------------------------------------------------- clock */

static struct MsgPort *timer_port;
static struct timerequest *timer_req;
static long stepped;            /* seconds the system time is off by */

static int timer_open(void)
{
    timer_port = CreateMsgPort();
    if (timer_port == NULL)
        return 0;
    timer_req = (struct timerequest *)CreateIORequest(timer_port, sizeof(*timer_req));
    if (timer_req == NULL ||
            OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer_req, 0) != 0) {
        if (timer_req)
            DeleteIORequest((struct IORequest *)timer_req);
        DeleteMsgPort(timer_port);
        timer_req = NULL;
        return 0;
    }
    TimerBase = timer_req->tr_node.io_Device;
    return 1;
}

static void timer_close(void)
{
    if (timer_req == NULL)
        return;
    CloseDevice((struct IORequest *)timer_req);
    DeleteIORequest((struct IORequest *)timer_req);
    DeleteMsgPort(timer_port);
}

/* Set the system time secs seconds forward (back if negative). */
static int clock_step(long secs)
{
    timer_req->tr_node.io_Command = TR_GETSYSTIME;
    if (DoIO((struct IORequest *)timer_req) != 0)
        return -1;
    timer_req->tr_time.tv_secs += secs;
    timer_req->tr_node.io_Command = TR_SETSYSTIME;
    if (DoIO((struct IORequest *)timer_req) != 0)
        return -1;
    stepped += secs;
    return 0;
}

/* The system time where it was before the case. */
static void clock_restore(void)
{
    if (stepped != 0)
        clock_step(-stepped);
}

/* --------------------------------------------------------- advertisements */

/* A Router Advertisement, router lifetime 0, with the options given, from
 * and to the interface's link-local address: it loops back, never onto the
 * link. */
static int send_ra(const u_char *opts, int optlen)
{
    u_char pkt[256];
    struct nd_router_advert *ra = (struct nd_router_advert *)pkt;
    struct sockaddr_in6 a;
    int s, hops = 255, n;

    bzero(pkt, sizeof(pkt));
    ra->nd_ra_type = ND_ROUTER_ADVERT;
    memcpy(pkt + sizeof(*ra), opts, optlen);

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
    n = sendto(s, (char *)pkt, sizeof(*ra) + optlen, 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(10);      /* let the stack process it */
    return n == (int)(sizeof(*ra) + optlen) ? 0 : -1;
}

/* A Prefix Information Option for prefix/64, on-link and autonomous. */
static int send_prefix(const char *prefix, u_int32_t valid, u_int32_t preferred)
{
    struct nd_opt_prefix_info pi;

    bzero(&pi, sizeof(pi));
    pi.nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
    pi.nd_opt_pi_len = 4;
    pi.nd_opt_pi_prefix_len = 64;
    pi.nd_opt_pi_flags_reserved = ND_OPT_PI_FLAG_ONLINK | ND_OPT_PI_FLAG_AUTO;
    pi.nd_opt_pi_valid_time = htonl(valid);
    pi.nd_opt_pi_preferred_time = htonl(preferred);
    inet_pton(AF_INET6, (char *)prefix, &pi.nd_opt_pi_prefix);
    return send_ra((u_char *)&pi, sizeof(pi));
}

/* A Route Information Option for prefix/64, medium preference. */
static int send_rio(const char *prefix, u_int32_t lifetime)
{
    u_char o[24];
    struct in6_addr a;
    u_int32_t lt = htonl(lifetime);

    bzero(o, sizeof(o));
    o[0] = ND_OPT_ROUTE_INFO;
    o[1] = 3;
    o[2] = 64;
    memcpy(o + 4, &lt, 4);
    inet_pton(AF_INET6, (char *)prefix, &a);
    memcpy(o + 8, &a, sizeof(a));
    return send_ra(o, sizeof(o));
}

/* An RDNSS option naming our link-local address. */
static int send_rdnss(u_int32_t lifetime)
{
    u_char o[24];
    u_int32_t lt = htonl(lifetime);

    bzero(o, sizeof(o));
    o[0] = ND_OPT_RDNSS;
    o[1] = 3;
    memcpy(o + 4, &lt, 4);
    memcpy(o + 8, &linklocal, sizeof(linklocal));
    return send_ra(o, sizeof(o));
}

/* ------------------------------------------------------------- addresses */

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

static void del_addr(const struct in6_addr *addr)
{
    struct in6_ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
    ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
    ifr.ifr_ifru.ifru_addr.sin6_addr = *addr;
    IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    CloseSocket(s);
}

/* ---------------------------------------------------------------- routes */

/* The route the table uses for dst, via RTM_GET, into *rtm. Returns 1 with
 * *gw set for a gateway route, 0 when there is none. */
static int route_get(const char *dst, struct in6_addr *gw, struct rt_msghdr *rtm)
{
    static int seq;
    long buf[512 / sizeof(long)];       /* aligned for the sockaddrs */
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
        const char *p = (const char *)(h + 1);
        const struct sockaddr *a = (const struct sockaddr *)p;     /* RTA_DST */
        const struct sockaddr_in6 *g;

        p += ROUNDUP(a->sa_len);
        g = (const struct sockaddr_in6 *)p;                         /* RTA_GATEWAY */
        if (g->sin6_family != AF_INET6)
            return 0;
        *gw = g->sin6_addr;
    }
    if (rtm)
        *rtm = *h;
    return 1;
}

static int via_us(const char *dst)
{
    struct in6_addr gw;
    return route_get(dst, &gw, NULL) == 1 && memcmp(&gw, &linklocal, sizeof(gw)) == 0;
}

static int no_route(const char *dst)
{
    struct in6_addr gw;
    return route_get(dst, &gw, NULL) == 0;
}

/* A Packet Too Big reporting mtu, quoting a UDP datagram from ::1 to dst,
 * sent to ::1, as cunit-net-pmtu6 sends it. */
static int send_ptb(const char *dst, unsigned mtu)
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
    bzero(&d, sizeof(d));
    d.sin6_len = sizeof(d);
    d.sin6_family = AF_INET6;
    d.sin6_addr = lo;
    r = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(s);
    Delay(10);
    return r;
}

/* Add (RTM_ADD) or delete a host route to dst through our link-local
 * address; with expire, that expiry (RTV_EXPIRE). */
static int host_route(int type, const char *dst, u_long expire)
{
    long buf[512 / sizeof(long)];
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
    h->rtm_flags = RTF_UP | RTF_GATEWAY | RTF_HOST | RTF_STATIC;
    h->rtm_addrs = RTA_DST | RTA_GATEWAY;
    h->rtm_seq = 1;
    if (expire) {
        h->rtm_inits = RTV_EXPIRE;
        h->rtm_rmx.rmx_expire = expire;
    }
    p = (char *)(h + 1);
    sa = (struct sockaddr_in6 *)p;
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)dst, &sa->sin6_addr);
    p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p;
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    sa->sin6_addr = linklocal;
    sa->sin6_scope_id = ifindex;
    p += ROUNDUP(sizeof(*sa));
    h->rtm_msglen = p - (char *)buf;
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* Wait up to secs for dst to have no route. */
static int wait_no_route(const char *dst, int secs)
{
    int i;

    for (i = 0; i < secs * 5; i++) {
        if (no_route(dst))
            return 1;
        Delay(10);
    }
    return 0;
}

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { clock_restore(); return 0; }

/* The test's own check: a step moves the system time by the step and the
 * monotonic clock not at all. */
void testCLOCKSTEP_STEP(void)
{
    struct timeval up0, up1, sys0, sys1;

    GetUpTime(&up0);
    GetSysTime(&sys0);
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    GetUpTime(&up1);
    GetSysTime(&sys1);
    CU_ASSERT(sys1.tv_secs - sys0.tv_secs >= DAY - 1 && sys1.tv_secs - sys0.tv_secs <= DAY + 1);
    CU_ASSERT(up1.tv_secs - up0.tv_secs <= 1);
    clock_restore();
    GetSysTime(&sys1);
    CU_ASSERT(sys1.tv_secs - sys0.tv_secs <= 1);
}

/* An address with 600 s to live is there after the clock goes a day on. */
void testCLOCKSTEP_ADDR_FORWARD(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:50::", &a);
    CU_ASSERT_EQUAL(send_prefix("2001:db8:4d54:50::", 600, 600), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    Delay(2 * TICKS_PER_SECOND);
    CU_ASSERT(have_addr(&a, NULL));
    clock_restore();
    send_prefix("2001:db8:4d54:50::", 0, 0);
    del_addr(&a);
}

/* An address with 4 s to live ends in 4 s after the clock goes a day back. */
void testCLOCKSTEP_ADDR_BACK(void)
{
    struct in6_addr a;
    NEED_IF();
    slaac_addr("2001:db8:4d54:51::", &a);
    CU_ASSERT_EQUAL(send_prefix("2001:db8:4d54:51::", 4, 2), 0);
    CU_ASSERT_FATAL(wait_for(&a, 1, 5));
    CU_ASSERT_EQUAL_FATAL(clock_step(-DAY), 0);
    CU_ASSERT(wait_for(&a, 0, 8));
    clock_restore();
    del_addr(&a);
}

/* A route with 600 s to live is there after the clock goes a day on. */
void testCLOCKSTEP_ROUTE_FORWARD(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_rio("2001:db8:4d54:52::", 600), 0);
    CU_ASSERT_FATAL(via_us("2001:db8:4d54:52::1"));
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    Delay(2 * TICKS_PER_SECOND);
    CU_ASSERT(via_us("2001:db8:4d54:52::1"));
    clock_restore();
    send_rio("2001:db8:4d54:52::", 0);
}

/* A route with 3 s to live ends in 3 s after the clock goes a day back. */
void testCLOCKSTEP_ROUTE_BACK(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_rio("2001:db8:4d54:53::", 3), 0);
    CU_ASSERT_FATAL(via_us("2001:db8:4d54:53::1"));
    CU_ASSERT_EQUAL_FATAL(clock_step(-DAY), 0);
    CU_ASSERT(wait_no_route("2001:db8:4d54:53::1", 7));
    clock_restore();
    send_rio("2001:db8:4d54:53::", 0);
}

/* A DNS server with 600 s to live still answers after the clock goes a day
 * on. The name asked after the step is one not asked before, so the
 * resolver's cache cannot answer for the server. */
void testCLOCKSTEP_RDNSS_FORWARD(void)
{
    NEED_IF();
    if (!dns_ready) {
        CU_PASS("the test's DNS server is not running - skipping");
        return;
    }
    CU_ASSERT_EQUAL_FATAL(send_rdnss(600), 0);
    CU_ASSERT_EQUAL(dns_lookup("one.rdnss.test"), 77);
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    Delay(2 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(dns_lookup("two.rdnss.test"), 78);
    clock_restore();
    send_rdnss(0);
}

/* A path MTU lowered for 600 s is still lowered after the clock goes a day
 * on (RFC 8201 section 4: kept at least 5 minutes). */
void testCLOCKSTEP_PMTU_FORWARD(void)
{
    struct rt_msghdr rtm;
    struct in6_addr gw;
    NEED_IF();
    CU_ASSERT_EQUAL(send_rio("2001:db8:4d54:54::", 600), 0);
    CU_ASSERT_FATAL(via_us("2001:db8:4d54:54::1"));
    CU_ASSERT(send_ptb("2001:db8:4d54:54::1", 1400) > 0);
    CU_ASSERT_FATAL(route_get("2001:db8:4d54:54::1", &gw, &rtm) == 1);
    CU_ASSERT_EQUAL_FATAL(rtm.rtm_rmx.rmx_mtu, 1400);
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    Delay(2 * TICKS_PER_SECOND);
    CU_ASSERT(route_get("2001:db8:4d54:54::1", &gw, &rtm) == 1 && rtm.rtm_rmx.rmx_mtu == 1400);
    clock_restore();
    send_rio("2001:db8:4d54:54::", 0);
}

/* An expiry given to a route, a time of day, reads back as it was. */
void testCLOCKSTEP_EXPIRE_GIVEN(void)
{
    struct rt_msghdr rtm;
    struct in6_addr gw;
    u_long t = (u_long)time(NULL) + 1000;
    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(host_route(RTM_ADD, "2001:db8:4d54:55::1", t), 0);
    CU_ASSERT_FATAL(route_get("2001:db8:4d54:55::1", &gw, &rtm) == 1);
    CU_ASSERT(rtm.rtm_rmx.rmx_expire + 1 >= t && rtm.rtm_rmx.rmx_expire <= t + 1);
    host_route(RTM_DELETE, "2001:db8:4d54:55::1", 0);
}

/* An answer cached for 600 s still answers after the clock goes a day on;
 * the server is gone by then, so only the cache can. */
void testCLOCKSTEP_DNSCACHE_FORWARD(void)
{
    NEED_IF();
    if (!dns_ready) {
        CU_PASS("the test's DNS server is not running - skipping");
        return;
    }
    CU_ASSERT_EQUAL_FATAL(send_rdnss(600), 0);
    CU_ASSERT_EQUAL_FATAL(dns_lookup("cached.rdnss.test"), 83);
    CU_ASSERT_EQUAL_FATAL(send_rdnss(0), 0);
    CU_ASSERT_EQUAL_FATAL(dns_lookup("cached.rdnss.test"), 83);    /* the cache's */
    CU_ASSERT_EQUAL_FATAL(clock_step(DAY), 0);
    CU_ASSERT_EQUAL(dns_lookup("cached.rdnss.test"), 83);
    clock_restore();
}

/* One cached for 3 s ends in 3 s after the clock goes a day back. */
void testCLOCKSTEP_DNSCACHE_BACK(void)
{
    NEED_IF();
    if (!dns_ready) {
        CU_PASS("the test's DNS server is not running - skipping");
        return;
    }
    CU_ASSERT_EQUAL_FATAL(send_rdnss(600), 0);
    CU_ASSERT_EQUAL_FATAL(dns_lookup("short.rdnss.test"), 84);
    CU_ASSERT_EQUAL_FATAL(send_rdnss(0), 0);
    CU_ASSERT_EQUAL_FATAL(clock_step(-DAY), 0);
    Delay(5 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(dns_lookup("short.rdnss.test"), 0);
    clock_restore();
}

/* -------------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    CU_pSuite p;
    int server = 0;
    struct in6_addr gw;

    if (argc > 1 && strcmp(argv[1], "--serve") == 0)
        return dns_serve();
    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping CLOCKSTEP tests\n");
        return 0;
    }
    if (!timer_open()) {
        printf("timer.device not available - skipping CLOCKSTEP tests\n");
        CloseLibrary(SocketBase);
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && have_addr(NULL, &linklocal);
    if (!ready)
        printf("%s: no interface with a link-local address - skipping CLOCKSTEP tests\n", ifname);
    else if (route_get("2001:db8:ffff::1", &gw, NULL) != 0) {
        printf("%s: the link has a default router - skipping CLOCKSTEP tests\n", ifname);
        ready = 0;
    } else if (dns_other_server())
        printf("another DNS server answers first - skipping the RDNSS case\n");
    else if (!(server = dns_start(argv[0])))
        printf("the test's DNS server did not start - skipping the RDNSS case\n");
    dns_ready = server;

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("CLOCKSTEP_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a step moves the system time only", testCLOCKSTEP_STEP) ||
        !CU_add_test(p, "an address outlives a step forward", testCLOCKSTEP_ADDR_FORWARD) ||
        !CU_add_test(p, "an address ends on time after a step back", testCLOCKSTEP_ADDR_BACK) ||
        !CU_add_test(p, "an RIO route outlives a step forward", testCLOCKSTEP_ROUTE_FORWARD) ||
        !CU_add_test(p, "an RIO route ends on time after a step back", testCLOCKSTEP_ROUTE_BACK) ||
        !CU_add_test(p, "an RDNSS server outlives a step forward", testCLOCKSTEP_RDNSS_FORWARD) ||
        !CU_add_test(p, "a path MTU outlives a step forward", testCLOCKSTEP_PMTU_FORWARD) ||
        !CU_add_test(p, "a route's expiry reads back as given", testCLOCKSTEP_EXPIRE_GIVEN) ||
        !CU_add_test(p, "a cached answer outlives a step forward", testCLOCKSTEP_DNSCACHE_FORWARD) ||
        !CU_add_test(p, "a cached answer ends on time after a step back", testCLOCKSTEP_DNSCACHE_BACK))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-CLOCKSTEP");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    clock_restore();
    if (server)
        dns_stop();
    timer_close();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
