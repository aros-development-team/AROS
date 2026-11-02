/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the flags of Neighbor Advertisements (RFC 4861 section
    4.4), which are in network byte order: the Router, Solicited and
    Override bits are the top three of the first octet. A Neighbor
    Advertisement the stack sends in answer to a Neighbor Solicitation for
    its address has Solicited and Override set there; one it receives with
    Solicited set makes the neighbour REACHABLE (section 7.2.5), so traffic
    to it sends no Neighbor Solicitation for the reachable time - where an
    entry left STALE would be probed 5 s after the first packet (section
    7.3.3).

    The test sends the Neighbor Solicitation and the Neighbor Advertisement
    itself, from a raw ICMPv6 socket on a second link-local address of the
    interface, fe80::4d54:2, to the interface's own link-local address; the
    neighbour of the second case, fe80::4d54:6e, does not exist, and the
    Neighbor Solicitations for it are watched with the bsdsocket BPF calls.
    Usage:  cunit-net-naflags6 [IFNAME]  (default eth0). Without an
    interface with a link-local address the tests are skipped.
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

#define NEIGHBOUR   "fe80::4d54:6e"   /* no other test uses it: it is left REACHABLE */

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

/* While sending a datagram to target twice a second for secs, the number
 * of Neighbor Solicitations for it on the link. */
static int ns_while_sending(const char *target, int secs)
{
    struct in6_addr want, got;
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
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long n;

        if (ticks % 5 == 0)
            sendto(s, (char *)"x", 1, 0, (struct sockaddr *)&d, sizeof(d));
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                if (ns_target((u_char *)p + h->bh_hdrlen, h->bh_caplen, &got) &&
                        memcmp(&got, &want, sizeof(got)) == 0)
                    count++;
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

/* Asked for its address, the stack answers with Solicited and Override,
 * in the first octet of the flags. */
void testNAFLAGS6_SENT(void)
{
    struct nd_neighbor_solicit ns;
    u_char r[256];
    int s, i, n, found = 0;

    NEED_IF();
    CU_ASSERT_FATAL((s = nd_socket()) >= 0);
    bzero(&ns, sizeof(ns));
    ns.nd_ns_type = ND_NEIGHBOR_SOLICIT;
    ns.nd_ns_target = r1;
    CU_ASSERT_EQUAL(to_r1(s, &ns, sizeof(ns)), 0);
    for (i = 0; i < 4 && !found; i++) {
        if (sock_wait(s, 0, 1) <= 0)
            continue;
        n = recv(s, (char *)r, sizeof(r), 0);
        if (n >= (int)sizeof(struct nd_neighbor_advert) && r[0] == ND_NEIGHBOR_ADVERT &&
                memcmp(&((struct nd_neighbor_advert *)r)->nd_na_target, &r1, sizeof(r1)) == 0) {
            found = 1;
            CU_ASSERT_EQUAL(r[4] & 0xe0, 0x60);         /* S, O; not R */
            CU_ASSERT_EQUAL(r[5] | r[6] | r[7], 0);
        }
    }
    CU_ASSERT(found);
    CloseSocket(s);
}

/* An answer with Solicited set makes the neighbour REACHABLE: no probe in
 * the next 8 s of traffic to it. */
void testNAFLAGS6_RECEIVED(void)
{
    struct { struct nd_neighbor_advert na; u_char opt[8]; } na;
    struct sockaddr_in6 d;
    int s, u;

    NEED_IF();
    CU_ASSERT_FATAL((s = nd_socket()) >= 0);
    /* the first datagram starts address resolution */
    bzero(&d, sizeof(d));
    d.sin6_len = sizeof(d);
    d.sin6_family = AF_INET6;
    inet_pton(AF_INET6, NEIGHBOUR, &d.sin6_addr);
    d.sin6_port = htons(9);
    d.sin6_scope_id = ifindex;
    u = socket(AF_INET6, SOCK_DGRAM, 0);
    sendto(u, (char *)"x", 1, 0, (struct sockaddr *)&d, sizeof(d));
    CloseSocket(u);
    Delay(10);
    /* the neighbour's answer: Solicited, Override, its link-layer address */
    bzero(&na, sizeof(na));
    na.na.nd_na_type = ND_NEIGHBOR_ADVERT;
    na.na.nd_na_flags_reserved = ND_NA_FLAG_SOLICITED | ND_NA_FLAG_OVERRIDE;
    inet_pton(AF_INET6, NEIGHBOUR, &na.na.nd_na_target);
    na.opt[0] = ND_OPT_TARGET_LINKADDR;
    na.opt[1] = 1;
    na.opt[2] = 0x02; na.opt[3] = 0x00; na.opt[4] = 0x4d;
    na.opt[5] = 0x54; na.opt[6] = 0x00; na.opt[7] = 0x09;
    CU_ASSERT_EQUAL(to_r1(s, &na, sizeof(na)), 0);
    CloseSocket(s);
    Delay(10);
    CU_ASSERT_EQUAL(ns_while_sending(NEIGHBOUR, 8), 0);
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
        printf("bsdsocket.library 4 not available - skipping NAFLAGS6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no suitable interface - skipping NAFLAGS6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        teardown();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NAFLAGS6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an answer we send has Solicited and Override", testNAFLAGS6_SENT) ||
        !CU_add_test(p, "a Solicited answer we receive makes it REACHABLE", testNAFLAGS6_RECEIVED))
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
    CU_set_output_filename("Net-NAFLAGS6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    teardown();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
