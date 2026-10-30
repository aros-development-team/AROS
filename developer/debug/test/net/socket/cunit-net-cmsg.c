/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the ancillary data recvmsg() returns with a datagram over
    the loopback: IPV6_PKTINFO (RFC 3542 section 6.1), IPV6_HOPLIMIT
    (section 6.3) and IPv4's IP_RECVDSTADDR. Each message must carry its whole
    data at CMSG_DATA() with cmsg_len = CMSG_LEN() - on LP64 the data starts
    past padding after the header - several messages must all arrive, walkable
    with CMSG_NXTHDR(), and a control buffer too small for them must be
    reported with MSG_CTRUNC.
*/

#include "socktest.h"

#include <net/if.h>

struct Library *SocketBase = NULL;

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

int main(void)
{
    CU_pSuite p;

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
        !CU_add_test(p, "IP_RECVDSTADDR over 127.0.0.1", testCMSG_V4_RECVDSTADDR))
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
