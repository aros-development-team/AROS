/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the delivery of IPv6 multicast to this host's own members
    of a group (RFC 3493 section 5.2): a datagram a program sends to a group
    on an interface reaches every socket on the host that listens on the
    group's port there, as long as the host is a member and the sender did not
    turn IPV6_MULTICAST_LOOP off; a group the host did not join is not
    delivered.

    The sender binds the interface's link-local address and names the
    interface with IPV6_MULTICAST_IF, so the test needs no peer, only a
    multicast-capable interface (on hosted AROS, tap.device attached to a host
    tap). Usage:  cunit-net-mcast6 [IFNAME]  (default eth0). Without such an
    interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() in bsdsocket.library */
#include "socktest.h"

#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;

#define PORT 40531

/* --------------------------------------------------------------- helpers */

static void group_addr(struct sockaddr_in6 *a, const char *group, int port)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_port = htons(port);
    a->sin6_scope_id = ifindex;
    inet_pton(AF_INET6, (char *)group, &a->sin6_addr);
}

/* A socket on [::]:PORT, sharing the port. */
static int listener(void)
{
    struct sockaddr_in6 a;
    int on = 1, s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_port = htons(PORT);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0) {
        CloseSocket(s);
        return -1;
    }
    return s;
}

static int join(int s, const char *group)
{
    struct ipv6_mreq mr;
    bzero(&mr, sizeof(mr));
    inet_pton(AF_INET6, (char *)group, &mr.ipv6mr_multiaddr);
    mr.ipv6mr_interface = ifindex;
    return setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mr, sizeof(mr));
}

/* A sender on the interface's link-local address, leaving by the interface. */
static int sender(int loop)
{
    struct sockaddr_in6 a;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = linklocal;
    a.sin6_scope_id = ifindex;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
        CloseSocket(s);
        return -1;
    }
    return s;
}

static int send_group(int s, const char *group, const char *msg)
{
    struct sockaddr_in6 g;
    group_addr(&g, group, PORT);
    return sendto(s, (char *)msg, strlen(msg), 0, (struct sockaddr *)&g, sizeof(g));
}

/* Whether msg arrives on s within secs; other datagrams are skipped. */
static int got(int s, const char *msg, int secs)
{
    char buf[64];
    int n;

    while (sock_wait(s, 0, secs) > 0) {
        n = recv(s, buf, sizeof(buf) - 1, 0);
        if (n < 0)
            return 0;
        buf[n] = 0;
        if (strcmp(buf, msg) == 0)
            return 1;
    }
    return 0;
}

/* The hop limit delivered with msg, or -1 if the message did not arrive
 * with IPV6_HOPLIMIT ancillary data. */
static int got_hlim(int s, const char *msg, int secs)
{
    union { struct cmsghdr c; char b[CMSG_SPACE(sizeof(int))]; } ctl;
    struct msghdr mh;
    struct iovec iov;
    struct cmsghdr *c;
    char buf[64];
    int n;

    while (sock_wait(s, 0, secs) > 0) {
        bzero(&mh, sizeof(mh));
        bzero(&ctl, sizeof(ctl));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf) - 1;
        mh.msg_iov = &iov;
        mh.msg_iovlen = 1;
        mh.msg_control = &ctl;
        mh.msg_controllen = sizeof(ctl);
        n = recvmsg(s, &mh, 0);
        if (n < 0)
            return -1;
        buf[n] = 0;
        if (strcmp(buf, msg) != 0)
            continue;
        for (c = CMSG_FIRSTHDR(&mh); c != NULL; c = CMSG_NXTHDR(&mh, c)) {
            if (c->cmsg_level == IPPROTO_IPV6 &&
                    c->cmsg_type == IPV6_HOPLIMIT &&
                    c->cmsg_len >= CMSG_LEN(sizeof(int))) {
                int hlim;
                memcpy(&hlim, CMSG_DATA(c), sizeof(hlim));
                return hlim;
            }
        }
    }
    return -1;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no multicast interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A member of a group receives a datagram this host sends to it. */
