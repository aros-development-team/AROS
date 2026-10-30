/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for raw IPv6 sockets (RFC 3542 section 3): the IPv6 socket
    options apply to them, they receive every ICMPv6 message the ICMP6_FILTER
    passes (also those the kernel answers itself, like Echo Request), data
    arrives without the IPv6 header, and the hop limit on the wire is the
    one asked for: IPV6_UNICAST_HOPS, IPV6_MULTICAST_HOPS (default 1), or
    IPV6_HOPLIMIT ancillary data.

    Received packets are sent to ::1. Hop limits on the wire are watched
    with the bsdsocket BPF calls on Echo Requests to ff02::1, which needs a
    multicast-capable interface (on hosted AROS, tap.device attached to a
    host tap); without one those tests are skipped. Usage:  cunit-net-raw6
    [IFNAME]  (default eth0).
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static int ready;
static long bpf = -1;

#define ECHO_ID         0x4d54
#define PROTO_EXP       253     /* RFC 3692 experimental protocol */

static void sa6(struct sockaddr_in6 *a, const char *addr)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
}

static int icmp6_socket(void)
{
    return socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
}

/* An Echo Request with id ECHO_ID and seq, to dst (zone scope). */
static int send_echo(int s, const char *dst, unsigned scope, int seq)
{
    struct sockaddr_in6 d;
    struct icmp6_hdr h;

    bzero(&h, sizeof(h));
    h.icmp6_type = ICMP6_ECHO_REQUEST;
    h.icmp6_id = htons(ECHO_ID);
    h.icmp6_seq = htons(seq);
    sa6(&d, dst);
    d.sin6_scope_id = scope;
    return sendto(s, (char *)&h, sizeof(h), 0, (struct sockaddr *)&d, sizeof(d));
}

/*
 * Wait up to secs for an ICMPv6 message of type with our id and seq. Returns
 * its length (0 if none came); *hlim gets the IPV6_HOPLIMIT ancillary data,
 * or -1 without it.
 */
static int recv_echo(int s, int type, int seq, int secs, int *hlim)
{
    unsigned char buf[256];
    union { struct cmsghdr c; char b[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct in6_pktinfo))]; } ctl;
    struct sockaddr_in6 from;
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    int n;

    while (sock_wait(s, 0, secs) > 0) {
        bzero(&mh, sizeof(mh));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);
        mh.msg_name = &from;
        mh.msg_namelen = sizeof(from);
        mh.msg_iov = &iov;
        mh.msg_iovlen = 1;
        mh.msg_control = &ctl;
        mh.msg_controllen = sizeof(ctl);
        n = recvmsg(s, &mh, 0);
        if (n < (int)sizeof(struct icmp6_hdr))
            continue;
        if (buf[0] != type || ((buf[4] << 8) | buf[5]) != ECHO_ID ||
                ((buf[6] << 8) | buf[7]) != seq)
            continue;
        if (hlim) {
            *hlim = -1;
            for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c))
                if (c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_HOPLIMIT)
                    memcpy(hlim, CMSG_DATA(c), sizeof(int));
        }
        return n;
    }
    return 0;
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

/* The hop limit of our Echo Request seq to ff02::1 as it left, or -1. */
static int wire_hlim(int seq, int secs)
{
    struct in6_addr all;
    char buf[BPF_DFLTBUFSIZE];
    int ticks;

    inet_pton(AF_INET6, "ff02::1", &all);
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long n;
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                const u_char *f = (u_char *)p + h->bh_hdrlen, *ip = NULL, *q;
                u_int len = h->bh_caplen;

                if (len >= 14 + 48 && f[12] == 0x86 && f[13] == 0xdd)
                    ip = f + 14;
                else if (len >= 48 && (f[0] >> 4) == 6)
                    ip = f;
                if (ip && ip[6] == IPPROTO_ICMPV6 && memcmp(ip + 24, &all, 16) == 0) {
                    q = ip + 40;
                    if (q[0] == ICMP6_ECHO_REQUEST && ((q[4] << 8) | q[5]) == ECHO_ID &&
                            ((q[6] << 8) | q[7]) == seq)
                        return ip[7];
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
    return -1;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* IPV6_UNICAST_HOPS is kept, and getsockopt() returns it. */
void testRAW6_UNICAST_HOPS(void)
{
    int s = icmp6_socket(), v = 77;
    socklen_t len = sizeof(v);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &v, sizeof(v)), 0);
    v = -2;
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &v, &len), 0);
    CU_ASSERT_EQUAL(len, sizeof(int));
    CU_ASSERT_EQUAL(v, 77);
    CloseSocket(s);
}

