/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the ancillary data recvmsg() returns with a datagram over
    the loopback: IPV6_PKTINFO (RFC 3542 section 6.1), IPV6_HOPLIMIT
    (section 6.3), IPV6_TCLASS (section 6.5) and IPv4's IP_RECVDSTADDR and
    IP_RECVTOS - the traffic class and the TOS byte set by option or by
    ancillary data on the datagram (ECN for QUIC, RFC 9000 section 13.4) -
    and IP_RECVDSTADDR on a multicast datagram, looped back on IFNAME's
    address. Usage:  cunit-net-cmsg [IFNAME]  (default eth0; the multicast
    test is skipped without an IPv4 address on it). Each message must carry its whole
    data at CMSG_DATA() with cmsg_len = CMSG_LEN() - on LP64 the data starts
    past padding after the header - several messages must all arrive, walkable
    with CMSG_NXTHDR(), and a control buffer too small for them must be
    reported with MSG_CTRUNC.
*/

#include "socktest.h"

#include <net/if.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";

/* A control buffer aligned for struct cmsghdr. */
typedef union { struct cmsghdr h; char b[256]; } ctlbuf;

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A bound loopback receiver and a sender, a datagram from one to the other. */
static int pair(int family, int *rfd, int *sfd, struct sockaddr_storage *dst,
    socklen_t *dlen)
{
    unsigned short nport;

    *rfd = st_bound(family, SOCK_DGRAM, &nport);
    *sfd = socket(family, SOCK_DGRAM, 0);
    if (*rfd < 0 || *sfd < 0)
        return -1;
    *dlen = st_loopback(family, dst, nport);
    return 0;
}

/* recvmsg() one datagram with a control buffer of ctlsize bytes. */
static int recv_ctl(int fd, ctlbuf *ctl,
    int ctlsize, struct msghdr *mh)
{
    static char data[32];
    static struct iovec iov;

    bzero(mh, sizeof(*mh));
    bzero(ctl, sizeof(*ctl));
    iov.iov_base = data;
    iov.iov_len = sizeof(data);
    mh->msg_iov = &iov;
    mh->msg_iovlen = 1;
    mh->msg_control = ctl;
    mh->msg_controllen = ctlsize;
    if (sock_wait(fd, 0, 3) <= 0)
        return -1;
    return recvmsg(fd, mh, 0);
}

static struct cmsghdr *find(struct msghdr *mh, int level, int type)
{
    struct cmsghdr *c;
    for (c = CMSG_FIRSTHDR(mh); c != NULL; c = CMSG_NXTHDR(mh, c))
        if (c->cmsg_level == level && c->cmsg_type == type)
            return c;
    return NULL;
}

static unsigned lo_index(void)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    unsigned idx = 0;

    bzero(&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name, "lo0");
    if (s >= 0 && IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
        idx = ifr.ifr_index;
    if (s >= 0)
        CloseSocket(s);
    return idx;
}

/* IPV6_RECVPKTINFO: the destination and the receiving interface, whole. */
void testCMSG_V6_PKTINFO(void)
{
    ctlbuf ctl;
    struct sockaddr_storage dst;
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct msghdr mh;
    struct cmsghdr *c;
    socklen_t dlen;
    int r, s, on = 1;

    CU_ASSERT_EQUAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(sendto(s, "pi", 2, 0, (struct sockaddr *)&dst, dlen), 2);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, sizeof(ctl), &mh), 2);
    c = find(&mh, IPPROTO_IPV6, IPV6_PKTINFO);
    CU_ASSERT_PTR_NOT_NULL(c);
    if (c != NULL) {
        struct in6_pktinfo pi;
        CU_ASSERT_EQUAL(c->cmsg_len, CMSG_LEN(sizeof(struct in6_pktinfo)));
        memcpy(&pi, CMSG_DATA(c), sizeof(pi));
        CU_ASSERT(memcmp(&pi.ipi6_addr, &lo, sizeof(lo)) == 0);
        CU_ASSERT_EQUAL(pi.ipi6_ifindex, lo_index());
    }
    CU_ASSERT_EQUAL(mh.msg_flags & MSG_CTRUNC, 0);
    CloseSocket(s);
    CloseSocket(r);
}