void testMCAST6_MEMBER_RECEIVES(void)
{
    int r, s;
    NEED_IF();
    r = listener();
    s = sender(1);
    CU_ASSERT(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(join(r, "ff12::4d54:31"), 0);
    CU_ASSERT(send_group(s, "ff12::4d54:31", "member") > 0);
    CU_ASSERT(got(r, "member", 2));
    CloseSocket(s);
    CloseSocket(r);
}

/* Every socket listening on the group's port gets its own copy. */
void testMCAST6_EVERY_SOCKET(void)
{
    int a, b, s;
    NEED_IF();
    a = listener();
    b = listener();
    s = sender(1);
    CU_ASSERT(a >= 0 && b >= 0 && s >= 0);
    CU_ASSERT_EQUAL(join(a, "ff12::4d54:32"), 0);
    CU_ASSERT_EQUAL(join(b, "ff12::4d54:32"), 0);
    CU_ASSERT(send_group(s, "ff12::4d54:32", "both") > 0);
    CU_ASSERT(got(a, "both", 2));
    CU_ASSERT(got(b, "both", 2));
    CloseSocket(s);
    CloseSocket(b);
    CloseSocket(a);
}

/* As for IPv4, a socket on the port receives a group the host is a member
 * of on that interface even if that socket did not join it itself. */
void testMCAST6_PORT_WITHOUT_MEMBERSHIP(void)
{
    int member, plain, s;
    NEED_IF();
    member = listener();
    plain = listener();
    s = sender(1);
    CU_ASSERT(member >= 0 && plain >= 0 && s >= 0);
    CU_ASSERT_EQUAL(join(member, "ff12::4d54:33"), 0);
    CU_ASSERT(send_group(s, "ff12::4d54:33", "port") > 0);
    CU_ASSERT(got(plain, "port", 2));
    CloseSocket(s);
    CloseSocket(plain);
    CloseSocket(member);
}

/* With IPV6_MULTICAST_LOOP 0 the datagram does not come back to the host. */
void testMCAST6_LOOP_OFF(void)
{
    int r, s;
    NEED_IF();
    r = listener();
    s = sender(0);
    CU_ASSERT(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(join(r, "ff12::4d54:34"), 0);
    CU_ASSERT(send_group(s, "ff12::4d54:34", "noloop") > 0);
    CU_ASSERT_FALSE(got(r, "noloop", 1));
    CloseSocket(s);
    CloseSocket(r);
}

/* A group nobody on the host joined is not delivered to the port. */
void testMCAST6_NOT_A_MEMBER(void)
{
    int r, s;
    NEED_IF();
    r = listener();
    s = sender(1);
    CU_ASSERT(r >= 0 && s >= 0);
    CU_ASSERT(send_group(s, "ff12::4d54:35", "nobody") > 0);
    CU_ASSERT_FALSE(got(r, "nobody", 1));
    CloseSocket(s);
    CloseSocket(r);
}

/* IPV6_MULTICAST_HOPS, rather than the unicast default, supplies the IPv6
 * header of a UDP group datagram. */
void testMCAST6_HOPS(void)
{
    int r, s, on = 1, hops = 255;
    NEED_IF();
    r = listener();
    s = sender(1);
    CU_ASSERT(r >= 0 && s >= 0);
    CU_ASSERT_EQUAL(join(r, "ff12::4d54:36"), 0);
    CU_ASSERT_EQUAL(setsockopt(r, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
                              &on, sizeof(on)), 0);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                              &hops, sizeof(hops)), 0);
    CU_ASSERT(send_group(s, "ff12::4d54:36", "hops") > 0);
    CU_ASSERT_EQUAL(got_hlim(r, "hops", 2), 255);
    CloseSocket(s);
    CloseSocket(r);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    struct ifreq ifr;
    int s;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping MCAST6 tests\n");
        return 0;
    }

    /* a multicast-capable interface with an index and a link-local address */
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (s >= 0 && IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
            (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
            IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0) {
        struct in6_ifreq req[16];
        struct in6_ifconf ifc;
        int i, n;

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
                    ready = 1;
                }
        }
    }
    if (!ready)
        printf("%s: no multicast interface with a link-local address - skipping MCAST6 tests\n", ifname);
    if (s >= 0)
        CloseSocket(s);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MCAST6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a group nobody joined is not delivered", testMCAST6_NOT_A_MEMBER) ||
        !CU_add_test(p, "a member receives a group datagram", testMCAST6_MEMBER_RECEIVES) ||
        !CU_add_test(p, "a socket on the port without a membership receives", testMCAST6_PORT_WITHOUT_MEMBERSHIP) ||
        !CU_add_test(p, "every socket on the port gets a copy", testMCAST6_EVERY_SOCKET) ||
        !CU_add_test(p, "IPV6_MULTICAST_HOPS applies to UDP", testMCAST6_HOPS) ||
        !CU_add_test(p, "IPV6_MULTICAST_LOOP 0 keeps it off the host", testMCAST6_LOOP_OFF))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-MCAST6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
