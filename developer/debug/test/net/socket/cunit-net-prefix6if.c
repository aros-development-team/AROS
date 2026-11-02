/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for a prefix advertised on two links (RFC 4861 section
    6.3.4): it is on-link on each link it is advertised on, for the
    lifetime advertised there - its route on the interface of that link,
    also when it is not the first interface. The lifetime heard on one link does not end
    it on the other, and when it ends on the link that has the route to
    it - the routing table holds one route for a prefix - the route goes
    to the other link, where it is still on-link.

    The test sends the Router Advertisements itself, each to its
    interface's own link-local address, as cunit-net-slaac6 does, router
    lifetime 0, one Prefix Information Option (on-link, autonomous). The
    route to the prefix is read back with RTM_GET, with its interface.
    Usage:  cunit-net-prefix6if [IFNAME1 IFNAME2]  (default eth0 eth1).
    Without two interfaces with link-local addresses the tests are skipped
    (on hosted AROS: AROS_TAP2_IP, a second tap.device unit).
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname[2] = { "eth0", "eth1" };
static unsigned ifindex[2];
static struct in6_addr linklocal[2];
static int ready;

#define P1  "2001:db8:4d54:90::"
#define P2  "2001:db8:4d54:91::"
#define P3  "2001:db8:4d54:92::"

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#define NEED_IF() do { if (!ready) { CU_PASS("no two suitable interfaces - skipping"); return; } } while (0)

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

/* The link-local address of interface i into *ll. */
static int get_linklocal(int i, struct in6_addr *ll)
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
            if (strcmp(req[k].ifr_name, ifname[i]) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[k].ifr_ifru.ifru_addr.sin6_addr)) {
                *ll = req[k].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

/* A Router Advertisement on interface i, router lifetime 0, with prefix/64
 * on-link and autonomous for this valid (and preferred) lifetime. */
static int send_ra(int i, const char *prefix, u_int32_t valid)
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
    a.sin6_addr = linklocal[i];
    a.sin6_scope_id = ifindex[i];
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) != 0) {
        CloseSocket(s);
        return -1;
    }
    n = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    Delay(25);
    return n == (int)sizeof(pkt) ? 0 : -1;
}

/* The interface index of the on-link route the table has for dst, via
 * RTM_GET; 0 when it has none (or one through a router). */
static unsigned route_ifindex(const char *dst)
{
    static int seq;
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa;
    int s, n = 0, want = ++seq, bit;
    char *p;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return 0;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_GET;
    h->rtm_addrs = RTA_DST | RTA_IFP;
    h->rtm_seq = want;
    sa = (struct sockaddr_in6 *)(h + 1);
    sa->sin6_len = sizeof(*sa);
    sa->sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)dst, &sa->sin6_addr);
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)buf, h->rtm_msglen, 0) < 0) {
        CloseSocket(s);
        return 0;
    }
    while (sock_wait(s, 0, 2) > 0) {
        n = recv(s, (char *)buf, sizeof(buf), 0);
        if (n <= 0 || (h->rtm_type == RTM_GET && h->rtm_seq == want))
            break;
    }
    CloseSocket(s);
    if (n <= 0 || h->rtm_errno != 0 || (h->rtm_flags & RTF_GATEWAY) ||
            !(h->rtm_addrs & RTA_IFP))
        return 0;
    p = (char *)(h + 1);
    for (bit = 1; bit < RTA_IFP; bit <<= 1)
        if (h->rtm_addrs & bit)
            p += ROUNDUP(((struct sockaddr *)p)->sa_len);
    return ((struct sockaddr_dl *)p)->sdl_family == AF_LINK ?
           ((struct sockaddr_dl *)p)->sdl_index : 0;
}

/* Delete the address SLAAC made from prefix on interface i. */
static void del_slaac(int i, const char *prefix)
{
    struct in6_ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname[i], sizeof(ifr.ifr_name) - 1);
    ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
    ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)prefix, &ifr.ifr_ifru.ifru_addr.sin6_addr);
    memcpy(&ifr.ifr_ifru.ifru_addr.sin6_addr.s6_addr[8], &linklocal[i].s6_addr[8], 8);
    IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    CloseSocket(s);
}

static void forget(const char *prefix)
{
    int i;

    for (i = 0; i < 2; i++) {
        send_ra(i, prefix, 0);
        del_slaac(i, prefix);
    }
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A short lifetime heard on the second link does not end the prefix on
 * the first, which keeps its route. */
void testPREFIX6IF_LIFETIMES_APART(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_ra(0, P1, 600), 0);
    CU_ASSERT_EQUAL_FATAL(route_ifindex(P1 "1"), ifindex[0]);
    CU_ASSERT_EQUAL(send_ra(1, P1, 3), 0);
    Delay(5 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(route_ifindex(P1 "1"), ifindex[0]);
    forget(P1);
}

/* When it ends on the link with the route, the route goes to the other
 * link, where it is still on-link. */
void testPREFIX6IF_HANDOVER(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_ra(0, P2, 3), 0);
    CU_ASSERT_EQUAL_FATAL(route_ifindex(P2 "1"), ifindex[0]);
    CU_ASSERT_EQUAL(send_ra(1, P2, 600), 0);
    Delay(5 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(route_ifindex(P2 "1"), ifindex[1]);
    forget(P2);
}

/* A prefix heard only on the second link is on-link there. */
void testPREFIX6IF_SECOND_LINK(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_ra(1, P3, 600), 0);
    CU_ASSERT_EQUAL(route_ifindex(P3 "1"), ifindex[1]);
    forget(P3);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int i;

    if (argc > 2) {
        ifname[0] = argv[1];
        ifname[1] = argv[2];
    }
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping PREFIX6IF tests\n");
        return 0;
    }
    ready = 1;
    for (i = 0; i < 2; i++)
        if (index_of(ifname[i], &ifindex[i]) != 0 || !get_linklocal(i, &linklocal[i]))
            ready = 0;
    if (!ready)
        printf("%s, %s: not two interfaces with link-local addresses - skipping PREFIX6IF tests\n",
               ifname[0], ifname[1]);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PREFIX6IF_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a prefix of the second link is on-link there", testPREFIX6IF_SECOND_LINK) ||
        !CU_add_test(p, "the lifetime on one link does not end it on the other", testPREFIX6IF_LIFETIMES_APART) ||
        !CU_add_test(p, "the route goes to the link where it is still on-link", testPREFIX6IF_HANDOVER))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-PREFIX6IF");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
