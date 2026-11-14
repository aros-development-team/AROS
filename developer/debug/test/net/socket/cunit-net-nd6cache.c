/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the IPv6 neighbour cache (RFC 4861 section 5.1): an
    entry per neighbour, and a neighbour is an address on a link - the same
    link-local address on two links is two neighbours, each with its own
    link-layer address, and sending to one does not disturb the other.

    The entries are managed with a routing socket, RTF_LLINFO messages as
    ndp -s and -d send (RTM_ADD with a link-layer gateway, RTM_GET,
    RTM_DELETE), and listed with GetRouteInfo(AF_INET6, RTF_LLINFO), as
    ip -6 neigh lists them. A message without RTF_LLINFO is routing as
    before. What leaves each link is read with the bsdsocket BPF calls.

    The cache of an interface holds nd6_maxnbrs (256) entries; an entry
    with packets waiting for its address to be resolved is never given up
    for a new one, which then fails (ENOBUFS).

    Needs two multicast-capable interfaces (on hosted AROS, tap.device on
    two host taps). Usage:  cunit-net-nd6cache [IF1 [IF2]]  (default eth0
    eth1). Without them the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton(), GetRouteInfo() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/bpf.h>
#include <net/route.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname[2] = { "eth0", "eth1" };
static unsigned ifindex[2];
static int have_ifs;
static long bpf[2] = { -1, -1 };

#define TWIN    "fe80::4d54:99"         /* set by hand on both links */
#define NOBODY  "fe80::4d54:1:0"        /* + n: addresses nobody has */
#define MAXNBRS 256                     /* nd6_maxnbrs */
#define FLOOD   300

static const unsigned char mac[2][6] = {
    { 0x02, 0x00, 0x4d, 0x54, 0x00, 0xa0 },
    { 0x02, 0x00, 0x4d, 0x54, 0x00, 0xa1 },
};

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, unsigned scope, int port)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_port = htons(port);
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_scope_id = scope;
}

static void nobody(struct in6_addr *a, int n)
{
    inet_pton(AF_INET6, (char *)NOBODY, a);
    a->s6_addr[14] = n >> 8;
    a->s6_addr[15] = n;
}

/*
 * One routing socket message about the neighbour addr on link i (-1: no
 * interface named) and its answer. lladdr, for RTM_ADD, is the gateway.
 * With llinfo 0 the message is about a route, not a neighbour. Returns
 * rtm_errno (0 on success); the answer's link-layer address and flags go
 * to got and flags.
 */
static int rtmsg(int type, const struct in6_addr *addr, int i, const unsigned char *lladdr,
                 int llinfo, unsigned char *got, int *flags)
{
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *dst = (struct sockaddr_in6 *)(h + 1);
    int s = socket(PF_ROUTE, SOCK_RAW, 0), len, tick;
    static int seq = 100;

    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = type;
    h->rtm_flags = RTF_HOST | (llinfo ? RTF_LLINFO : 0) | (type == RTM_ADD ? RTF_STATIC : 0);
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = ++seq;
    h->rtm_index = i >= 0 ? ifindex[i] : 0;
    sa6(dst, addr, 0, 0);
    len = sizeof(*h) + ROUNDUP(sizeof(*dst));
    if (lladdr) {
        struct sockaddr_dl *sdl = (struct sockaddr_dl *)((char *)buf + len);

        bzero(sdl, sizeof(*sdl));
        sdl->sdl_family = AF_LINK;
        sdl->sdl_index = ifindex[i];
        sdl->sdl_alen = 6;
        memcpy(LLADDR(sdl), lladdr, 6);
        sdl->sdl_len = (sdl->sdl_data - (char *)sdl) + 6;
        h->rtm_addrs |= RTA_GATEWAY;
        len += ROUNDUP(sdl->sdl_len);
    }
    h->rtm_msglen = len;
    if (send(s, (char *)buf, len, 0) < 0) {
        int e = Errno();

        CloseSocket(s);
        return e;
    }
    for (tick = 0; tick < 20; tick++) {     /* our own message comes back */
        int n;

        if (sock_wait(s, 0, 0) <= 0) {
            Delay(5);
            continue;
        }
        n = recv(s, (char *)buf, sizeof(buf), 0);
        if (n < (int)sizeof(*h) || h->rtm_seq != seq || h->rtm_pid != (pid_t)FindTask(NULL))
            continue;
        if (flags)
            *flags = h->rtm_flags;
        if (got && h->rtm_errno == 0 && (h->rtm_addrs & (RTA_DST | RTA_GATEWAY)) == (RTA_DST | RTA_GATEWAY)) {
            struct sockaddr *d = (struct sockaddr *)(h + 1);
            struct sockaddr_dl *g = (struct sockaddr_dl *)((char *)d + ROUNDUP(d->sa_len));

            memset(got, 0, 6);
            if (g->sdl_family == AF_LINK && g->sdl_alen == 6)
                memcpy(got, LLADDR(g), 6);
        }
        CloseSocket(s);
        return h->rtm_errno;
    }
    CloseSocket(s);
    return -1;
}

