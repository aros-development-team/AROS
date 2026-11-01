/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv4 path MTU discovery (RFC 1191): TCP sends with DF
    set; an ICMP "fragmentation needed" puts the MTU it reports on a host
    route of the destination's own - not on the router's route, which other
    destinations share - kept for a while, then given back; an MTU of 0
    (an older router) gives the next plateau below the quoted datagram's
    length (section 7); and it is never below 552.

    A route to 10.77.42.0/24 through a router on the interface's subnet is
    added with a routing socket; the ICMP messages, quoting datagrams to
    destinations behind it, are sent to 127.0.0.1 from a raw ICMP socket;
    the routes are read back with RTM_GET. DF is read from a SYN to the
    neighbour with 1 as its last octet (on hosted AROS, the host side of the
    tap), with the bsdsocket BPF calls. Usage:  cunit-net-pmtu4 [IFNAME]
    (default eth0). Without an IPv4 interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/route.h>
#include <net/bpf.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static struct in_addr ours, gw, neighbour;
static int ready;
static long bpf = -1;

#define NET     "10.77.42.0"
#define DST_A   "10.77.42.1"            /* the message is about this one */
#define DST_B   "10.77.42.2"            /* and not about this one */
#define DST_C   "10.77.42.3"
#define DST_D   "10.77.42.4"
#define EXPIRE  600

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

static void sa4(struct sockaddr_in *a, struct in_addr addr)
{
    bzero(a, sizeof(*a));
    a->sin_len = sizeof(*a);
    a->sin_family = AF_INET;
    a->sin_addr = addr;
}

static struct in_addr addr4(const char *s)
{
    struct in_addr a;

    inet_pton(AF_INET, (char *)s, &a);
    return a;
}

/* Add or delete NET/24 (or a host route) through gw. */
static int route(int type, const char *dst, int host)
{
    long buf[256 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    struct sockaddr_in *sa;
    char *p;
    int s = socket(PF_ROUTE, SOCK_RAW, 0), r;

    if (s < 0)
        return -1;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = type;
    h->rtm_flags = RTF_UP | RTF_GATEWAY | (host ? RTF_HOST : RTF_STATIC);
    h->rtm_addrs = RTA_DST | RTA_GATEWAY | (host ? 0 : RTA_NETMASK);
    h->rtm_seq = 1;
    p = (char *)(h + 1);
    sa = (struct sockaddr_in *)p; sa4(sa, addr4(dst)); p += ROUNDUP(sizeof(*sa));
    sa = (struct sockaddr_in *)p; sa4(sa, gw);         p += ROUNDUP(sizeof(*sa));
    if (!host) {
        sa = (struct sockaddr_in *)p; sa4(sa, addr4("255.255.255.0"));
        p += ROUNDUP(sizeof(*sa));
    }
    h->rtm_msglen = p - (char *)buf;
    r = send(s, (char *)buf, h->rtm_msglen, 0);
    CloseSocket(s);
    return r < 0 ? -1 : 0;
}

/* The route the table uses for dst: its flags and metrics. */
static int route_get(const char *dst, int *flags, struct rt_metrics *rmx)
{
    long buf[512 / sizeof(long)];
    struct rt_msghdr *h = (struct rt_msghdr *)buf;
    int s = socket(PF_ROUTE, SOCK_RAW, 0), n = 0;

    if (s < 0)
        return 0;
    bzero(buf, sizeof(buf));
    h->rtm_version = RTM_VERSION;
    h->rtm_type = RTM_GET;
    h->rtm_addrs = RTA_DST;
    h->rtm_seq = 2;
    sa4((struct sockaddr_in *)(h + 1), addr4(dst));
    h->rtm_msglen = sizeof(*h) + ROUNDUP(sizeof(struct sockaddr_in));
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
    return 1;
}

static unsigned short cksum(const unsigned char *p, int len)
{
    unsigned long sum = 0;

    for (; len > 1; p += 2, len -= 2)
        sum += (p[0] << 8) | p[1];
    if (len)
        sum += p[0] << 8;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (unsigned short)~sum;
}

/* A "fragmentation needed" reporting mtu, about a 1500-byte UDP datagram
 * from us to dst, sent to 127.0.0.1. */
static int send_needfrag(const char *dst, unsigned mtu)
{
    unsigned char pkt[8 + 20 + 8];
    unsigned char *q = pkt + 8;
    struct in_addr d = addr4(dst);
    struct sockaddr_in to;
    unsigned short c;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP), r;

    if (s < 0)
        return -1;
    bzero(pkt, sizeof(pkt));
    pkt[0] = ICMP_UNREACH;
    pkt[1] = ICMP_UNREACH_NEEDFRAG;
    pkt[6] = mtu >> 8;
    pkt[7] = mtu & 0xff;
    q[0] = 0x45;                        /* the quoted IPv4 header */
    q[2] = 1500 >> 8; q[3] = 1500 & 0xff;
    q[6] = 0x40;                        /* DF */
    q[8] = 64;
    q[9] = IPPROTO_UDP;
    memcpy(q + 12, &ours, 4);
    memcpy(q + 16, &d, 4);
    c = cksum(q, 20);
    q[10] = c >> 8; q[11] = c & 0xff;
    c = cksum(pkt, sizeof(pkt));
    pkt[2] = c >> 8; pkt[3] = c & 0xff;
    sa4(&to, addr4("127.0.0.1"));
    r = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&to, sizeof(to));
    CloseSocket(s);
    Delay(10);
    return r;
}

