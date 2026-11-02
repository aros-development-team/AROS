/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for what a UDP application relies on (QUIC, Network MIDI,
    mDNS), over 127.0.0.1 and ::1:
    - datagrams of 1200 to 9000 bytes arrive whole and unchanged (above
      the loopback MTU, 1536, they are fragmented and reassembled on the
      way); one larger than the send buffer (9216 bytes by default) is
      refused with EMSGSIZE - a configured limit, raised with SO_SNDBUF;
    - a datagram read into a shorter buffer is cut, with MSG_TRUNC in
      msg_flags, and the rest is gone;
    - SO_RCVBUF reads back what was set, and a burst that fits it arrives
      complete and in order;
    - an empty non-blocking socket says EWOULDBLOCK;
    - every datagram of a burst carries the ancillary data asked for.
    Usage:  cunit-net-udpio
*/

#include "socktest.h"

struct Library *SocketBase = NULL;

typedef union { struct cmsghdr h; char b[256]; } ctlbuf;

static unsigned char sbuf[16384], rbuf[16384];

static int pair(int family, int *r, int *s, struct sockaddr_storage *dst, socklen_t *dlen)
{
    unsigned short port;

    *r = st_bound(family, SOCK_DGRAM, &port);
    *s = socket(family, SOCK_DGRAM, 0);
    if (*r < 0 || *s < 0)
        return -1;
    *dlen = st_loopback(family, dst, port);
    return 0;
}

static void fill(int n, unsigned seed)
{
    int i;

    for (i = 0; i < n; i++)
        sbuf[i] = (unsigned char)(i * 7 + seed);
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* Sizes up to 9000 bytes, whole and unchanged. */
static void sizes(int family)
{
    static const int len[] = { 1200, 1452, 1472, 1500, 4000, 8192, 9000 };
    struct sockaddr_storage dst;
    socklen_t dlen;
    int r, s, i;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    for (i = 0; i < (int)(sizeof(len) / sizeof(len[0])); i++) {
        fill(len[i], i);
        CU_ASSERT_EQUAL(sendto(s, sbuf, len[i], 0, (struct sockaddr *)&dst, dlen), len[i]);
        CU_ASSERT(sock_wait(r, 0, 2) > 0);
        CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), len[i]);
        CU_ASSERT(memcmp(sbuf, rbuf, len[i]) == 0);
    }
    CloseSocket(s);
    CloseSocket(r);
}

void testUDPIO_SIZES4(void) { sizes(AF_INET); }
void testUDPIO_SIZES6(void) { sizes(AF_INET6); }

/* Larger than the send buffer: EMSGSIZE; with a larger SO_SNDBUF, sent. */
static void sndbuf_limit(int family)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    int r, s, big = 16384;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(sendto(s, sbuf, 9217, 0, (struct sockaddr *)&dst, dlen), -1);
    CU_ASSERT_EQUAL(Errno(), EMSGSIZE);
    CU_ASSERT_EQUAL(setsockopt(s, SOL_SOCKET, SO_SNDBUF, &big, sizeof(big)), 0);
    CU_ASSERT_EQUAL(setsockopt(r, SOL_SOCKET, SO_RCVBUF, &big, sizeof(big)), 0);
    fill(12000, 3);
    CU_ASSERT_EQUAL(sendto(s, sbuf, 12000, 0, (struct sockaddr *)&dst, dlen), 12000);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), 12000);
    CU_ASSERT(memcmp(sbuf, rbuf, 12000) == 0);
    CloseSocket(s);
    CloseSocket(r);
}

void testUDPIO_SNDBUF4(void) { sndbuf_limit(AF_INET); }
void testUDPIO_SNDBUF6(void) { sndbuf_limit(AF_INET6); }

/* A short buffer: the datagram is cut, MSG_TRUNC set, the rest dropped. */
static void truncated(int family)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    struct iovec iov;
    struct msghdr mh;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    fill(1000, 9);
    CU_ASSERT_EQUAL(sendto(s, sbuf, 1000, 0, (struct sockaddr *)&dst, dlen), 1000);
    CU_ASSERT_EQUAL(sendto(s, "next", 4, 0, (struct sockaddr *)&dst, dlen), 4);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    bzero(&mh, sizeof(mh));
    iov.iov_base = rbuf;
    iov.iov_len = 100;
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    CU_ASSERT_EQUAL(recvmsg(r, &mh, 0), 100);
    CU_ASSERT(mh.msg_flags & MSG_TRUNC);
    CU_ASSERT(memcmp(sbuf, rbuf, 100) == 0);
    /* the next read is the next datagram, not the rest of this one */
    CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), 4);
    CU_ASSERT(memcmp(rbuf, "next", 4) == 0);
    CloseSocket(s);
    CloseSocket(r);
}

void testUDPIO_TRUNC4(void) { truncated(AF_INET); }
void testUDPIO_TRUNC6(void) { truncated(AF_INET6); }

/* SO_RCVBUF reads back; a burst that fits it arrives complete and in
 * order. */