/* An ICMPv6 socket sees the Echo Request the kernel answers, and the Reply. */
void testRAW6_ECHO_REQUEST_AND_REPLY(void)
{
    int s = icmp6_socket();

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(send_echo(s, "::1", 0, 1), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(recv_echo(s, ICMP6_ECHO_REQUEST, 1, 2, NULL), sizeof(struct icmp6_hdr));
    CloseSocket(s);
    s = icmp6_socket();
    CU_ASSERT_EQUAL(send_echo(s, "::1", 0, 2), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(recv_echo(s, ICMP6_ECHO_REPLY, 2, 2, NULL), sizeof(struct icmp6_hdr));
    CloseSocket(s);
}

/* ICMP6_FILTER: passing only Echo Request keeps the Reply away. */
void testRAW6_FILTER(void)
{
    struct icmp6_filter f, g;
    socklen_t len = sizeof(g);
    int s = icmp6_socket();

    CU_ASSERT_FATAL(s >= 0);
    ICMP6_FILTER_SETBLOCKALL(&f);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REQUEST, &f);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &f, sizeof(f)), 0);
    bzero(&g, sizeof(g));
    CU_ASSERT_EQUAL(getsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &g, &len), 0);
    CU_ASSERT_EQUAL(len, sizeof(g));
    CU_ASSERT(memcmp(&f, &g, sizeof(f)) == 0);
    CU_ASSERT_EQUAL(send_echo(s, "::1", 0, 3), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(recv_echo(s, ICMP6_ECHO_REPLY, 3, 2, NULL), 0);
    CloseSocket(s);
}

/* A raw socket of another protocol gets the data, not the IPv6 header. */
void testRAW6_NO_IPV6_HEADER(void)
{
    struct sockaddr_in6 d, from;
    socklen_t flen = sizeof(from);
    char buf[128];
    int s = socket(AF_INET6, SOCK_RAW, PROTO_EXP), n;

    CU_ASSERT_FATAL(s >= 0);
    sa6(&d, "::1");
    CU_ASSERT_EQUAL(sendto(s, "RAW6", 4, 0, (struct sockaddr *)&d, sizeof(d)), 4);
    CU_ASSERT(sock_wait(s, 0, 2) > 0);
    n = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, &flen);
    CU_ASSERT_EQUAL(n, 4);
    CU_ASSERT(n >= 4 && memcmp(buf, "RAW6", 4) == 0);
    CU_ASSERT(memcmp(&from.sin6_addr, &d.sin6_addr, sizeof(d.sin6_addr)) == 0);
    CloseSocket(s);
}

/* IPV6_RECVHOPLIMIT gives the hop limit the packet came with. */
void testRAW6_RECVHOPLIMIT(void)
{
    int s = icmp6_socket(), on = 1, v = 77, hlim = 0;

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &v, sizeof(v)), 0);
    CU_ASSERT_EQUAL(send_echo(s, "::1", 0, 4), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(recv_echo(s, ICMP6_ECHO_REQUEST, 4, 2, &hlim), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(hlim, 77);
    CloseSocket(s);
}

/* IPV6_MULTICAST_HOPS and IPV6_MULTICAST_IF apply on the wire. */
void testRAW6_MULTICAST_HOPS(void)
{
    int s, v = 255;
    NEED_IF();
    s = icmp6_socket();
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &v, sizeof(v)), 0);
    CU_ASSERT_EQUAL(send_echo(s, "ff02::1", 0, 5), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(wire_hlim(5, 2), 255);
    CloseSocket(s);
}