/* Seconds, on the clock the stack keeps route lifetimes in. */
static unsigned long now(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return ds.ds_Days * 86400UL + ds.ds_Minute * 60UL + ds.ds_Tick / TICKS_PER_SECOND;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void)
{
    if (SocketBase == NULL)
        return -1;
    if (ready && route(RTM_ADD, NET, 0) != 0)
        ready = 0;
    return 0;
}

int clean_suite(void)
{
    if (ready) {
        route(RTM_DELETE, DST_A, 1);
        route(RTM_DELETE, DST_C, 1);
        route(RTM_DELETE, DST_D, 1);
        route(RTM_DELETE, NET, 0);
    }
    return 0;
}

/* The MTU goes on a host route of the destination's own, for a while. */
void testPMTU4_OWN_ROUTE(void)
{
    struct rt_metrics a, b;
    unsigned long t = now();
    int fa = 0, fb = 0;
    NEED_IF();
    CU_ASSERT(send_needfrag(DST_A, 1400) > 0);
    CU_ASSERT_FATAL(route_get(DST_A, &fa, &a));
    CU_ASSERT(fa & RTF_HOST);
    CU_ASSERT_EQUAL(a.rmx_mtu, 1400);
    CU_ASSERT(a.rmx_expire >= t + EXPIRE - 10 && a.rmx_expire <= t + EXPIRE + 10);
    CU_ASSERT_FATAL(route_get(DST_B, &fb, &b));
    CU_ASSERT(!(fb & RTF_HOST));
    CU_ASSERT(b.rmx_mtu != 1400);
}

/* No MTU in the message: the next plateau below 1500, 1492 (RFC 1191). */
void testPMTU4_PLATEAU(void)
{
    struct rt_metrics c;
    int fc = 0;
    NEED_IF();
    CU_ASSERT(send_needfrag(DST_C, 0) > 0);
    CU_ASSERT_FATAL(route_get(DST_C, &fc, &c));
    CU_ASSERT_EQUAL(c.rmx_mtu, 1492);
}

/* Never below 552. */
void testPMTU4_MINIMUM(void)
{
    struct rt_metrics d;
    int fd = 0;
    NEED_IF();
    CU_ASSERT(send_needfrag(DST_D, 200) > 0);
    CU_ASSERT_FATAL(route_get(DST_D, &fd, &d));
    CU_ASSERT_EQUAL(d.rmx_mtu, 552);
}

/* TCP segments are sent with DF. */
void testPMTU4_TCP_DF(void)
{
    char buf[BPF_DFLTBUFSIZE];
    struct sockaddr_in d;
    long on = 1;
    int s, t, seen = 0, df = 0;
    NEED_IF();
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    s = socket(AF_INET, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    IoctlSocket(s, FIONBIO, (char *)&on);
    sa4(&d, neighbour);
    d.sin_port = htons(9);
    connect(s, (struct sockaddr *)&d, sizeof(d));
    for (t = 0; t < 30 && !seen; t++) {
        long got;

        while (!seen && (got = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;

            while (!seen && p < buf + got) {
                const struct bpf_hdr *bh = (const struct bpf_hdr *)p;
                const unsigned char *fr = (unsigned char *)p + bh->bh_hdrlen;

                if (bh->bh_caplen >= 14 + 20 + 14 && fr[12] == 0x08 && fr[13] == 0x00 &&
                        fr[14 + 9] == IPPROTO_TCP && memcmp(fr + 14 + 16, &neighbour, 4) == 0 &&
                        (fr[14 + 20 + 13] & 0x02)) {          /* our SYN */
                    seen = 1;
                    df = (fr[14 + 6] & 0x40) != 0;
                }
                p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
            }
        }
        Delay(5);
    }
    CloseSocket(s);
    if (!seen) {
        CU_PASS("no SYN seen (the neighbour did not answer ARP) - skipping");
        return;
    }
    CU_ASSERT(df);
}

static int setup(void)
{
    struct ifreq ifr;
    u_int one = 1;
    int s = socket(AF_INET, SOCK_DGRAM, 0), ok;
    char drain[BPF_DFLTBUFSIZE];

    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ok = IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0 &&
         ((struct sockaddr_in *)&ifr.ifr_addr)->sin_family == AF_INET;
    if (ok)
        ours = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    CloseSocket(s);
    if (!ok || ours.s_addr == 0)
        return 0;
    gw = neighbour = ours;
    ((unsigned char *)&gw)[3] = 77;
    ((unsigned char *)&neighbour)[3] = 1;
    if (gw.s_addr == ours.s_addr || neighbour.s_addr == ours.s_addr)
        return 0;
    bpf = bpf_open(0);
    if (bpf < 0)
        return 0;
    if (bpf_ioctl(bpf, BIOCSETIF, (char *)ifname) != 0 ||
            bpf_ioctl(bpf, BIOCIMMEDIATE, (char *)&one) != 0)
        return 0;
    while (bpf_read(bpf, drain, sizeof(drain)) > 0)
        ;
    return 1;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping PMTU4 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no IPv4 interface - skipping PMTU4 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PMTU4_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the path MTU goes on a route of the destination's own", testPMTU4_OWN_ROUTE) ||
        !CU_add_test(p, "without an MTU, the next plateau is taken", testPMTU4_PLATEAU) ||
        !CU_add_test(p, "the path MTU is never below 552", testPMTU4_MINIMUM) ||
        !CU_add_test(p, "TCP segments are sent with DF", testPMTU4_TCP_DF))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-PMTU4");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
