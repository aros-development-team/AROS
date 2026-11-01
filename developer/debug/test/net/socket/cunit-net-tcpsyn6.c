/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the TCP options of the SYN the stack sends over IPv6:
    the MSS, window scale, SACK-permitted and timestamp options it asks for
    by default (RFC 9293, RFC 7323, RFC 2018), and none of the T/TCP CC
    options (RFC 1644), which RFC 6247 moved to Historic.

    The test opens a connection to a neighbour on the link - found with an
    Echo Request to ff02::1 - and reads its own SYN with the bsdsocket BPF
    calls; whether the neighbour accepts the connection does not matter.
    It needs a multicast-capable interface with a neighbour that answers
    (on hosted AROS, tap.device attached to a host tap). Usage:
    cunit-net-tcpsyn6 [IFNAME]  (default eth0). Without one the tests are
    skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>
#include <sys/ioctl.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal, peer;
static int ready;
static long bpf = -1;

#define ECHO_ID         0x4d55
#define PORT            9       /* discard; a closed port does as well */

/* TCP option kinds */
#define OPT_MSS         2
#define OPT_WSCALE      3
#define OPT_SACKOK      4
#define OPT_TS          8
#define OPT_CC          11      /* RFC 1644: CC, CC.NEW, CC.ECHO */
#define OPT_CCECHO      13

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
        struct timeval tv = { 0, 100000 };
        fd_set r;

        FD_ZERO(&r);
        FD_SET(s, &r);
        if (WaitSelect(s + 1, &r, NULL, NULL, &tv, NULL) <= 0)
            continue;
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

/*
 * Start a connection to the peer and capture its SYN: the TCP options go to
 * opt, their length is returned (-1 if no SYN was seen). The wait covers a
 * retransmitted SYN, should the first one be lost to neighbour discovery.
 */
static int syn_options(unsigned char *opt, int max)
{
    struct sockaddr_in6 d, me;
    socklen_t mlen = sizeof(me);
    char buf[BPF_DFLTBUFSIZE];
    long one = 1;
    int s = socket(AF_INET6, SOCK_STREAM, 0), ticks, len = -1;

    if (s < 0)
        return -1;
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    /* the local port, known before the connection starts: the peer may
     * refuse it before connect() returns */
    sa6(&me, NULL, 0);
    if (bind(s, (struct sockaddr *)&me, sizeof(me)) != 0 ||
            getsockname(s, (struct sockaddr *)&me, &mlen) != 0) {
        CloseSocket(s);
        return -1;
    }
    IoctlSocket(s, FIONBIO, (char *)&one);
    sa6(&d, &peer, PORT);
    connect(s, (struct sockaddr *)&d, sizeof(d));
    for (ticks = 0; ticks < 50 && len < 0; ticks++) {
        long got;

        while (len < 0 && (got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (len < 0 && p < buf + got) {
                const struct bpf_hdr *h = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + h->bh_hdrlen, *ip = NULL, *th;
                unsigned caplen = h->bh_caplen;

                if (caplen >= 14 + 40 + 20 && fr[12] == 0x86 && fr[13] == 0xdd) {
                    ip = fr + 14;
                    caplen -= 14;
                } else if (caplen >= 40 + 20 && (fr[0] >> 4) == 6)
                    ip = fr;
                th = ip ? ip + 40 : NULL;
                if (ip && ip[6] == IPPROTO_TCP && memcmp(ip + 24, &peer, 16) == 0 &&
                        memcmp(th, &me.sin6_port, 2) == 0 && (th[13] & 0x12) == 0x02) {
                    int n = (th[12] >> 4) * 4 - 20;

                    if (n > max)
                        n = max;
                    if (n >= 0 && 40 + 20 + (unsigned)n <= caplen) {
                        memcpy(opt, th + 20, n);
                        len = n;
                    }
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        if (len < 0)
            Delay(5);
    }
    CloseSocket(s);
    return len;
}

/* Whether the options carry kind (low..high). */
static int has_option(const unsigned char *opt, int len, int low, int high)
{
    int i = 0;

    while (i < len) {
        if (opt[i] == 0)                /* end of option list */
            break;
        if (opt[i] == 1) {              /* no-operation */
            i++;
            continue;
        }
        if (opt[i] >= low && opt[i] <= high)
            return 1;
        if (i + 1 >= len || opt[i + 1] < 2)
            break;
        i += opt[i + 1];
    }
    return 0;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The SYN asks for MSS, window scale, SACK and timestamps. */
void testTCPSYN6_OPTIONS(void)
{
    unsigned char opt[40];
    int len;
    NEED_IF();
    len = syn_options(opt, sizeof(opt));
    CU_ASSERT_FATAL(len > 0);
    CU_ASSERT(has_option(opt, len, OPT_MSS, OPT_MSS));
    CU_ASSERT(has_option(opt, len, OPT_WSCALE, OPT_WSCALE));
    CU_ASSERT(has_option(opt, len, OPT_SACKOK, OPT_SACKOK));
    CU_ASSERT(has_option(opt, len, OPT_TS, OPT_TS));
}

/* The SYN carries no T/TCP CC option (RFC 6247). */
void testTCPSYN6_NO_TTCP(void)
{
    unsigned char opt[40];
    int len;
    NEED_IF();
    len = syn_options(opt, sizeof(opt));
    CU_ASSERT_FATAL(len >= 0);
    CU_ASSERT(!has_option(opt, len, OPT_CC, OPT_CCECHO));
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
    CloseSocket(s);
    return ok && find_linklocal() && find_peer() && bpf_start() == 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TCPSYN6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no multicast interface with a neighbour - skipping TCPSYN6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TCPSYN6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the SYN asks for MSS, window scale, SACK and timestamps", testTCPSYN6_OPTIONS) ||
        !CU_add_test(p, "the SYN carries no T/TCP CC option", testTCPSYN6_NO_TTCP))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TCPSYN6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