/* Without IPV6_MULTICAST_HOPS a multicast datagram leaves with hop limit 1. */
void testRAW6_MULTICAST_DEFAULT_HOPS(void)
{
    int s;
    NEED_IF();
    s = icmp6_socket();
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(send_echo(s, "ff02::1", ifindex, 6), sizeof(struct icmp6_hdr));
    CU_ASSERT_EQUAL(wire_hlim(6, 2), 1);
    CloseSocket(s);
}

/* IPV6_HOPLIMIT and IPV6_PKTINFO as ancillary data on sendmsg(). */
void testRAW6_HOPLIMIT_CMSG(void)
{
    union { struct cmsghdr c; char b[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct in6_pktinfo))]; } ctl;
    struct in6_pktinfo pi;
    struct sockaddr_in6 d;
    struct icmp6_hdr h;
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    int s, v = 255;
    NEED_IF();
    s = icmp6_socket();
    CU_ASSERT_FATAL(s >= 0);

    bzero(&h, sizeof(h));
    h.icmp6_type = ICMP6_ECHO_REQUEST;
    h.icmp6_id = htons(ECHO_ID);
    h.icmp6_seq = htons(7);
    sa6(&d, "ff02::1");
    iov.iov_base = &h;
    iov.iov_len = sizeof(h);
    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    mh.msg_name = &d;
    mh.msg_namelen = sizeof(d);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = &ctl;
    mh.msg_controllen = sizeof(ctl);
    c = CMSG_FIRSTHDR(&mh);
    c->cmsg_level = IPPROTO_IPV6;
    c->cmsg_type = IPV6_HOPLIMIT;
    c->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c), &v, sizeof(v));
    c = CMSG_NXTHDR(&mh, c);
    c->cmsg_level = IPPROTO_IPV6;
    c->cmsg_type = IPV6_PKTINFO;
    c->cmsg_len = CMSG_LEN(sizeof(pi));
    bzero(&pi, sizeof(pi));
    pi.ipi6_ifindex = ifindex;
    memcpy(CMSG_DATA(c), &pi, sizeof(pi));

    CU_ASSERT_EQUAL(sendmsg(s, &mh, 0), sizeof(h));
    CU_ASSERT_EQUAL(wire_hlim(7, 2), 255);
    CloseSocket(s);
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
    CloseSocket(s);
    if (!ok)
        return 0;
    ifindex = ifr.ifr_index;
    return bpf_start() == 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping RAW6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no multicast interface - skipping the RAW6 tests on the wire\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("RAW6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "IPV6_UNICAST_HOPS is kept", testRAW6_UNICAST_HOPS) ||
        !CU_add_test(p, "an ICMPv6 socket sees Echo Request and Reply", testRAW6_ECHO_REQUEST_AND_REPLY) ||
        !CU_add_test(p, "ICMP6_FILTER keeps blocked types away", testRAW6_FILTER) ||
        !CU_add_test(p, "data arrives without the IPv6 header", testRAW6_NO_IPV6_HEADER) ||
        !CU_add_test(p, "IPV6_RECVHOPLIMIT gives the hop limit", testRAW6_RECVHOPLIMIT) ||
        !CU_add_test(p, "IPV6_MULTICAST_HOPS applies on the wire", testRAW6_MULTICAST_HOPS) ||
        !CU_add_test(p, "multicast hop limit defaults to 1", testRAW6_MULTICAST_DEFAULT_HOPS) ||
        !CU_add_test(p, "IPV6_HOPLIMIT ancillary data applies on the wire", testRAW6_HOPLIMIT_CMSG))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-RAW6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