/* IPV6_RECVHOPLIMIT: the hop limit the datagram arrived with. */
void testCMSG_V6_HOPLIMIT(void)
{
    ctlbuf ctl;
    struct sockaddr_storage dst;
    struct msghdr mh;
    struct cmsghdr *c;
    socklen_t dlen;
    int r, s, on = 1, hops = 47;

    CU_ASSERT_EQUAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)), 0);
    CU_ASSERT_EQUAL(sendto(s, "hl", 2, 0, (struct sockaddr *)&dst, dlen), 2);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, sizeof(ctl), &mh), 2);
    c = find(&mh, IPPROTO_IPV6, IPV6_HOPLIMIT);
    CU_ASSERT_PTR_NOT_NULL(c);
    if (c != NULL) {
        int hl = 0;
        CU_ASSERT_EQUAL(c->cmsg_len, CMSG_LEN(sizeof(int)));
        memcpy(&hl, CMSG_DATA(c), sizeof(hl));
        CU_ASSERT_EQUAL(hl, 47);
    }
    CloseSocket(s);
    CloseSocket(r);
}

/* Both at once: two messages, each found by walking the buffer. */
void testCMSG_V6_BOTH(void)
{
    ctlbuf ctl;
    struct sockaddr_storage dst;
    struct msghdr mh;
    socklen_t dlen;
    int r, s, on = 1;

    CU_ASSERT_EQUAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    setsockopt(r, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on));
    setsockopt(r, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
    CU_ASSERT_EQUAL(sendto(s, "two", 3, 0, (struct sockaddr *)&dst, dlen), 3);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, sizeof(ctl), &mh), 3);
    CU_ASSERT_PTR_NOT_NULL(find(&mh, IPPROTO_IPV6, IPV6_PKTINFO));
    CU_ASSERT_PTR_NOT_NULL(find(&mh, IPPROTO_IPV6, IPV6_HOPLIMIT));
    CU_ASSERT_EQUAL(mh.msg_flags & MSG_CTRUNC, 0);
    CloseSocket(s);
    CloseSocket(r);
}

/* A buffer with room for the first message only reports MSG_CTRUNC. */
void testCMSG_V6_CTRUNC(void)
{
    ctlbuf ctl;
    struct sockaddr_storage dst;
    struct msghdr mh;
    socklen_t dlen;
    int r, s, on = 1;

    CU_ASSERT_EQUAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    setsockopt(r, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on));
    setsockopt(r, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
    CU_ASSERT_EQUAL(sendto(s, "cut", 3, 0, (struct sockaddr *)&dst, dlen), 3);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, CMSG_SPACE(sizeof(struct in6_pktinfo)), &mh), 3);
    CU_ASSERT_PTR_NOT_NULL(find(&mh, IPPROTO_IPV6, IPV6_PKTINFO));
    CU_ASSERT(mh.msg_flags & MSG_CTRUNC);
    CloseSocket(s);
    CloseSocket(r);
}

/* IPv4 IP_RECVDSTADDR: the destination address, whole. */
void testCMSG_V4_RECVDSTADDR(void)
{
    ctlbuf ctl;
    struct sockaddr_storage dst;
    struct msghdr mh;
    struct cmsghdr *c;
    socklen_t dlen;
    int r, s, on = 1;

    CU_ASSERT_EQUAL(pair(AF_INET, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(sendto(s, "v4", 2, 0, (struct sockaddr *)&dst, dlen), 2);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, sizeof(ctl), &mh), 2);
    c = find(&mh, IPPROTO_IP, IP_RECVDSTADDR);
    CU_ASSERT_PTR_NOT_NULL(c);
    if (c != NULL) {
        struct in_addr a;
        CU_ASSERT_EQUAL(c->cmsg_len, CMSG_LEN(sizeof(struct in_addr)));
        memcpy(&a, CMSG_DATA(c), sizeof(a));
        CU_ASSERT_EQUAL(a.s_addr, htonl(INADDR_LOOPBACK));
    }
    CloseSocket(s);
    CloseSocket(r);
}