/*
 * The neighbour cache rows GetRouteInfo() lists for addr: how many, and for
 * each link i, whether there is one on it and its link-layer address.
 */
static int listed(const struct in6_addr *addr, int on[2], unsigned char got[2][6], int *incomplete)
{
    struct rt_msghdr *buf = GetRouteInfo(AF_INET6, RTF_LLINFO), *h;
    char *p;
    int n = 0, i;

    for (i = 0; i < 2; i++)
        if (on)
            on[i] = 0;
    if (incomplete)
        *incomplete = 0;
    if (buf == NULL)
        return 0;
    for (p = (char *)buf; (h = (struct rt_msghdr *)p)->rtm_msglen != 0; p += h->rtm_msglen) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)(h + 1);
        struct sockaddr_dl *g = (struct sockaddr_dl *)((char *)d + ROUNDUP(d->sin6_len));

        if (!(h->rtm_flags & RTF_LLINFO) || d->sin6_family != AF_INET6 ||
                memcmp(&d->sin6_addr, addr, sizeof(*addr)) != 0)
            continue;
        n++;
        if (g->sdl_alen == 0 && incomplete)
            (*incomplete)++;
        for (i = 0; i < 2; i++)
            if (h->rtm_index == ifindex[i] && on) {
                on[i] = 1;
                if (got && g->sdl_family == AF_LINK && g->sdl_alen == 6)
                    memcpy(got[i], LLADDR(g), 6);
            }
    }
    FreeRouteInfo(buf);
    return n;
}

/* How many rows GetRouteInfo() lists on link i, and how many of them are
 * NOBODY addresses still unresolved. */
static int rows_on(int i, int *nobodies)
{
    struct rt_msghdr *buf = GetRouteInfo(AF_INET6, RTF_LLINFO), *h;
    struct in6_addr nb;
    char *p;
    int n = 0;

    *nobodies = 0;
    if (buf == NULL)
        return 0;
    nobody(&nb, 0);
    for (p = (char *)buf; (h = (struct rt_msghdr *)p)->rtm_msglen != 0; p += h->rtm_msglen) {
        const struct sockaddr_in6 *d = (const struct sockaddr_in6 *)(h + 1);

        if (h->rtm_index != ifindex[i])
            continue;
        n++;
        if (memcmp(&d->sin6_addr, &nb, 14) == 0)
            (*nobodies)++;
    }
    FreeRouteInfo(buf);
    return n;
}

static long bpf_on(const char *name)
{
    u_int one = 1;
    char buf[BPF_DFLTBUFSIZE];
    long b = bpf_open(0);

    if (b < 0)
        return -1;
    if (bpf_ioctl(b, BIOCSETIF, (char *)name) != 0 ||
            bpf_ioctl(b, BIOCIMMEDIATE, (char *)&one) != 0) {
        bpf_close(b);
        return -1;
    }
    while (bpf_read(b, buf, sizeof(buf)) > 0)
        ;
    return b;
}

/*
 * What left link i: the Neighbor Solicitations for addr, and the UDP
 * datagrams to addr - how many in all, and how many went to the
 * link-layer address want.
 */
