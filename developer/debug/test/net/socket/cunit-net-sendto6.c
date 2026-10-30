/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for sendto() on an unconnected UDP/IPv6 socket: the source
    the stack picks for one datagram must not stick to the socket. The
    socket stays bound to the unspecified address, keeps receiving
    datagrams sent to its other addresses, and an IPV6_PKTINFO source on a
    later datagram is used.

    Everything runs over the loopback addresses ::1 and the link-local of
    an interface (default eth0, as the first argument). Without that
    interface's link-local the tests that need it are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int have_ll;

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr,
                unsigned short port, unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_port = port;
    a->sin6_scope_id = scope;
}

/* A UDP socket bound to [addr]:0; *port gets its port (network order). */
static int udp_bound(const struct in6_addr *addr, unsigned short *port)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    sa6(&a, addr, 0, 0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            getsockname(s, (struct sockaddr *)&a, &len) != 0) {
        CloseSocket(s);
        return -1;
    }
    *port = a.sin6_port;
    return s;
}

static int recv_from(int s, struct sockaddr_in6 *from)
{
    char buf[64];
    socklen_t len = sizeof(*from);

    if (sock_wait(s, 0, 2) <= 0)
        return -1;
    return recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)from, &len);
}

#define NEED_LL() do { if (!have_ll) { CU_PASS("no link-local - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* After sendto(), the socket is still bound to the unspecified address. */
void testSENDTO6_STAYS_UNBOUND(void)
{
    struct in6_addr any = IN6ADDR_ANY_INIT, lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d, l;
    socklen_t len = sizeof(l);
    unsigned short sp, rp;
    int s = udp_bound(&any, &sp), r = udp_bound(&lo, &rp);

    CU_ASSERT_FATAL(s >= 0 && r >= 0);
    sa6(&d, &lo, rp, 0);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&l, &len), 0);
    CU_ASSERT(IN6_IS_ADDR_UNSPECIFIED(&l.sin6_addr));
    CU_ASSERT_EQUAL(l.sin6_port, sp);
    CloseSocket(r);
    CloseSocket(s);
}

/* ...and still receives datagrams sent to another of its addresses. */
void testSENDTO6_STILL_RECEIVES(void)
{
    struct in6_addr any = IN6ADDR_ANY_INIT, lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d, from;
    unsigned short sp, rp;
    int s, r, t;
    NEED_LL();
    s = udp_bound(&any, &sp);
    r = udp_bound(&lo, &rp);
    t = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0 && r >= 0 && t >= 0);
    sa6(&d, &lo, rp, 0);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    /* now to s on the link-local address, not the one it sent from */
    sa6(&d, &linklocal, sp, ifindex);
    CU_ASSERT_EQUAL(sendto(t, "y", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL(recv_from(s, &from), 1);
    CloseSocket(t);
    CloseSocket(r);
    CloseSocket(s);
}

/* An IPV6_PKTINFO source is used, also after an earlier sendto(). */
void testSENDTO6_PKTINFO_SOURCE(void)
{
    struct in6_addr any = IN6ADDR_ANY_INIT, lo = IN6ADDR_LOOPBACK_INIT;
    union { struct cmsghdr c; char b[CMSG_SPACE(sizeof(struct in6_pktinfo))]; } ctl;
    struct sockaddr_in6 d, from;
    struct in6_pktinfo pi;
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    unsigned short sp, rp;
    int s, r;
    NEED_LL();
    s = udp_bound(&any, &sp);
    r = udp_bound(&any, &rp);
    CU_ASSERT_FATAL(s >= 0 && r >= 0);

    sa6(&d, &lo, rp, 0);
    CU_ASSERT_EQUAL(sendto(s, "x", 1, 0, (struct sockaddr *)&d, sizeof(d)), 1);
    CU_ASSERT_EQUAL(recv_from(r, &from), 1);

    /* to ::1 again, from the link-local this time */
    bzero(&mh, sizeof(mh));
    bzero(&ctl, sizeof(ctl));
    iov.iov_base = "y";
    iov.iov_len = 1;
    mh.msg_name = &d;
    mh.msg_namelen = sizeof(d);
    mh.msg_iov = &iov;
    mh.msg_iovlen = 1;
    mh.msg_control = &ctl;
    mh.msg_controllen = sizeof(ctl);
    c = CMSG_FIRSTHDR(&mh);
    c->cmsg_level = IPPROTO_IPV6;
    c->cmsg_type = IPV6_PKTINFO;
    c->cmsg_len = CMSG_LEN(sizeof(pi));
    bzero(&pi, sizeof(pi));
    pi.ipi6_addr = linklocal;
    memcpy(CMSG_DATA(c), &pi, sizeof(pi));
    CU_ASSERT_EQUAL(sendmsg(s, &mh, 0), 1);
    CU_ASSERT_EQUAL(recv_from(r, &from), 1);
    CU_ASSERT(memcmp(&from.sin6_addr, &linklocal, sizeof(linklocal)) == 0);
    CloseSocket(r);
    CloseSocket(s);
}

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s, i, n, found = 0;

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) != 0) {
        CloseSocket(s);
        return 0;
    }
    ifindex = ifr.ifr_index;
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        n = ifc.ifc6_len / sizeof(req[0]);
        for (i = 0; i < n; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                linklocal = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    return found;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SENDTO6 tests\n");
        return 0;
    }
    have_ll = setup();
    if (!have_ll)
        printf("%s: no link-local address - skipping the SENDTO6 tests that need one\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SENDTO6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "sendto() leaves the socket unbound", testSENDTO6_STAYS_UNBOUND) ||
        !CU_add_test(p, "after sendto() the socket receives on its other addresses", testSENDTO6_STILL_RECEIVES) ||
        !CU_add_test(p, "an IPV6_PKTINFO source is used", testSENDTO6_PKTINFO_SOURCE))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SENDTO6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
