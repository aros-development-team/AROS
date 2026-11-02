/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for SO_TIMESTAMP and SO_TS_CLOCK (FreeBSD's): a UDP
    datagram carries the time the stack received it - SCM_TIMESTAMP, a
    struct timeval on gettimeofday()'s clock, by default; SCM_MONOTONIC, a
    struct timespec on clock_gettime(CLOCK_MONOTONIC)'s, with
    SO_TS_MONOTONIC. The stamp lies between the time before the datagram
    was sent and the time the socket became readable, however late
    recvmsg() comes; datagrams received apart have stamps that far apart;
    each keeps the kind of stamp the socket asked for when it came in. The
    options' defaults and errors, and the layout of the two structures the
    stack hands over, are checked too. Over 127.0.0.1 and ::1.
    Usage:  cunit-net-timestamp
*/

#include "socktest.h"

#include <proto/dos.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>

struct Library *SocketBase = NULL;

typedef union { struct cmsghdr h; char b[256]; } ctlbuf;

static int geti(int s, int opt)
{
    int v = -99;
    socklen_t l = sizeof(v);

    if (getsockopt(s, SOL_SOCKET, opt, &v, &l) != 0)
        return -98;
    return v;
}

static int seti(int s, int opt, int v)
{
    return setsockopt(s, SOL_SOCKET, opt, &v, sizeof(v));
}

/* Microseconds, on gettimeofday()'s clock or CLOCK_MONOTONIC's. */
static long long real_us(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

static long long mono_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* The next datagram on r: its stamp in microseconds, and which message
 * carried it (SCM_TIMESTAMP, SCM_MONOTONIC, 0 for none; -1 on error). */
static int recv_stamp(int r, long long *us)
{
    static char data[32];
    ctlbuf ctl;
    struct iovec iov;
    struct msghdr mh;
    struct cmsghdr *c;

    *us = 0;
    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    iov.iov_base = data;
    iov.iov_len = sizeof(data);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = &ctl;
    mh.msg_controllen = sizeof(ctl.b);
    if (recvmsg(r, &mh, 0) < 0)
        return -1;
    for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c)) {
        if (c->cmsg_level != SOL_SOCKET)
            continue;
        if (c->cmsg_type == SCM_TIMESTAMP && c->cmsg_len == CMSG_LEN(sizeof(struct timeval))) {
            struct timeval tv;

            memcpy(&tv, CMSG_DATA(c), sizeof(tv));
            *us = (long long)tv.tv_sec * 1000000 + tv.tv_usec;
            return SCM_TIMESTAMP;
        }
        if (c->cmsg_type == SCM_MONOTONIC && c->cmsg_len == CMSG_LEN(sizeof(struct timespec))) {
            struct timespec ts;

            memcpy(&ts, CMSG_DATA(c), sizeof(ts));
            *us = (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
            return SCM_MONOTONIC;
        }
        return -1;          /* a timestamp of the wrong size */
    }
    return 0;
}

/* A bound loopback receiver and a sender. */
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

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* The structures the stack hands over: timeval is two 32-bit fields on
 * every AROS target; timespec is a 32-bit time_t and a long, so 8 bytes on
 * 32-bit targets and 16 on 64-bit ones. */
void testTIMESTAMP_LAYOUT(void)
{
    CU_ASSERT_EQUAL(sizeof(struct timeval), 8);
    CU_ASSERT_EQUAL(offsetof(struct timeval, tv_sec), 0);
    CU_ASSERT_EQUAL(offsetof(struct timeval, tv_usec), 4);
    CU_ASSERT_EQUAL(sizeof(((struct timeval *)0)->tv_sec), 4);
    CU_ASSERT_EQUAL(sizeof(((struct timeval *)0)->tv_usec), 4);
    CU_ASSERT_EQUAL(sizeof(((struct timespec *)0)->tv_sec), 4);
    CU_ASSERT_EQUAL(sizeof(((struct timespec *)0)->tv_nsec), sizeof(long));
    CU_ASSERT_EQUAL(offsetof(struct timespec, tv_sec), 0);
    CU_ASSERT_EQUAL(offsetof(struct timespec, tv_nsec), sizeof(long));
    CU_ASSERT_EQUAL(sizeof(struct timespec), 2 * sizeof(long));
}

/* Off, and SO_TS_REALTIME_MICRO, until set; the clocks not implemented
 * give EOPNOTSUPP, those that do not exist EINVAL. */
void testTIMESTAMP_OPTIONS(void)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(geti(s, SO_TIMESTAMP), 0);
    CU_ASSERT_EQUAL(geti(s, SO_TS_CLOCK), SO_TS_REALTIME_MICRO);
    CU_ASSERT_EQUAL(seti(s, SO_TIMESTAMP, 1), 0);
    CU_ASSERT_NOT_EQUAL(geti(s, SO_TIMESTAMP), 0);
    CU_ASSERT_EQUAL(seti(s, SO_TS_CLOCK, SO_TS_MONOTONIC), 0);
    CU_ASSERT_EQUAL(geti(s, SO_TS_CLOCK), SO_TS_MONOTONIC);
    CU_ASSERT_EQUAL(seti(s, SO_TS_CLOCK, SO_TS_REALTIME), -1);
    CU_ASSERT_EQUAL(Errno(), EOPNOTSUPP);
    CU_ASSERT_EQUAL(seti(s, SO_TS_CLOCK, SO_TS_BINTIME), -1);
    CU_ASSERT_EQUAL(Errno(), EOPNOTSUPP);
    CU_ASSERT_EQUAL(seti(s, SO_TS_CLOCK, SO_TS_CLOCK_MAX + 1), -1);
    CU_ASSERT_EQUAL(Errno(), EINVAL);
    CU_ASSERT_EQUAL(seti(s, SO_TS_CLOCK, -1), -1);
    CU_ASSERT_EQUAL(Errno(), EINVAL);
    CU_ASSERT_EQUAL(geti(s, SO_TS_CLOCK), SO_TS_MONOTONIC);
    CU_ASSERT_EQUAL(seti(s, SO_TIMESTAMP, 0), 0);
    CU_ASSERT_EQUAL(geti(s, SO_TIMESTAMP), 0);
    CloseSocket(s);
}

