/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv6 routes through a router. A route added with a
    gateway must keep that gateway, a default route (::/0) must cover every
    destination, and a datagram that follows such a route must be sent to the
    router: neighbour discovery asks for the router's address, not the
    destination's (RFC 4861 section 7.2.2).

    The routes are added through a routing socket (RTM_ADD) and read back with
    RTM_GET; the Neighbor Solicitation is watched with the bsdsocket BPF calls.
    The test needs a multicast-capable interface with an IPv6 link-local
    address and no default route of its own (on hosted AROS, tap.device
    attached to a host tap). Usage:  cunit-net-route6 [IFNAME]  (default eth0).
    Without such an interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/route.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static int ready;
static long bpf = -1;

#define GW      "fe80::4d54:9"          /* a router on the link; nobody answers */
#define NET     "2001:db8:4d54:40::"    /* reached through it */
#define DST     "2001:db8:4d54:40::1"
#define SRCNET  "2001:db8:4d54:41::1"   /* an address of ours to send from */

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/* ---------------------------------------------------------------- routes */

static void sa6(struct sockaddr_in6 *a, const char *addr)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
}

/* Add or delete dst/plen through gw with a routing socket. */
static int route(int type, const char *dst, int plen, const char *gw)
{
    union { struct rt_msghdr h; char b[512]; } msg;
    struct sockaddr_in6 *sa;
    char *p;
    int s, i, r;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.h.rtm_version = RTM_VERSION;
    msg.h.rtm_type = type;
    msg.h.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    msg.h.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    msg.h.rtm_seq = 1;
    p = (char *)(&msg.h + 1);
    sa = (struct sockaddr_in6 *)p; sa6(sa, dst); p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p; sa6(sa, gw);  p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in6 *)p; sa6(sa, NULL);
    for (i = 0; i < plen / 8; i++)
        sa->sin6_addr.s6_addr[i] = 0xff;
    p += ROUNDUP(sizeof(*sa));
    msg.h.rtm_msglen = p - (char *)&msg;
    r = send(s, (char *)&msg, msg.h.rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* The gateway of the route the table uses for dst, via RTM_GET: 1 with *gw
 * set for an IPv6 gateway, 2 for a gateway of another family (a link-layer
 * address put in its place), 0 for no gateway route. */
static int route_get(const char *dst, struct in6_addr *gw)
{
    union { struct rt_msghdr h; char b[512]; } msg;
    struct sockaddr_in6 *sa;
    char *p;
    int s, n = 0;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.h.rtm_version = RTM_VERSION;
    msg.h.rtm_type = RTM_GET;
    msg.h.rtm_addrs = RTA_DST;
    msg.h.rtm_seq = 2;
    sa = (struct sockaddr_in6 *)(&msg.h + 1);
    sa6(sa, dst);
    msg.h.rtm_msglen = sizeof(msg.h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)&msg, msg.h.rtm_msglen, 0) < 0) {
        CloseSocket(s);
        return 0;
    }
    while (sock_wait(s, 0, 2) > 0) {
        n = recv(s, (char *)&msg, sizeof(msg), 0);
        if (n <= 0 || (msg.h.rtm_type == RTM_GET && msg.h.rtm_seq == 2))
            break;
    }
    CloseSocket(s);
    if (n <= 0 || msg.h.rtm_errno != 0 || !(msg.h.rtm_flags & RTF_GATEWAY) ||
            !(msg.h.rtm_addrs & RTA_GATEWAY))
        return 0;
    p = (char *)(&msg.h + 1);
    p += ROUNDUP(((struct sockaddr *)p)->sa_len);       /* past RTA_DST */
    sa = (struct sockaddr_in6 *)p;
    if (sa->sin6_family != AF_INET6)
        return 2;
    *gw = sa->sin6_addr;
    return 1;
}

static int via_gw(const char *dst)
{
    struct in6_addr gw, want;
    inet_pton(AF_INET6, GW, &want);
    return route_get(dst, &gw) == 1 && memcmp(&gw, &want, sizeof(gw)) == 0;
}

/* ----------------------------------------------------------- an address */

