/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for ICMPv6 error messages (RFC 4443) arriving at the stack:
    one goes to the socket the quoted packet came from, and none of them
    takes the stack down (they used to reach the IPv4 handlers of every
    protocol with no address, and one dereferenced it).

    The test sends the errors itself, from a raw ICMPv6 socket to the
    interface's own link-local address, so they loop back and are received
    on that interface without going out on the link. Each quotes a UDP
    datagram from the link-local to PEER, which need not exist.

    Usage:  cunit-net-icmp6err [IFNAME]  (default eth0). Without an
    interface with a link-local address the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;

#define PEER        "fe80::4d54:65"
#define PEER_PORT   9

static int index_of(const char *name, unsigned *index)
{
    struct ifreq ifr;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
    r = IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr);
    CloseSocket(s);
    if (r != 0)
        return -1;
    *index = ifr.ifr_index;
    return 0;
}

/* The interface's link-local address, into *ll. */
static int find_linklocal(struct in6_addr *ll)
{
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s = socket(AF_INET6, SOCK_DGRAM, 0), i, n, found = 0;

    if (s < 0)
        return 0;
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        n = ifc.ifc6_len / sizeof(req[0]);
        for (i = 0; i < n && !found; i++) {
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                *ll = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
        }
    }
    CloseSocket(s);
    return found;
}

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, int port)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    a->sin6_addr = *addr;
    a->sin6_port = htons(port);
    a->sin6_scope_id = ifindex;
}

/* A UDP socket connected to PEER; its local port into *lport. */
static int connected(int *lport)
{
    struct sockaddr_in6 a;
    struct in6_addr peer;
    socklen_t len = sizeof(a);
    int s = socket(AF_INET6, SOCK_DGRAM, 0);

    if (s < 0)
        return -1;
    inet_pton(AF_INET6, PEER, &peer);
    sa6(&a, &peer, PEER_PORT);
    if (connect(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            getsockname(s, (struct sockaddr *)&a, &len) != 0) {
        CloseSocket(s);
        return -1;
    }
    *lport = ntohs(a.sin6_port);
    return s;
}

/* An ICMPv6 error of type/code (word: the MTU or pointer) quoting a UDP
 * datagram from our link-local, port sport, to PEER, port dport. */
static int send_error(int type, int code, u_int32_t word, int sport, int dport)
{
    u_char pkt[sizeof(struct icmp6_hdr) + sizeof(struct ip6_hdr) + 8];
    struct icmp6_hdr *ic = (struct icmp6_hdr *)pkt;
    struct ip6_hdr *ip6 = (struct ip6_hdr *)(ic + 1);
    u_int16_t *udp = (u_int16_t *)(ip6 + 1);
    struct sockaddr_in6 a;
    int s, n;

    bzero(pkt, sizeof(pkt));
    ic->icmp6_type = type;
    ic->icmp6_code = code;
    ic->icmp6_data32[0] = htonl(word);
    ip6->ip6_flow = htonl(0x60000000);
    ip6->ip6_plen = htons(8);
    ip6->ip6_nxt = IPPROTO_UDP;
    ip6->ip6_hlim = 64;
    ip6->ip6_src = linklocal;
    inet_pton(AF_INET6, PEER, &ip6->ip6_dst);
    udp[0] = htons(sport);
    udp[1] = htons(dport);
    udp[2] = htons(8);

    s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (s < 0)
        return -1;
    sa6(&a, &linklocal, 0);
    n = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    return n == (int)sizeof(pkt) ? 0 : -1;
}

/* The error the socket reports within secs, or 0 for none. */
static int pending_error(int s, int secs)
{
    char buf[16];

    if (sock_wait(s, 0, secs) <= 0)
        return 0;
    if (recv(s, buf, sizeof(buf), 0) >= 0)
        return 0;
    return Errno();
}

#define NEED_IF() do { if (!ready) { CU_PASS("no interface with a link-local address - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A Port Unreachable about a connected UDP socket's datagram reaches it
 * as ECONNREFUSED. */
void testICMP6ERR_PORT_UNREACH(void)
{
    int s, lport;
    NEED_IF();
    s = connected(&lport);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0,
                               lport, PEER_PORT), 0);
    CU_ASSERT_EQUAL(pending_error(s, 2), ECONNREFUSED);
    CloseSocket(s);
}

/* ...but not one about a datagram to another port. */
void testICMP6ERR_OTHER_PORT(void)
{
    int s, lport;
    NEED_IF();
    s = connected(&lport);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0,
                               lport, PEER_PORT + 1), 0);
    CU_ASSERT_EQUAL(pending_error(s, 1), 0);
    CloseSocket(s);
}

/* A Packet Too Big only updates the path MTU; the socket sees no error. */
void testICMP6ERR_PACKET_TOO_BIG(void)
{
    int s, lport;
    NEED_IF();
    s = connected(&lport);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_PACKET_TOO_BIG, 0, 1280, lport, PEER_PORT), 0);
    CU_ASSERT_EQUAL(pending_error(s, 1), 0);
    CloseSocket(s);
}

/* Every error type and code is taken in, and the stack is still there. */
void testICMP6ERR_ALL_KINDS(void)
{
    int code, s, lport;
    NEED_IF();
    for (code = 0; code <= 7; code++)
        CU_ASSERT_EQUAL(send_error(ICMP6_DST_UNREACH, code, 0, 1, PEER_PORT), 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_PACKET_TOO_BIG, 0, 1280, 1, PEER_PORT), 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_TRANSIT, 0, 1, PEER_PORT), 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_REASSEMBLY, 0, 1, PEER_PORT), 0);
    CU_ASSERT_EQUAL(send_error(ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, 6, 1, PEER_PORT), 0);
    Delay(25);
    s = connected(&lport);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CloseSocket(s);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ICMP6ERR tests\n");
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && find_linklocal(&linklocal);
    if (!ready)
        printf("%s: no interface with a link-local address - skipping ICMP6ERR tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ICMP6ERR_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a Packet Too Big is no error for the socket", testICMP6ERR_PACKET_TOO_BIG) ||
        !CU_add_test(p, "a Port Unreachable reaches the socket it is about", testICMP6ERR_PORT_UNREACH) ||
        !CU_add_test(p, "a Port Unreachable for another port does not", testICMP6ERR_OTHER_PORT) ||
        !CU_add_test(p, "every error kind is taken in, the stack stays up", testICMP6ERR_ALL_KINDS))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ICMP6ERR");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