static void watch(int i, const struct in6_addr *addr, const unsigned char *want,
                  int *ns, int *udp, int *to_want)
{
    char buf[BPF_DFLTBUFSIZE];
    long got;

    *ns = *udp = *to_want = 0;
    while ((got = bpf_read(bpf[i], buf, sizeof(buf))) > 0) {
        char *p = buf;

        while (p < buf + got) {
            const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
            const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen;

            if (bh->bh_caplen >= 14 + 40 + 8 && fr[12] == 0x86 && fr[13] == 0xdd) {
                const unsigned char *ip = fr + 14;

                if (ip[6] == IPPROTO_ICMPV6 && bh->bh_caplen >= 14 + 40 + 24 &&
                        ip[40] == ND_NEIGHBOR_SOLICIT && memcmp(ip + 48, addr, 16) == 0)
                    (*ns)++;
                if (ip[6] == IPPROTO_UDP && memcmp(ip + 24, addr, 16) == 0) {
                    (*udp)++;
                    if (memcmp(fr, want, 6) == 0)
                        (*to_want)++;
                }
            }
            p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
        }
    }
}

#define NEED_IFS() do { if (!have_ifs) { CU_PASS("no two suitable interfaces - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The same address set by hand on two links is two entries, each with
 * its own link-layer address, read back one by one and in the listing. */
void testND6CACHE_TWO_LINKS(void)
{
    struct in6_addr twin;
    unsigned char got[2][6];
    int on[2], i, flags = 0;
    NEED_IFS();
    inet_pton(AF_INET6, (char *)TWIN, &twin);
    for (i = 0; i < 2; i++)
        CU_ASSERT_EQUAL(rtmsg(RTM_ADD, &twin, i, mac[i], 1, NULL, NULL), 0);
    for (i = 0; i < 2; i++) {
        memset(got[i], 0, 6);
        CU_ASSERT_EQUAL(rtmsg(RTM_GET, &twin, i, NULL, 1, got[i], &flags), 0);
        CU_ASSERT(memcmp(got[i], mac[i], 6) == 0);
        CU_ASSERT((flags & (RTF_LLINFO | RTF_STATIC)) == (RTF_LLINFO | RTF_STATIC));
    }
    memset(got, 0, sizeof(got));
    CU_ASSERT_EQUAL(listed(&twin, on, got, NULL), 2);
    CU_ASSERT(on[0] && on[1]);
    CU_ASSERT(memcmp(got[0], mac[0], 6) == 0);
    CU_ASSERT(memcmp(got[1], mac[1], 6) == 0);
}

/* Datagrams to the address on each link by turns: each leaves by its own
 * link, to that link's neighbour, and none is resolved again. */
void testND6CACHE_ALTERNATE(void)
{
    struct in6_addr twin;
    struct sockaddr_in6 d;
    int s, i, k, ns, udp, to;
    NEED_IFS();
    inet_pton(AF_INET6, (char *)TWIN, &twin);
    for (i = 0; i < 2; i++)
        watch(i, &twin, mac[i], &ns, &udp, &to);    /* drain */
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    for (k = 0; k < 8; k++) {
        sa6(&d, &twin, ifindex[k & 1], 9);
        CU_ASSERT_EQUAL(sendto(s, "twin", 4, 0, (struct sockaddr *)&d, sizeof(d)), 4);
    }
    CloseSocket(s);
    Delay(25);
    for (i = 0; i < 2; i++) {
        watch(i, &twin, mac[i], &ns, &udp, &to);
        CU_ASSERT_EQUAL(ns, 0);
        CU_ASSERT_EQUAL(udp, 4);
        CU_ASSERT_EQUAL(to, 4);
    }
}

/* A message without RTF_LLINFO is about routes: the address's route is the
 * link's prefix, not a neighbour. */
void testND6CACHE_ROUTE_KEPT(void)
{
    struct in6_addr twin;
    int flags = 0;
    NEED_IFS();
    inet_pton(AF_INET6, (char *)TWIN, &twin);
    CU_ASSERT_EQUAL(rtmsg(RTM_GET, &twin, -1, NULL, 0, NULL, &flags), 0);
    CU_ASSERT((flags & RTF_UP) != 0);
    CU_ASSERT((flags & (RTF_LLINFO | RTF_HOST)) == 0);
}