/* sendto() with one ancillary message of len bytes of data. */
static int send_ctl(int s, struct sockaddr_storage *dst, socklen_t dlen,
    int level, int type, const void *data, int len)
{
    ctlbuf ctl;
    struct iovec iov;
    struct msghdr mh;
    struct cmsghdr *c;

    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    iov.iov_base = "c";
    iov.iov_len = 1;
    mh.msg_name = dst;
    mh.msg_namelen = dlen;
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = &ctl;
    mh.msg_controllen = CMSG_SPACE(len);
    c = CMSG_FIRSTHDR(&mh);
    c->cmsg_level = level;
    c->cmsg_type = type;
    c->cmsg_len = CMSG_LEN(len);
    memcpy(CMSG_DATA(c), data, len);
    return sendmsg(s, &mh, 0);
}

/* The int an IPV6_TCLASS message carries, or -1 without one. */
static int got_tclass(int r)
{
    ctlbuf ctl;
    struct msghdr mh;
    struct cmsghdr *c;
    int v = -1;

    if (recv_ctl(r, &ctl, sizeof(ctl), &mh) < 0)
        return -2;
    c = find(&mh, IPPROTO_IPV6, IPV6_TCLASS);
    if (c != NULL && c->cmsg_len == CMSG_LEN(sizeof(int)))
        memcpy(&v, CMSG_DATA(c), sizeof(v));
    return v;
}

/* The byte an IP_RECVTOS message carries, or -1 without one. */
static int got_tos(int r)
{
    ctlbuf ctl;
    struct msghdr mh;
    struct cmsghdr *c;

    if (recv_ctl(r, &ctl, sizeof(ctl), &mh) < 0)
        return -2;
    c = find(&mh, IPPROTO_IP, IP_RECVTOS);
    if (c == NULL || c->cmsg_len != CMSG_LEN(1))
        return -1;
    return *(unsigned char *)CMSG_DATA(c);
}

/* IPV6_TCLASS: set on the socket, or on one datagram, it is what the
 * datagram arrives with; IPV6_RECVTCLASS reports it. */
