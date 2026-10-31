/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the packets queued while a neighbour's link-layer
    address is resolved (RFC 4861 section 7.2.2): a datagram fragmented
    while its neighbour is resolved leaves as all of its fragments, not
    just the last; and when resolution fails, the socket that sent to the
    neighbour is told with an ICMPv6 Destination Unreachable, address
    unreachable.

    The first test needs a neighbour on the link (found with an Echo Request
    to ff02::1); it deletes the neighbour's entry with a routing socket, so
    that the next datagram has to be resolved again, and reads what goes out
    with the bsdsocket BPF calls. The second sends to a link-local address
    nobody has. Both need a multicast-capable interface (on hosted AROS,
    tap.device attached to a host tap). Usage:  cunit-net-ndqueue6 [IFNAME]
    (default eth0). Without one the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal, peer;
static int have_if, have_peer;
static long bpf = -1;

#define ECHO_ID         0x4d56
#define NOBODY          "fe80::4d54:dead"       /* no node on the link has it */
#define BIG             3000                    /* three fragments on a 1500-byte link */

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, int port)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_port = htons(port);
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_scope_id = ifindex;
}

static int find_linklocal(void)
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

        for (i = 0; i < n && !found; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                linklocal = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

/* A neighbour: the first link-local address other than ours to answer an
 * Echo Request to ff02::1. */
static int find_peer(void)
{
    struct sockaddr_in6 d;
    struct in6_addr all;
    struct icmp6_filter f;
    struct icmp6_hdr h;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6), tick, found = 0;

    if (s < 0)
        return 0;
    ICMP6_FILTER_SETBLOCKALL(&f);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &f);
    setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &f, sizeof(f));
    bzero(&h, sizeof(h));
    h.icmp6_type = ICMP6_ECHO_REQUEST;
    h.icmp6_id = htons(ECHO_ID);
    inet_pton(AF_INET6, "ff02::1", &all);
    sa6(&d, &all, 0);
    if (sendto(s, (char *)&h, sizeof(h), 0, (struct sockaddr *)&d, sizeof(d)) != sizeof(h)) {
        CloseSocket(s);
        return 0;
    }
    for (tick = 0; tick < 20 && !found; tick++) {
        unsigned char buf[128];
        struct sockaddr_in6 from;
        socklen_t flen = sizeof(from);

        if (sock_wait(s, 0, 0) <= 0) {
            struct timeval tv = { 0, 100000 };
            WaitSelect(0, NULL, NULL, NULL, &tv, NULL);
            continue;
        }
        if (recvfrom(s, (char *)buf, sizeof(buf), 0, (struct sockaddr *)&from, &flen) >= 8 &&
                buf[0] == ICMP6_ECHO_REPLY && ((buf[4] << 8) | buf[5]) == ECHO_ID &&
                IN6_IS_ADDR_LINKLOCAL(&from.sin6_addr) &&
                memcmp(&from.sin6_addr, &linklocal, sizeof(linklocal)) != 0) {
            peer = from.sin6_addr;
            found = 1;
        }
    }
    CloseSocket(s);
    return found;
}

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

