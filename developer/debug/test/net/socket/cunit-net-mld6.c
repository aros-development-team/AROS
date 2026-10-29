/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for bsdsocket.library IPv6 multicast group membership as seen
    on the wire: every join must be announced with an MLD Report and every
    release with a Done (RFC 2710 sections 4, 5; RFC 3810 section 6.1),
    whoever owns the membership:
      - a socket's IPV6_JOIN_GROUP/IPV6_LEAVE_GROUP, and closing the socket;
      - an address's solicited-node group (RFC 4861 section 7.2.1), joined
        when the address is added and left when it is deleted.

    The test watches its own outgoing MLD with the bsdsocket BPF interface,
    so it needs no peer, only a multicast-capable interface (on hosted AROS,
    tap.device attached to a host tap). Usage:  cunit-net-mld6 [IFNAME]
    (default eth0). Without such an interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* the Roadshow bpf_*() calls in bsdsocket.library */
#include "socktest.h"

#include <proto/dos.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <net/bpf.h>
#include <stdlib.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static long bpf = -1;

#define MLD_REPORT   131    /* MLDv1 Listener Report */
#define MLD_DONE     132    /* MLDv1 Listener Done */
#define MLD_V2REPORT 143    /* MLDv2 Listener Report */
#define MODE_TO_IN   3      /* MLDv2 record CHANGE_TO_INCLUDE: leaving */

enum { EV_NONE, EV_REPORT, EV_LEAVE };

/* --------------------------------------------------------------- capture */

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

/* Parse one captured packet; if it is an MLD message about group g, return
 * EV_REPORT or EV_LEAVE. The stack hands BPF the IPv6 packet itself (the
 * SANA-II driver builds the link header), although BIOCGDLT says
 * DLT_EN10MB; an Ethernet frame is accepted as well. */
static int mld_event(const u_char *f, u_int len, const struct in6_addr *g)
{
    const u_char *p, *end = f + len;
    u_int nxt;

    if (len >= 40 && (f[0] >> 4) == 6)
        p = f;
    else if (len >= 14 + 40 && f[12] == 0x86 && f[13] == 0xdd)
        p = f + 14;
    else
        return EV_NONE;
    if (p + 40 > end)
        return EV_NONE;
    nxt = p[6];
    p += 40;
    if (nxt == 0) {                     /* Hop-by-Hop (Router Alert) */
        if (p + 8 > end)
            return EV_NONE;
        nxt = p[0];
        p += (p[1] + 1) * 8;
    }
    if (nxt != 58 || p + 24 > end)      /* ICMPv6 */
        return EV_NONE;
    if (p[0] == MLD_REPORT || p[0] == MLD_DONE) {
        if (memcmp(p + 8, g, 16) != 0)
            return EV_NONE;
        return p[0] == MLD_REPORT ? EV_REPORT : EV_LEAVE;
    }
    if (p[0] == MLD_V2REPORT) {
        u_int n = (p[6] << 8) | p[7];
        const u_char *r = p + 8;
        while (n-- > 0 && r + 20 <= end) {
            u_int nsrc = (r[2] << 8) | r[3];
            if (memcmp(r + 4, g, 16) == 0)
                return r[0] == MODE_TO_IN && nsrc == 0 ? EV_LEAVE : EV_REPORT;
            r += 20 + r[1] * 4 + nsrc * 16;
        }
    }
    return EV_NONE;
}

/* Wait up to secs for an MLD message of the given kind about group g. */
static int mld_wait(const char *group, int kind, int secs)
{
    struct in6_addr g;
    char buf[BPF_DFLTBUFSIZE];
    int ticks;

    inet_pton(AF_INET6, (char *)group, &g);
    for (ticks = 0; ticks < secs * 10; ticks++) {
        long n;
        while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
            char *p = buf;
            while (p < buf + n) {
                struct bpf_hdr *h = (struct bpf_hdr *)p;
                if (mld_event((u_char *)p + h->bh_hdrlen, h->bh_caplen, &g) == kind)
                    return 1;
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
    return 0;
}

/* --------------------------------------------------------------- helpers */

static int mc_socket(void)
{
    int on = 1, s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s >= 0)
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    return s;
}

static int mc_group(int s, int opt, const char *group)
{
    struct ipv6_mreq mr;
    bzero(&mr, sizeof(mr));
    inet_pton(AF_INET6, (char *)group, &mr.ipv6mr_multiaddr);
    mr.ipv6mr_interface = ifindex;
    return setsockopt(s, IPPROTO_IPV6, opt, &mr, sizeof(mr));
}

static int addr6(int add, const char *addr)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r, i;
    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;
        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        ifra.ifra_addr.sin6_len = sizeof(ifra.ifra_addr);
        ifra.ifra_addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, (char *)addr, &ifra.ifra_addr.sin6_addr);
        ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
        ifra.ifra_prefixmask.sin6_family = AF_INET6;
        for (i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
        ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, (char *)addr, &ifr.ifr_ifru.ifru_addr.sin6_addr);
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

