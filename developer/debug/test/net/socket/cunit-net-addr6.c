/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the life of an IPv6 address added by hand. While it
    exists its prefix is on-link through a route, and deleting it takes that
    route away; before it is used it goes through duplicate address detection
    (RFC 4862 section 5.4) - a Neighbor Solicitation for it from the
    unspecified address - and adding it does not make the host solicit routers
    again (RFC 4861 section 6.3.7: that is for an interface coming up).
    Deleting an address while it is still being checked stops the check.

    Routes are read with RTM_GET on a routing socket; the solicitations are
    watched with the bsdsocket BPF calls. The test needs a multicast-capable
    interface (on hosted AROS, tap.device attached to a host tap).
    Usage:  cunit-net-addr6 [IFNAME]  (default eth0). Without such an
    interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/route.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static int ready;
static long bpf = -1;

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

static void sa6(struct sockaddr_in6 *a, const char *addr)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
}

/* Whether the table has a route for dst (of any kind), via RTM_GET. */
static int has_route(const char *dst)
{
    union { struct rt_msghdr h; char b[512]; } msg;
    struct sockaddr_in6 *sa;
    int s, n = 0;

    s = socket(PF_ROUTE, SOCK_RAW, 0);
    if (s < 0)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.h.rtm_version = RTM_VERSION;
    msg.h.rtm_type = RTM_GET;
    msg.h.rtm_addrs = RTA_DST;
    msg.h.rtm_seq = 3;
    sa = (struct sockaddr_in6 *)(&msg.h + 1);
    sa6(sa, dst);
    msg.h.rtm_msglen = sizeof(msg.h) + ROUNDUP(sizeof(*sa));
    if (send(s, (char *)&msg, msg.h.rtm_msglen, 0) < 0) {
        CloseSocket(s);
        return 0;
    }
    while (sock_wait(s, 0, 2) > 0) {
        n = recv(s, (char *)&msg, sizeof(msg), 0);
        if (n <= 0 || (msg.h.rtm_type == RTM_GET && msg.h.rtm_seq == 3))
            break;
    }
    CloseSocket(s);
    return n > 0 && msg.h.rtm_errno == 0;
}

/* Add or delete addr/64 on the interface. */
static int addr6(int add, const char *addr)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r, i;
    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;
        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        sa6(&ifra.ifra_addr, addr);
        sa6(&ifra.ifra_prefixmask, NULL);
        for (i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        sa6(&ifr.ifr_ifru.ifru_addr, addr);
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
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

/* Watch outgoing ICMPv6 for secs: count Neighbor Solicitations for target
 * from the unspecified address, and Router Solicitations. */
static void watch(const char *target, int secs, int *dad_ns, int *rs)
{
    static const u_char unspec[16];
    struct in6_addr want;
    char buf[BPF_DFLTBUFSIZE];
    int ticks;

    *dad_ns = *rs = 0;
    if (target)
        inet_pton(AF_INET6, (char *)target, &want);
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long n;
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                const u_char *f = (u_char *)p + h->bh_hdrlen, *ip = NULL;
                u_int len = h->bh_caplen;

                if (len >= 14 + 40 && f[12] == 0x86 && f[13] == 0xdd)
                    ip = f + 14;
                else if (len >= 40 && (f[0] >> 4) == 6)
                    ip = f;
                if (ip && ip + 40 + 24 <= f + len && ip[6] == IPPROTO_ICMPV6) {
                    if (ip[40] == ND_ROUTER_SOLICIT)
                        (*rs)++;
                    else if (ip[40] == ND_NEIGHBOR_SOLICIT && target &&
                            memcmp(ip + 8, unspec, 16) == 0 &&
                            memcmp(ip + 40 + 8, &want, 16) == 0)
                        (*dad_ns)++;
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* Deleting an address takes the route to its prefix with it. */
void testADDR6_DELETE_REMOVES_PREFIX_ROUTE(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, "fd00:4d54:51::1"), 0);
    Delay(50);
    CU_ASSERT(has_route("fd00:4d54:51::2"));
    CU_ASSERT_EQUAL(addr6(0, "fd00:4d54:51::1"), 0);
    CU_ASSERT(!has_route("fd00:4d54:51::2"));
}

/* A new address is checked for duplicates before it is used. */
void testADDR6_DAD(void)
{
    int ns, rs;
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, "fd00:4d54:52::1"), 0);
    watch("fd00:4d54:52::1", 3, &ns, &rs);
    CU_ASSERT(ns >= 1);
    addr6(0, "fd00:4d54:52::1");
}

/* An address deleted while it is still being checked is not probed for
 * any more: duplicate address detection stops with it. */
void testADDR6_DELETE_STOPS_DAD(void)
{
    int ns, rs;
    char buf[BPF_DFLTBUFSIZE];
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, "fd00:4d54:54::1"), 0);
    Delay(10);
    CU_ASSERT_EQUAL(addr6(0, "fd00:4d54:54::1"), 0);
    while (bpf_read(bpf, buf, sizeof(buf)) > 0)
        ;
    watch("fd00:4d54:54::1", 3, &ns, &rs);
    CU_ASSERT_EQUAL(ns, 0);
}

/* Adding an address does not make the host solicit routers. */
void testADDR6_NO_ROUTER_SOLICITATION(void)
{
    int ns, rs;
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, "fd00:4d54:53::1"), 0);
    watch(NULL, 4, &ns, &rs);
    CU_ASSERT_EQUAL(rs, 0);
    addr6(0, "fd00:4d54:53::1");
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
        printf("bsdsocket.library not available - skipping ADDR6 tests\n");
        return 0;
    }
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (s >= 0 && IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
            (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP) &&
            bpf_start() == 0)
        ready = 1;
    else
        printf("%s: no multicast interface or no BPF - skipping ADDR6 tests\n", ifname);
    if (s >= 0)
        CloseSocket(s);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ADDR6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "deleting an address removes its prefix route", testADDR6_DELETE_REMOVES_PREFIX_ROUTE) ||
        !CU_add_test(p, "a new address goes through DAD", testADDR6_DAD) ||
        !CU_add_test(p, "deleting an address stops its DAD", testADDR6_DELETE_STOPS_DAD) ||
        !CU_add_test(p, "adding an address solicits no routers", testADDR6_NO_ROUTER_SOLICITATION))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ADDR6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