/* Deleting the entry on one link leaves the other's. */
void testND6CACHE_DELETE_ONE(void)
{
    struct in6_addr twin;
    unsigned char got[6];
    int on[2];
    NEED_IFS();
    inet_pton(AF_INET6, (char *)TWIN, &twin);
    CU_ASSERT_EQUAL(rtmsg(RTM_DELETE, &twin, 0, NULL, 1, NULL, NULL), 0);
    CU_ASSERT_EQUAL(rtmsg(RTM_GET, &twin, 0, NULL, 1, NULL, NULL), ESRCH);
    memset(got, 0, 6);
    CU_ASSERT_EQUAL(rtmsg(RTM_GET, &twin, 1, NULL, 1, got, NULL), 0);
    CU_ASSERT(memcmp(got, mac[1], 6) == 0);
    CU_ASSERT_EQUAL(listed(&twin, on, NULL, NULL), 1);
    CU_ASSERT(!on[0] && on[1]);
    CU_ASSERT_EQUAL(rtmsg(RTM_DELETE, &twin, 1, NULL, 1, NULL, NULL), 0);
    CU_ASSERT_EQUAL(listed(&twin, NULL, NULL, NULL), 0);
    CU_ASSERT_EQUAL(rtmsg(RTM_DELETE, &twin, 1, NULL, 1, NULL, NULL), ESRCH);
}

/*
 * A datagram each to more addresses than the cache holds, nobody's: the
 * cache fills with entries waiting to be resolved, none of them given up,
 * and the datagrams past it fail with ENOBUFS. Once resolution has failed
 * the entries are gone, and the cache takes new ones again.
 */
void testND6CACHE_FULL(void)
{
    struct in6_addr a;
    struct sockaddr_in6 d;
    int s, n, ok = 0, nobufs = 0, other = 0, before, waiting, rows;
    NEED_IFS();
    before = rows_on(1, &waiting);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    for (n = 1; n <= FLOOD; n++) {
        nobody(&a, n);
        sa6(&d, &a, ifindex[1], 9);
        if (sendto(s, "full", 4, 0, (struct sockaddr *)&d, sizeof(d)) == 4)
            ok++;
        else if (Errno() == ENOBUFS)
            nobufs++;
        else
            other++;
    }
    rows = rows_on(1, &waiting);
    printf("cache full: %d entries before, %d sent, %d ENOBUFS, %d other errors, %d rows (%d waiting)\n",
           before, ok, nobufs, other, rows, waiting);
    CU_ASSERT_EQUAL(other, 0);
    CU_ASSERT(nobufs > 0);
    CU_ASSERT_EQUAL(ok + nobufs, FLOOD);
    CU_ASSERT(rows <= MAXNBRS);
    CU_ASSERT_EQUAL(waiting, ok);               /* none given up */
    Delay(5 * 50);                              /* 3 solicitations, 1 s apart */
    rows_on(1, &waiting);
    CU_ASSERT_EQUAL(waiting, 0);
    nobody(&a, 1);
    sa6(&d, &a, ifindex[1], 9);
    CU_ASSERT_EQUAL(sendto(s, "full", 4, 0, (struct sockaddr *)&d, sizeof(d)), 4);
    CloseSocket(s);
}

static int setup(void)
{
    struct ifreq ifr;
    int s, i, ok = 1;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    for (i = 0; i < 2 && ok; i++) {
        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname[i]);
        ok = IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
             (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
             IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0;
        if (ok)
            ifindex[i] = ifr.ifr_index;
        if (ok && (bpf[i] = bpf_on(ifname[i])) < 0)
            ok = 0;
    }
    CloseSocket(s);
    return ok;
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int i;

    if (argc > 1)
        ifname[0] = argv[1];
    if (argc > 2)
        ifname[1] = argv[2];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ND6CACHE tests\n");
        return 0;
    }
    have_ifs = setup();
    if (!have_ifs)
        printf("%s, %s: not two multicast interfaces - skipping ND6CACHE tests\n", ifname[0], ifname[1]);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ND6CACHE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "one address on two links is two neighbours", testND6CACHE_TWO_LINKS) ||
        !CU_add_test(p, "sending to them by turns resolves neither again", testND6CACHE_ALTERNATE) ||
        !CU_add_test(p, "a message without RTF_LLINFO is about routes", testND6CACHE_ROUTE_KEPT) ||
        !CU_add_test(p, "deleting one leaves the other", testND6CACHE_DELETE_ONE) ||
        !CU_add_test(p, "a full cache keeps the entries waiting", testND6CACHE_FULL)) {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_automated_run_tests();
    CU_cleanup_registry();
    for (i = 0; i < 2; i++)
        if (bpf[i] >= 0)
            bpf_close(bpf[i]);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