#define NEED_IF() do { if (bpf < 0) { CU_PASS("no multicast interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* Control: an explicit join is reported, an explicit leave sends Done. */
void testMLD6_JOIN_LEAVE(void)
{
    int s;
    NEED_IF();
    s = mc_socket();
    CU_ASSERT_EQUAL(mc_group(s, IPV6_JOIN_GROUP, "ff12::4d54:21"), 0);
    CU_ASSERT(mld_wait("ff12::4d54:21", EV_REPORT, 3));
    CU_ASSERT_EQUAL(mc_group(s, IPV6_LEAVE_GROUP, "ff12::4d54:21"), 0);
    CU_ASSERT(mld_wait("ff12::4d54:21", EV_LEAVE, 3));
    CloseSocket(s);
}

/* Closing a socket releases the groups it joined. */
void testMLD6_CLOSE_LEAVES(void)
{
    int s;
    NEED_IF();
    s = mc_socket();
    CU_ASSERT_EQUAL(mc_group(s, IPV6_JOIN_GROUP, "ff12::4d54:22"), 0);
    CU_ASSERT(mld_wait("ff12::4d54:22", EV_REPORT, 3));
    CloseSocket(s);
    CU_ASSERT(mld_wait("ff12::4d54:22", EV_LEAVE, 3));
}

/* After a socket closed, joining its group again is a new membership. */
void testMLD6_REJOIN_AFTER_CLOSE(void)
{
    int s;
    NEED_IF();
    s = mc_socket();
    mc_group(s, IPV6_JOIN_GROUP, "ff12::4d54:23");
    mld_wait("ff12::4d54:23", EV_REPORT, 3);
    CloseSocket(s);
    Delay(50);
    s = mc_socket();
    CU_ASSERT_EQUAL(mc_group(s, IPV6_JOIN_GROUP, "ff12::4d54:23"), 0);
    CU_ASSERT(mld_wait("ff12::4d54:23", EV_REPORT, 3));
    mc_group(s, IPV6_LEAVE_GROUP, "ff12::4d54:23");
    CloseSocket(s);
}

/* An address added by hand joins its solicited-node group... */
void testMLD6_ADDR_JOINS_SOLICITED(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(1, "fd00:4d54::21:3458"), 0);
    CU_ASSERT(mld_wait("ff02::1:ff21:3458", EV_REPORT, 3));
}

/* ...and leaves it when it is deleted. */
void testMLD6_ADDR_LEAVES_SOLICITED(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(addr6(0, "fd00:4d54::21:3458"), 0);
    CU_ASSERT(mld_wait("ff02::1:ff21:3458", EV_LEAVE, 3));
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
        printf("bsdsocket.library not available - skipping MLD6 tests\n");
        return 0;
    }

    /* a multicast-capable interface, watched with BPF */
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (s >= 0 && IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
            (ifr.ifr_flags & IFF_MULTICAST) && (ifr.ifr_flags & IFF_UP)) {
        if (IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
            ifindex = ifr.ifr_index;
        if (ifindex == 0 || bpf_start() != 0)
            printf("%s: no index or BPF - skipping MLD6 tests\n", ifname);
    } else {
        printf("%s: no multicast interface - skipping MLD6 tests\n", ifname);
    }
    if (s >= 0)
        CloseSocket(s);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MLD6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "join/leave send Report/Done", testMLD6_JOIN_LEAVE) ||
        !CU_add_test(p, "closing a socket leaves its groups", testMLD6_CLOSE_LEAVES) ||
        !CU_add_test(p, "rejoin after close is reported", testMLD6_REJOIN_AFTER_CLOSE) ||
        !CU_add_test(p, "added address joins solicited-node", testMLD6_ADDR_JOINS_SOLICITED) ||
        !CU_add_test(p, "deleted address leaves solicited-node", testMLD6_ADDR_LEAVES_SOLICITED))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-MLD6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