static void burst(int family)
{
    struct sockaddr_storage dst;
    socklen_t dlen, l = sizeof(int);
    int r, s, i, rb = 65536, got = -1, n = 0, ok = 1;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(setsockopt(r, SOL_SOCKET, SO_RCVBUF, &rb, sizeof(rb)), 0);
    CU_ASSERT_EQUAL(getsockopt(r, SOL_SOCKET, SO_RCVBUF, &got, &l), 0);
    CU_ASSERT_EQUAL(got, rb);
    for (i = 0; i < 40; i++) {      /* 40 x 1200 bytes: 48 KB, fits */
        fill(1200, i);
        sbuf[0] = (unsigned char)i;
        CU_ASSERT_EQUAL(sendto(s, sbuf, 1200, 0, (struct sockaddr *)&dst, dlen), 1200);
    }
    while (sock_wait(r, 0, 1) > 0 && recv(r, rbuf, sizeof(rbuf), 0) == 1200) {
        if (rbuf[0] != (unsigned char)n)
            ok = 0;
        n++;
    }
    CU_ASSERT_EQUAL(n, 40);
    CU_ASSERT(ok);
    CloseSocket(s);
    CloseSocket(r);
}

void testUDPIO_BURST4(void) { burst(AF_INET); }
void testUDPIO_BURST6(void) { burst(AF_INET6); }

/* An empty non-blocking socket: EWOULDBLOCK, at once. */
void testUDPIO_NONBLOCK(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    long on = 1;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(IoctlSocket(r, FIONBIO, (char *)&on), 0);
    CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), 1);
    CU_ASSERT_EQUAL(recv(r, rbuf, sizeof(rbuf), 0), -1);
    CU_ASSERT_EQUAL(Errno(), EWOULDBLOCK);
    CloseSocket(s);
    CloseSocket(r);
}

/* Every datagram of a burst has the destination address it asked for. */
static void ancillary(int family)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    int r, s, i, on = 1, n = 0, with = 0;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(family == AF_INET ?
                    setsockopt(r, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) :
                    setsockopt(r, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)), 0);
    for (i = 0; i < 30; i++)
        CU_ASSERT_EQUAL(sendto(s, sbuf, 200, 0, (struct sockaddr *)&dst, dlen), 200);
    while (sock_wait(r, 0, 1) > 0) {
        ctlbuf ctl;
        struct iovec iov;
        struct msghdr mh;
        struct cmsghdr *c;

        bzero(&mh, sizeof(mh));
        iov.iov_base = rbuf;
        iov.iov_len = sizeof(rbuf);
        mh.msg_iov = &iov;
        mh.msg_iovlen = 1;
        mh.msg_control = &ctl;
        mh.msg_controllen = sizeof(ctl.b);
        if (recvmsg(r, &mh, 0) != 200)
            break;
        n++;
        for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c))
            if ((family == AF_INET && c->cmsg_level == IPPROTO_IP && c->cmsg_type == IP_RECVDSTADDR) ||
                    (family == AF_INET6 && c->cmsg_level == IPPROTO_IPV6 && c->cmsg_type == IPV6_PKTINFO)) {
                with++;
                break;
            }
    }
    CU_ASSERT_EQUAL(n, 30);
    CU_ASSERT_EQUAL(with, 30);
    CloseSocket(s);
    CloseSocket(r);
}

void testUDPIO_ANCILLARY4(void) { ancillary(AF_INET); }
void testUDPIO_ANCILLARY6(void) { ancillary(AF_INET6); }

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping UDPIO tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("UDPIO_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "1200 to 9000 bytes, whole (IPv4)", testUDPIO_SIZES4) ||
        !CU_add_test(p, "1200 to 9000 bytes, whole (IPv6)", testUDPIO_SIZES6) ||
        !CU_add_test(p, "above the send buffer: EMSGSIZE, until SO_SNDBUF (IPv4)", testUDPIO_SNDBUF4) ||
        !CU_add_test(p, "above the send buffer: EMSGSIZE, until SO_SNDBUF (IPv6)", testUDPIO_SNDBUF6) ||
        !CU_add_test(p, "a short buffer cuts with MSG_TRUNC (IPv4)", testUDPIO_TRUNC4) ||
        !CU_add_test(p, "a short buffer cuts with MSG_TRUNC (IPv6)", testUDPIO_TRUNC6) ||
        !CU_add_test(p, "a burst that fits SO_RCVBUF arrives whole (IPv4)", testUDPIO_BURST4) ||
        !CU_add_test(p, "a burst that fits SO_RCVBUF arrives whole (IPv6)", testUDPIO_BURST6) ||
        !CU_add_test(p, "an empty non-blocking socket says EWOULDBLOCK", testUDPIO_NONBLOCK) ||
        !CU_add_test(p, "every datagram has its ancillary data (IPv4)", testUDPIO_ANCILLARY4) ||
        !CU_add_test(p, "every datagram has its ancillary data (IPv6)", testUDPIO_ANCILLARY6))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-UDPIO");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