/* Delete the neighbour cache entry (the host route) for addr. */
static int forget_neighbour(const struct in6_addr *addr)
{
    long buf[256 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in6 *sa = (struct sockaddr_in6 *)(h + 1);
    int s = socket(PF_ROUTE, SOCK_RAW, 0), r;

    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_DELETE;
    h->rtm_flags = RTF_HOST;
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = 1;
    sa6(sa, addr, 0);
    sa->sin6_scope_id = 0;
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(*sa));
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/*
 * What goes out on the link for secs: whether a Neighbor Solicitation for
 * addr, and the fragments of one datagram to addr - how many, and how many
 * bytes they carry together, the last one with M clear.
 */
static void watch(const struct in6_addr *addr, int secs, int *ns, int *frags, int *bytes, int *last)
{
    char buf[BPF_DFLTBUFSIZE];
    unsigned long id = 0;
    int ticks, have_id = 0;

    *ns = *frags = *bytes = *last = 0;
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long got;

        while ((got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (p < buf + got) {
                const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen, *ip = NULL;
                unsigned len = bh->bh_caplen;

                if (len >= 14 + 40 && fr[12] == 0x86 && fr[13] == 0xdd) {
                    ip = fr + 14;
                    len -= 14;
                } else if (len >= 40 && (fr[0] >> 4) == 6)
                    ip = fr;
                if (ip && ip[6] == IPPROTO_ICMPV6 && len >= 40 + 24 && ip[40] == ND_NEIGHBOR_SOLICIT &&
                        memcmp(ip + 48, addr, 16) == 0)
                    *ns = 1;
                if (ip && ip[6] == 44 && len >= 48 && memcmp(ip + 24, addr, 16) == 0) {
                    unsigned long fid = ((unsigned long)ip[44] << 24) | (ip[45] << 16) | (ip[46] << 8) | ip[47];
                    unsigned offlg = (ip[42] << 8) | ip[43];

                    if (!have_id) {
                        id = fid;
                        have_id = 1;
                    }
                    if (fid == id) {
                        (*frags)++;
                        *bytes += ((ip[4] << 8) | ip[5]) - 8;
                        if (!(offlg & 1))
                            *last = 1;
                    }
                }
                p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
            }
        }
        Delay(5);
    }
}

#define NEED_IF() do { if (!have_if) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A datagram fragmented while its neighbour is resolved leaves whole. */
void testND6_QUEUE_FRAGMENTS(void)
{
    static char payload[BIG];
    struct sockaddr_in6 d;
    int s, ns, frags, bytes, last;
    NEED_IF();
    if (!have_peer) {
        CU_PASS("no neighbour answers - skipping");
        return;
    }
    forget_neighbour(&peer);
    while (bpf_read(bpf, payload, sizeof(payload)) > 0)
        ;
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    memset(payload, 'q', sizeof(payload));
    sa6(&d, &peer, 9);
    CU_ASSERT_EQUAL(sendto(s, payload, BIG, 0, (struct sockaddr *)&d, sizeof(d)), BIG);
    watch(&peer, 3, &ns, &frags, &bytes, &last);
    CloseSocket(s);
    if (!ns) {
        CU_PASS("the neighbour entry could not be deleted - skipping");
        return;
    }
    CU_ASSERT_EQUAL(frags, 3);
    CU_ASSERT_EQUAL(bytes, 8 + BIG);            /* UDP header and payload */
    CU_ASSERT(last);
}

/* When resolution fails, the sender is told (address unreachable). */
void testND6_UNREACHABLE(void)
{
    struct in6_addr nobody;
    struct sockaddr_in6 d;
    long on = 1;
    char buf[8];
    int s, i, err = 0;
    NEED_IF();
    inet_pton(AF_INET6, (char *)NOBODY, &nobody);
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    sa6(&d, &nobody, 9);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&d, sizeof(d)), 0);
    IoctlSocket(s, FIONBIO, (char *)&on);
    CU_ASSERT_EQUAL(send(s, "probe", 5, 0), 5);
    for (i = 0; i < 80 && err == 0; i++) {      /* 3 solicitations, 1 s apart */
        struct timeval tv = { 0, 100000 };

        WaitSelect(0, NULL, NULL, NULL, &tv, NULL);
        if (recv(s, buf, sizeof(buf), 0) < 0 && Errno() != EWOULDBLOCK)
            err = Errno();
    }
    CloseSocket(s);
    CU_ASSERT(err == EHOSTUNREACH || err == EHOSTDOWN);
}

static int setup(void)
{
    struct ifreq ifr;
    int s, ok;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ok = IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
         (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
         IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0;
    if (ok)
        ifindex = ifr.ifr_index;
    if (ok && (IoctlSocket(s, SIOCGIFMTU, (char *)&ifr) != 0 || ifr.ifr_mtu >= 40 + 8 + BIG))
        ok = 0;
    CloseSocket(s);
    return ok && find_linklocal() && bpf_start() == 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping NDQUEUE6 tests\n");
        return 0;
    }
    have_if = setup();
    have_peer = have_if && find_peer();
    if (!have_if)
        printf("%s: no multicast interface with a link-local address - skipping NDQUEUE6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NDQUEUE6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a datagram fragmented while resolving leaves whole", testND6_QUEUE_FRAGMENTS) ||
        !CU_add_test(p, "a failed resolution tells the sender", testND6_UNREACHABLE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-NDQUEUE6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
