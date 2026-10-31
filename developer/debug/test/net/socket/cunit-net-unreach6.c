/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the ICMPv6 Destination Unreachable, port unreachable,
    that a UDP datagram to a port with no socket gets (RFC 4443 section
    3.1): it carries the start of the datagram, and comes from the address
    the datagram was sent to (section 2.2); a datagram to a group gets none
    (section 2.4); and a connected socket learns of it as ECONNREFUSED.

    The errors are read from a raw ICMPv6 socket. The datagrams go to ::1
    and, if the interface has one, to its own link-local address and to
    ff02::1 on it (on hosted AROS, tap.device attached to a host tap).
    Usage:  cunit-net-unreach6 [IFNAME]  (default eth0). Without such an
    interface those tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int have_if;

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, int port, unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_port = htons(port);
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_scope_id = scope;
}

/* A UDP port nobody has. */
static int free_port(void)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0), port = -1;

    if (s < 0)
        return -1;
    sa6(&a, NULL, 0, 0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) == 0 &&
            getsockname(s, (struct sockaddr *)&a, &len) == 0)
        port = ntohs(a.sin6_port);
    CloseSocket(s);
    return port;
}

/* A raw ICMPv6 socket that sees only Destination Unreachable. */
static int unreach_socket(void)
{
    struct icmp6_filter f;
    int s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

    if (s < 0)
        return -1;
    ICMP6_FILTER_SETBLOCKALL(&f);
    ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &f);
    setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &f, sizeof(f));
    return s;
}

/*
 * Send a datagram to dst%scope, port - again each second, as UDP may lose
 * one - and wait for a Destination Unreachable about it. Returns its code
 * (-1 if none came within secs), with *from its source.
 */
static int unreach_for(const struct in6_addr *dst, unsigned scope, int port, int secs,
                       struct in6_addr *from)
{
    struct sockaddr_in6 d;
    int r = unreach_socket(), u = socket(AF_INET6, SOCK_DGRAM, 0), code = -1, tick;

    if (r < 0 || u < 0) {
        if (r >= 0)
            CloseSocket(r);
        if (u >= 0)
            CloseSocket(u);
        return -1;
    }
    if (scope)
        setsockopt(u, IPPROTO_IPV6, IPV6_MULTICAST_IF, &scope, sizeof(scope));
    sa6(&d, dst, port, scope);
    for (tick = 0; tick < secs * 10 && code < 0; tick++) {
        unsigned char buf[256];
        struct sockaddr_in6 f;
        socklen_t flen = sizeof(f);
        int n;

        if (tick % 10 == 0)
            sendto(u, "probe", 5, 0, (struct sockaddr *)&d, sizeof(d));
        if (sock_wait(r, 0, 0) <= 0) {
            struct timeval tv = { 0, 100000 };
            WaitSelect(0, NULL, NULL, NULL, &tv, NULL);
            continue;
        }
        n = recvfrom(r, (char *)buf, sizeof(buf), 0, (struct sockaddr *)&f, &flen);
        /* ICMPv6 header, then the datagram: IPv6 header, UDP header */
        if (n >= 8 + 40 + 8 && buf[0] == ICMP6_DST_UNREACH && buf[8 + 6] == IPPROTO_UDP &&
                memcmp(buf + 8 + 24, dst, 16) == 0 &&
                ((buf[8 + 40 + 2] << 8) | buf[8 + 40 + 3]) == port) {
            code = buf[1];
            *from = f.sin6_addr;
        }
    }
    CloseSocket(u);
    CloseSocket(r);
    return code;
}

#define NEED_IF() do { if (!have_if) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* To ::1: port unreachable, from ::1. */
void testUNREACH6_LOOPBACK(void)
{
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT, from;
    int port = free_port();

    CU_ASSERT_FATAL(port > 0);
    CU_ASSERT_EQUAL(unreach_for(&lo, 0, port, 3, &from), ICMP6_DST_UNREACH_NOPORT);
    CU_ASSERT(IN6_ARE_ADDR_EQUAL(&from, &lo));
}

/* A connected socket gets ECONNREFUSED. */
void testUNREACH6_REFUSED(void)
{
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct sockaddr_in6 d;
    long on = 1;
    char buf[8];
    int port = free_port(), c, i, err = 0;

    CU_ASSERT_FATAL(port > 0);
    c = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    sa6(&d, &lo, port, 0);
    CU_ASSERT_EQUAL(connect(c, (struct sockaddr *)&d, sizeof(d)), 0);
    IoctlSocket(c, FIONBIO, (char *)&on);
    CU_ASSERT_EQUAL(send(c, "probe", 5, 0), 5);
    for (i = 0; i < 20 && err == 0; i++) {
        struct timeval tv = { 0, 100000 };

        WaitSelect(0, NULL, NULL, NULL, &tv, NULL);
        if (recv(c, buf, sizeof(buf), 0) < 0 && Errno() != EWOULDBLOCK)
            err = Errno();
    }
    CloseSocket(c);
    CU_ASSERT_EQUAL(err, ECONNREFUSED);
}

/* To our own link-local address: from that address. */
void testUNREACH6_LINKLOCAL(void)
{
    struct in6_addr from;
    int port;
    NEED_IF();
    port = free_port();
    CU_ASSERT_FATAL(port > 0);
    CU_ASSERT_EQUAL(unreach_for(&linklocal, ifindex, port, 3, &from), ICMP6_DST_UNREACH_NOPORT);
    CU_ASSERT(IN6_ARE_ADDR_EQUAL(&from, &linklocal));
}

/* To a group: no error. */
void testUNREACH6_GROUP(void)
{
    struct in6_addr all, from;
    int port;
    NEED_IF();
    port = free_port();
    CU_ASSERT_FATAL(port > 0);
    inet_pton(AF_INET6, "ff02::1", &all);
    CU_ASSERT_EQUAL(unreach_for(&all, ifindex, port, 1, &from), -1);
}

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), found = 0;

    if (s < 0)
        return 0;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0) {
        ifindex = ifr.ifr_index;
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
        printf("bsdsocket.library not available - skipping UNREACH6 tests\n");
        return 0;
    }
    have_if = setup();
    if (!have_if)
        printf("%s: no interface with a link-local address - skipping its UNREACH6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("UNREACH6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a closed port on ::1 is unreachable, from ::1", testUNREACH6_LOOPBACK) ||
        !CU_add_test(p, "a connected socket to a closed port gets ECONNREFUSED", testUNREACH6_REFUSED) ||
        !CU_add_test(p, "a closed port on our link-local is unreachable, from it", testUNREACH6_LINKLOCAL) ||
        !CU_add_test(p, "a closed port on a group gets no error", testUNREACH6_GROUP))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-UNREACH6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
