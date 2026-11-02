/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the validation of ICMPv6 Redirects (RFC 4861 section
    8.1): a Redirect is taken only from the router that is the first hop to
    its destination now - not from another router on the link, and, once a
    destination has been redirected, not from the router it was redirected
    from - and only with code 0 and options that parse; its target must be
    a link-local address or the destination itself. A valid one routes the
    destination through its target (section 8.3).

    As cunit-net-rio6, the test sends the Router Advertisements and the
    Redirects itself, from a raw ICMPv6 socket with hop limit 255 to the
    interface's own link-local address (router 1); a second link-local
    address on the interface stands for router 2, the less preferred
    default router. The routing table is read back with RTM_GET.

    It needs an interface with no IPv6 router of its own (on hosted AROS,
    tap.device attached to a host tap that sends no Router Advertisements).
    Usage:  cunit-net-redirect6 [IFNAME]  (default eth0). Without such an
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
#define PRF_LOW     0x18

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/* the destinations redirected, and the targets */
static const char *const dests[] = {
    "2001:db8:4d54:60::1", "2001:db8:4d54:60::2", "2001:db8:4d54:60::3",
    "2001:db8:4d54:60::4", "2001:db8:4d54:60::5", "2001:db8:4d54:60::6",
};
static const u_char none[1];     /* no options */

#define T1  "fe80::4d54:9"
#define T2  "fe80::4d54:a"

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ------------------------------------------------------------- messages */

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

/* Send a Redirect from src for dst to target, with this code and the
 * options in opts. */
static int send_redirect(const struct in6_addr *src, int code, const char *dst,
    const char *target, const u_char *opts, int optlen)
{
    u_char pkt[128];
    struct nd_redirect *rd = (struct nd_redirect *)pkt;
    struct sockaddr_in6 a;
    int s, hops = 255, n;

    bzero(pkt, sizeof(pkt));
    rd->nd_rd_type = ND_REDIRECT;
    rd->nd_rd_code = code;
    inet_pton(AF_INET6, (char *)target, &rd->nd_rd_target);
    inet_pton(AF_INET6, (char *)dst, &rd->nd_rd_dst);
    memcpy(pkt + sizeof(*rd), opts, optlen);

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
    a.sin6_addr = r1;
    n = sendto(s, (char *)pkt, sizeof(*rd) + optlen, 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(25);
    return n == (int)(sizeof(*rd) + optlen) ? 0 : -1;
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

static int via_str(const char *dst, const char *router)
{
    struct in6_addr r;
    inet_pton(AF_INET6, (char *)router, &r);
    return via(dst, &r);
}

/* Delete the host route a Redirect made for dst, if there is one. */
static void route_del_host(const char *dst)
{
    long buf[256 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    int s = socket(PF_ROUTE, SOCK_RAW, 0);

    if (s < 0)
        return;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_DELETE;
    h->rtm_flags = RTF_HOST;
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = 1;
    sa = (struct sockaddr_in6 *)(h + 1);
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)dst, &sa->sin6_addr);
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(*sa));
    send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
}

/* ----------------------------------------------------------------- tests */

int init_suite(void)
{
    if (SocketBase == NULL)
        return -1;
    if (ready) {
        /* router 1 the preferred default router, router 2 the other */
        send_ra(&r1, 600, PRF_HIGH, none, 0);
        send_ra(&r2, 600, PRF_LOW, none, 0);
    }
    return 0;
}

int clean_suite(void)
{
    if (ready) {
        unsigned i;

        for (i = 0; i < sizeof(dests) / sizeof(dests[0]); i++)
            route_del_host(dests[i]);
        send_ra(&r1, 0, PRF_HIGH, none, 0);
        send_ra(&r2, 0, PRF_LOW, none, 0);
    }
    return 0;
}

/* From the first hop: the destination goes through the target. */
void testREDIRECT6_FIRST_HOP(void)
{
    NEED_IF();
    CU_ASSERT_FATAL(via(dests[0], &r1));
    CU_ASSERT_EQUAL(send_redirect(&r1, 0, dests[0], T1, none, 0), 0);
    CU_ASSERT(via_str(dests[0], T1));
}

/* From another router on the link: ignored. */
void testREDIRECT6_OTHER_ROUTER(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_redirect(&r2, 0, dests[1], T1, none, 0), 0);
    CU_ASSERT(via(dests[1], &r1));
}

/* Once redirected, the router it was redirected from is no longer the
 * first hop. */
void testREDIRECT6_REDIRECTED(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_redirect(&r1, 0, dests[2], T1, none, 0), 0);
    CU_ASSERT_FATAL(via_str(dests[2], T1));
    CU_ASSERT_EQUAL(send_redirect(&r1, 0, dests[2], T2, none, 0), 0);
    CU_ASSERT(via_str(dests[2], T1));
}

/* A code other than 0: ignored. */
void testREDIRECT6_CODE(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_redirect(&r1, 1, dests[3], T1, none, 0), 0);
    CU_ASSERT(via(dests[3], &r1));
}

/* An option of length 0: the whole message is ignored. */
void testREDIRECT6_BAD_OPTION(void)
{
    static const u_char opt[8] = { ND_OPT_TARGET_LINKADDR, 0 };

    NEED_IF();
    CU_ASSERT_EQUAL(send_redirect(&r1, 0, dests[4], T1, opt, sizeof(opt)), 0);
    CU_ASSERT(via(dests[4], &r1));
}

/* A target neither link-local nor the destination: ignored. */
void testREDIRECT6_GLOBAL_TARGET(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_redirect(&r1, 0, dests[5], "2001:db8:4d54:61::9", none, 0), 0);
    CU_ASSERT(via(dests[5], &r1));
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
        printf("bsdsocket.library not available - skipping REDIRECT6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no interface without a router - skipping REDIRECT6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("REDIRECT6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "from the first hop, it is taken", testREDIRECT6_FIRST_HOP) ||
        !CU_add_test(p, "from another router, it is not", testREDIRECT6_OTHER_ROUTER) ||
        !CU_add_test(p, "nor from the router redirected from", testREDIRECT6_REDIRECTED) ||
        !CU_add_test(p, "nor with a code other than 0", testREDIRECT6_CODE) ||
        !CU_add_test(p, "nor with an option of length 0", testREDIRECT6_BAD_OPTION) ||
        !CU_add_test(p, "nor with a global target not the destination", testREDIRECT6_GLOBAL_TARGET))
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
    CU_set_output_filename("Net-REDIRECT6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    teardown();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
