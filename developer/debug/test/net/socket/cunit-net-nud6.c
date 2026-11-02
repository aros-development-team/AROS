/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for neighbour unreachability detection and address
    resolution (RFC 4861 sections 7.2.2 and 7.3.3): a neighbour in use whose
    reachability is not confirmed is probed with unicast Neighbor
    Solicitations - to its address, at the link-layer address cached - and
    the solicitations for a neighbour that does not answer, multicast to
    its solicited-node group while it is being resolved, go a RetransTimer
    (1 s) apart, three of them, however many packets wait for it.

    The test answers for the first neighbour itself, with an unsolicited
    Neighbor Advertisement from a second link-local address of the
    interface, fe80::4d54:2, which leaves its entry STALE; the second never
    answers. The solicitations are watched with the bsdsocket BPF calls.
    Usage:  cunit-net-nud6 [IFNAME]  (default eth0). Without an interface
    with a link-local address the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr r1;      /* the interface's link-local address */
static struct in6_addr r2;      /* fe80::4d54:2, added */
static int bpf = -1;
static int ready;

#define PROBED      "fe80::4d54:70"     /* answered for, then probed */
#define SILENT      "fe80::4d54:71"     /* never answers */
static const u_char probed_mac[6] = { 0x02, 0x00, 0x4d, 0x54, 0x00, 0x70 };

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

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

/* One Neighbor Solicitation seen: when, for whom, to which address and
 * link-layer address. */
struct ns_seen {
    double t;
    struct in6_addr target, dst;
    u_char ether_dst[6];
};

/* While sending a datagram to target every period ticks for secs, the
 * Neighbor Solicitations for it on the link, at most max of them. */
static int watch_ns(const char *target, int secs, int period, struct ns_seen *seen, int max)
{
    struct in6_addr want;
    struct sockaddr_in6 d;
    char buf[BPF_DFLTBUFSIZE];
    int s, ticks, count = 0;

    inet_pton(AF_INET6, (char *)target, &want);
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;                               /* what came before */
    bzero(&d, sizeof(d));
    d.sin6_len = sizeof(d);
    d.sin6_family = AF_INET6;
    d.sin6_addr = want;
    d.sin6_port = htons(9);
    d.sin6_scope_id = ifindex;
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    for (ticks = 0; ticks < secs * 50; ticks += 5) {
        long n;

        if (ticks % period == 0)
            sendto(s, (char *)"x", 1, 0, (struct sockaddr *)&d, sizeof(d));
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                const u_char *f = (u_char *)p + h->bh_hdrlen, *ip = f + 14;

                if (h->bh_caplen >= 14 + 40 + 24 && f[12] == 0x86 && f[13] == 0xdd &&
                        ip[6] == IPPROTO_ICMPV6 && ip[40] == ND_NEIGHBOR_SOLICIT &&
                        memcmp(ip + 48, &want, 16) == 0 && count < max) {
                    seen[count].t = h->bh_tstamp.tv_sec + h->bh_tstamp.tv_usec / 1e6;
                    memcpy(&seen[count].target, ip + 48, 16);
                    memcpy(&seen[count].dst, ip + 24, 16);
                    memcpy(seen[count].ether_dst, f, 6);
                    count++;
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
    CloseSocket(s);
    return count;
}

/* ------------------------------------------------------------- messages */

/* A raw ICMPv6 socket on fe80::4d54:2, hop limit 255. */
static int nd_socket(void)
{
    struct sockaddr_in6 a;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), hops = 255;

    if (s < 0)
        return -1;
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
    return s;
}

static int to_r1(int s, const void *pkt, int len)
{
    struct sockaddr_in6 a;

    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = r1;
    a.sin6_scope_id = ifindex;
    return sendto(s, (char *)pkt, len, 0, (struct sockaddr *)&a, sizeof(a)) == len ? 0 : -1;
}

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A STALE neighbour in use is probed: to its own address and link-layer
 * address, not to its solicited-node group. */
void testNUD6_PROBE_UNICAST(void)
{
    struct { struct nd_neighbor_advert na; u_char opt[8]; } na;
    struct ns_seen seen[8];
    struct sockaddr_in6 d;
    struct in6_addr probed;
    int s, u, n;

    NEED_IF();
    inet_pton(AF_INET6, PROBED, &probed);
    /* resolution starts; an unsolicited answer leaves the entry STALE */
    bzero(&d, sizeof(d));
    d.sin6_len = sizeof(d);
    d.sin6_family = AF_INET6;
    d.sin6_addr = probed;
    d.sin6_port = htons(9);
    d.sin6_scope_id = ifindex;
    u = socket(AF_INET6, SOCK_DGRAM, 0);
    sendto(u, (char *)"x", 1, 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(u);
    Delay(10);
    CU_ASSERT_FATAL((s = nd_socket()) >= 0);
    bzero(&na, sizeof(na));
    na.na.nd_na_type = ND_NEIGHBOR_ADVERT;
    na.na.nd_na_flags_reserved = ND_NA_FLAG_OVERRIDE;
    na.na.nd_na_target = probed;
    na.opt[0] = ND_OPT_TARGET_LINKADDR;
    na.opt[1] = 1;
    memcpy(na.opt + 2, probed_mac, 6);
    CU_ASSERT_EQUAL(to_r1(s, &na, sizeof(na)), 0);
    CloseSocket(s);
    Delay(10);
    /* in use: DELAY, then PROBE 5 s later */
    n = watch_ns(PROBED, 9, 25, seen, 8);
    CU_ASSERT_FATAL(n >= 1);
    CU_ASSERT(memcmp(&seen[0].dst, &probed, sizeof(probed)) == 0);
    CU_ASSERT(memcmp(seen[0].ether_dst, probed_mac, 6) == 0);
}

/* A neighbour that does not answer: three solicitations, a RetransTimer
 * apart, with a packet for it every fifth of a second - then a new
 * resolution, for a later packet, no sooner. */
void testNUD6_RETRANS_TIMER(void)
{
    struct ns_seen seen[16];
    int n, i;

    NEED_IF();
    n = watch_ns(SILENT, 5, 10, seen, 16);
    CU_ASSERT_FATAL(n >= 3);
    for (i = 1; i < 3; i++)
        CU_ASSERT(seen[i].t - seen[i - 1].t >= 0.9);
    /* then it gives up; a later packet starts resolution again */
    CU_ASSERT(n < 4 || seen[3].t - seen[0].t >= 2.9);
    CU_ASSERT(seen[0].dst.s6_addr[0] == 0xff);              /* multicast, resolving */
}

/* -------------------------------------------------------------------- setup */

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    struct in6_aliasreq ifra;
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
    return bpf_start() == 0;
}

static void teardown(void)
{
    struct in6_ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (bpf >= 0)
        bpf_close(bpf);
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
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping NUD6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no suitable interface - skipping NUD6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NUD6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a STALE neighbour in use is probed unicast", testNUD6_PROBE_UNICAST) ||
        !CU_add_test(p, "solicitations go a RetransTimer apart", testNUD6_RETRANS_TIMER))
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
    CU_set_output_filename("Net-NUD6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    teardown();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