/* Without SO_TIMESTAMP, no stamp. */
void testTIMESTAMP_OFF(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    long long us;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(AF_INET, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    CU_ASSERT_EQUAL(recv_stamp(r, &us), 0);
    CloseSocket(s);
    CloseSocket(r);
}

/*
 * The stamp is the time of reception: after the time just before the
 * datagram was sent, before the time the socket became readable - not
 * the time recvmsg() is called, half a second later.
 */
static void at_reception(int family, int clock)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    long long before, readable, us;
    long long (*now)(void) = clock == SO_TS_MONOTONIC ? mono_us : real_us;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(family, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(seti(r, SO_TIMESTAMP, 1), 0);
    CU_ASSERT_EQUAL(seti(r, SO_TS_CLOCK, clock), 0);
    before = now();
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    readable = now();
    Delay(TICKS_PER_SECOND / 2);
    CU_ASSERT_EQUAL(recv_stamp(r, &us), clock == SO_TS_MONOTONIC ? SCM_MONOTONIC : SCM_TIMESTAMP);
    CU_ASSERT(us >= before && us <= readable);
    CloseSocket(s);
    CloseSocket(r);
}

void testTIMESTAMP_REALTIME4(void) { at_reception(AF_INET, SO_TS_REALTIME_MICRO); }
void testTIMESTAMP_REALTIME6(void) { at_reception(AF_INET6, SO_TS_REALTIME_MICRO); }
void testTIMESTAMP_MONOTONIC4(void) { at_reception(AF_INET, SO_TS_MONOTONIC); }
void testTIMESTAMP_MONOTONIC6(void) { at_reception(AF_INET6, SO_TS_MONOTONIC); }

/* Two datagrams received 300 ms apart, read together: stamps 300 ms
 * apart. */
void testTIMESTAMP_DISTINCT(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    long long a, b;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(AF_INET6, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(seti(r, SO_TIMESTAMP, 1), 0);
    CU_ASSERT_EQUAL(seti(r, SO_TS_CLOCK, SO_TS_MONOTONIC), 0);
    CU_ASSERT_EQUAL(sendto(s, "1", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    Delay(TICKS_PER_SECOND * 3 / 10);
    CU_ASSERT_EQUAL(sendto(s, "2", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    Delay(TICKS_PER_SECOND / 10);
    CU_ASSERT_EQUAL(recv_stamp(r, &a), SCM_MONOTONIC);
    CU_ASSERT_EQUAL(recv_stamp(r, &b), SCM_MONOTONIC);
    CU_ASSERT(b - a >= 250000 && b - a <= 500000);
    CloseSocket(s);
    CloseSocket(r);
}

/* A datagram keeps the stamp its socket asked for when it came in: the
 * clock changed between two waiting datagrams changes the second only. */
void testTIMESTAMP_PER_DATAGRAM(void)
{
    struct sockaddr_storage dst;
    socklen_t dlen;
    long long us;
    int r, s;

    CU_ASSERT_EQUAL_FATAL(pair(AF_INET, &r, &s, &dst, &dlen), 0);
    CU_ASSERT_EQUAL(seti(r, SO_TIMESTAMP, 1), 0);
    CU_ASSERT_EQUAL(sendto(s, "1", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    CU_ASSERT(sock_wait(r, 0, 2) > 0);
    CU_ASSERT_EQUAL(seti(r, SO_TS_CLOCK, SO_TS_MONOTONIC), 0);
    CU_ASSERT_EQUAL(sendto(s, "2", 1, 0, (struct sockaddr *)&dst, dlen), 1);
    Delay(TICKS_PER_SECOND / 10);
    CU_ASSERT_EQUAL(recv_stamp(r, &us), SCM_TIMESTAMP);
    CU_ASSERT_EQUAL(recv_stamp(r, &us), SCM_MONOTONIC);
    CloseSocket(s);
    CloseSocket(r);
}

int main(void)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TIMESTAMP tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TIMESTAMP_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "timeval and timespec layout", testTIMESTAMP_LAYOUT) ||
        !CU_add_test(p, "the options, their defaults and errors", testTIMESTAMP_OPTIONS) ||
        !CU_add_test(p, "no stamp without SO_TIMESTAMP", testTIMESTAMP_OFF) ||
        !CU_add_test(p, "SCM_TIMESTAMP at reception (IPv4)", testTIMESTAMP_REALTIME4) ||
        !CU_add_test(p, "SCM_TIMESTAMP at reception (IPv6)", testTIMESTAMP_REALTIME6) ||
        !CU_add_test(p, "SCM_MONOTONIC at reception (IPv4)", testTIMESTAMP_MONOTONIC4) ||
        !CU_add_test(p, "SCM_MONOTONIC at reception (IPv6)", testTIMESTAMP_MONOTONIC6) ||
        !CU_add_test(p, "datagrams received apart, stamps apart", testTIMESTAMP_DISTINCT) ||
        !CU_add_test(p, "each datagram keeps its kind of stamp", testTIMESTAMP_PER_DATAGRAM))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TIMESTAMP");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