void testCMSG_V6_TCLASS(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    int r, s, on = 1, tc = 0xba, ce = 3, bad = 256;

    CU_ASSERT_EQUAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IPV6, IPV6_RECVTCLASS, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(sendto(s, "a", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT_EQUAL(got_tclass(r), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_TCLASS, &tc, sizeof(tc)), 0);
    CU_ASSERT_EQUAL(sendto(s, "b", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT_EQUAL(got_tclass(r), 0xba);
    CU_ASSERT_EQUAL(send_ctl(s, &dst, dlen, IPPROTO_IPV6, IPV6_TCLASS, &ce, sizeof(ce)), 1);
    CU_ASSERT_EQUAL(got_tclass(r), 3);
    CU_ASSERT_NOT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_TCLASS, &bad, sizeof(bad)), 0);
    CloseSocket(s);
    CloseSocket(r);
}

/* IP_TOS: set on the socket, or on one datagram (a byte or an int), it is
 * what the datagram arrives with; IP_RECVTOS reports it. */
void testCMSG_V4_TOS(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    unsigned char b = 1;
    int r, s, on = 1, tos = 0xba, i = 2;

    CU_ASSERT_EQUAL(pair(AF_INET, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IP, IP_RECVTOS, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)), 0);
    CU_ASSERT_EQUAL(sendto(s, "a", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT_EQUAL(got_tos(r), 0xba);
    CU_ASSERT_EQUAL(send_ctl(s, &dst, dlen, IPPROTO_IP, IP_TOS, &b, 1), 1);
    CU_ASSERT_EQUAL(got_tos(r), 1);
    CU_ASSERT_EQUAL(send_ctl(s, &dst, dlen, IPPROTO_IP, IP_TOS, &i, sizeof(i)), 1);
    CU_ASSERT_EQUAL(got_tos(r), 2);
    CloseSocket(s);
    CloseSocket(r);
}

/* A multicast datagram carries the ancillary data too: IP_RECVDSTADDR is
 * the group, IP_RECVTOS the TOS byte. */
void testCMSG_V4_MCAST(void)
{
    ctlbuf ctl;
    struct sockaddr_in a, g;
    struct ip_mreq mr;
    struct ifreq ifr;
    struct msghdr mh;
    struct cmsghdr *c;
    socklen_t l = sizeof(a);
    unsigned char ttl = 1, loop = 1;
    int r, s, on = 1, tos = 0x02;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) != 0 ||
            ((struct sockaddr_in *)&ifr.ifr_addr)->sin_family != AF_INET) {
        CloseSocket(s);
        CU_PASS("no IPv4 address on the interface - skipping");
        return;
    }
    r = socket(AF_INET, SOCK_DGRAM, 0);     /* on the wildcard address */
    CU_ASSERT_FATAL(r >= 0);
    bzero(&a, sizeof(a));
    a.sin_len = sizeof(a);
    a.sin_family = AF_INET;
    CU_ASSERT_EQUAL(bind(r, (struct sockaddr *)&a, sizeof(a)), 0);
    CU_ASSERT_EQUAL(getsockname(r, (struct sockaddr *)&a, &l), 0);
    bzero(&mr, sizeof(mr));
    mr.imr_multiaddr.s_addr = htonl(0xef4d0101);    /* 239.77.1.1 */
    mr.imr_interface = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IP, IP_RECVTOS, &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &mr.imr_interface, sizeof(mr.imr_interface)), 0);
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)), 0);
    bzero(&g, sizeof(g));
    g.sin_len = sizeof(g);
    g.sin_family = AF_INET;
    g.sin_port = a.sin_port;
    g.sin_addr = mr.imr_multiaddr;
    CU_ASSERT_EQUAL(sendto(s, "m4", 2, 0, (struct sockaddr *)&g, sizeof(g)), 2);
    CU_ASSERT_EQUAL(recv_ctl(r, &ctl, sizeof(ctl), &mh), 2);
    c = find(&mh, IPPROTO_IP, IP_RECVDSTADDR);
    CU_ASSERT_PTR_NOT_NULL(c);
    if (c != NULL) {
        struct in_addr d;
        memcpy(&d, CMSG_DATA(c), sizeof(d));
        CU_ASSERT_EQUAL(d.s_addr, mr.imr_multiaddr.s_addr);
    }
    c = find(&mh, IPPROTO_IP, IP_RECVTOS);
    CU_ASSERT_PTR_NOT_NULL(c);
    if (c != NULL)
        CU_ASSERT_EQUAL(*(unsigned char *)CMSG_DATA(c), 0x02);
    setsockopt(r, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mr, sizeof(mr));
    CloseSocket(s);
    CloseSocket(r);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping CMSG tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("CMSG_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "IPV6_PKTINFO over ::1", testCMSG_V6_PKTINFO) ||
        !CU_add_test(p, "IPV6_HOPLIMIT over ::1", testCMSG_V6_HOPLIMIT) ||
        !CU_add_test(p, "both IPv6 messages arrive", testCMSG_V6_BOTH) ||
        !CU_add_test(p, "short control buffer sets MSG_CTRUNC", testCMSG_V6_CTRUNC) ||
        !CU_add_test(p, "IP_RECVDSTADDR over 127.0.0.1", testCMSG_V4_RECVDSTADDR) ||
        !CU_add_test(p, "IPV6_TCLASS by option and per datagram", testCMSG_V6_TCLASS) ||
        !CU_add_test(p, "IP_TOS by option and per datagram", testCMSG_V4_TOS) ||
        !CU_add_test(p, "a multicast datagram has its ancillary data", testCMSG_V4_MCAST))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-CMSG");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