static int addr6(int add, const char *addr)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r, i;
    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;
        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        sa6(&ifra.ifra_addr, addr);
        sa6(&ifra.ifra_prefixmask, NULL);
        for (i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        sa6(&ifr.ifr_ifru.ifru_addr, addr);
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* -------------------------------------------------------------- capture */

static int bpf_start(void)
{
    u_int one = 1;
    char buf[BPF_DFLTBUFSIZE];

    bpf = bpf_open(0);
    if (bpf < 0)
        return -1;
    if (bpf_ioctl(bpf, BIOCSETIF, (char *)ifname) != 0 ||
            bpf_ioctl(bpf, BIOCIMMEDIATE, (char *)&one) != 0) {
        bpf_close(bpf);
        bpf = -1;
        return -1;
    }
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    return 0;
}

/* The target of a Neighbor Solicitation in one captured frame, if it is one. */
static int ns_target(const u_char *f, u_int len, struct in6_addr *tgt)
{
    const u_char *p, *end = f + len;

    if (len >= 14 + 40 && f[12] == 0x86 && f[13] == 0xdd)
        p = f + 14;
    else if (len >= 40 && (f[0] >> 4) == 6)
        p = f;
    else
        return 0;
    if (p + 40 + 24 > end || p[6] != IPPROTO_ICMPV6 || p[40] != ND_NEIGHBOR_SOLICIT)
        return 0;
    memcpy(tgt, p + 40 + 8, sizeof(*tgt));
    return 1;
}

/* Wait up to secs for a Neighbor Solicitation for target. */
static int ns_for(const char *target, int secs)
{
    struct in6_addr want, got;
    char buf[BPF_DFLTBUFSIZE];
    int ticks;

    inet_pton(AF_INET6, (char *)target, &want);
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long n;
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                if (ns_target((u_char *)p + h->bh_hdrlen, h->bh_caplen, &got) &&
                        memcmp(&got, &want, sizeof(got)) == 0)
                    return 1;
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
    return 0;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A route added through a router keeps the router as its gateway. */
void testROUTE6_KEEPS_GATEWAY(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(route(RTM_ADD, NET, 64, GW), 0);
    CU_ASSERT(via_gw(DST));
    route(RTM_DELETE, NET, 64, GW);
}

/* A default route covers every destination. */
void testROUTE6_DEFAULT_COVERS_ALL(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(route(RTM_ADD, "::", 0, GW), 0);
    CU_ASSERT(via_gw("2001:db8:ffff::1"));
    CU_ASSERT(via_gw("2001:db8:4d54:fff::1"));
    route(RTM_DELETE, "::", 0, GW);
}

/* A datagram that follows a route through a router makes the host ask for
 * the router, not for the destination. */
void testROUTE6_ASKS_FOR_ROUTER(void)
{
    struct sockaddr_in6 d;
    int s;

    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, SRCNET), 0);
    Delay(3 * 50);      /* the new address is tentative until DAD completes */
    CU_ASSERT_EQUAL(route(RTM_ADD, NET, 64, GW), 0);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    sa6(&d, DST);
    d.sin6_port = htons(9);
    CU_ASSERT_EQUAL(sendto(s, "hop", 3, 0, (struct sockaddr *)&d, sizeof(d)), 3);
    CU_ASSERT(ns_for(GW, 3));
    CloseSocket(s);
    route(RTM_DELETE, NET, 64, GW);
    addr6(0, SRCNET);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    struct ifreq ifr;
    struct in6_addr gw;
    int s;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ROUTE6 tests\n");
        return 0;
    }

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (s >= 0 && IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
            (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
            route_get("2001:db8:ffff::1", &gw) == 0 && bpf_start() == 0)
        ready = 1;
    else
        printf("%s: no interface without a default route, or no BPF - skipping ROUTE6 tests\n", ifname);
    if (s >= 0)
        CloseSocket(s);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ROUTE6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a route through a router keeps its gateway", testROUTE6_KEEPS_GATEWAY) ||
        !CU_add_test(p, "a default route covers every destination", testROUTE6_DEFAULT_COVERS_ALL) ||
        !CU_add_test(p, "a routed datagram asks for the router", testROUTE6_ASKS_FOR_ROUTER))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ROUTE6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
