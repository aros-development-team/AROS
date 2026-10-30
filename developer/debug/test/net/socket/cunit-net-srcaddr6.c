/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the source address the stack chooses on an interface that
    has a link-local and a wider-scope address. A destination of link scope
    gets the link-local source and a global destination a global one
    (RFC 6724 section 5, rule 2); MLD messages always come from the
    link-local address, whatever the group's scope (RFC 3810 section 5).

    The test adds fd00:4d54:61::1/64 to the interface. Sources are read with
    getsockname() after connect(); MLD reports are watched with the
    bsdsocket BPF calls. It needs a multicast-capable interface (on hosted
    AROS, tap.device attached to a host tap). Usage:  cunit-net-srcaddr6
    [IFNAME]  (default eth0). Without such an interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() and the bpf_*() calls */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <net/bpf.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal, ula;
static int ready;
static long bpf = -1;

#define ULA "fd00:4d54:61::1"

static void sa6(struct sockaddr_in6 *a, const char *addr)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        inet_pton(AF_INET6, (char *)addr, &a->sin6_addr);
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


/* The source the stack picks for dst: connect() a datagram socket, then
 * getsockname(). */
static int source_for(const char *dst, unsigned scope, struct in6_addr *src)
{
    struct sockaddr_in6 d, l;
    socklen_t len = sizeof(l);
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r = -1;

    if (s < 0)
        return -1;
    sa6(&d, dst);
    d.sin6_port = htons(9);
    d.sin6_scope_id = scope;
    if (connect(s, (struct sockaddr *)&d, sizeof(d)) == 0 &&
            getsockname(s, (struct sockaddr *)&l, &len) == 0) {
        *src = l.sin6_addr;
        r = 0;
    }
    CloseSocket(s);
    return r;
}

/* The source of the first MLD report about group seen within secs. */
static int mld_source(const char *group, int secs, struct in6_addr *src)
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
                const u_char *f = (u_char *)p + h->bh_hdrlen, *ip = NULL, *q, *end;
                u_int len = h->bh_caplen;

                if (len >= 14 + 40 && f[12] == 0x86 && f[13] == 0xdd)
                    ip = f + 14;
                else if (len >= 40 && (f[0] >> 4) == 6)
                    ip = f;
                end = f + len;
                if (ip && ip[6] == 0 && ip + 48 <= end) {       /* Hop-by-Hop */
                    q = ip + 40 + (ip[41] + 1) * 8;
                    if (ip[40] == IPPROTO_ICMPV6 && q + 24 <= end &&
                            ((q[0] == 131 && memcmp(q + 8, &g, 16) == 0) ||
                             (q[0] == 143 && q + 28 <= end && memcmp(q + 12, &g, 16) == 0))) {
                        memcpy(src, ip + 8, 16);
                        return 1;
                    }
                }
                p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
            }
        }
        Delay(5);
    }
    return 0;
}

static int join(int s, const char *group)
{
    struct ipv6_mreq mr;
    bzero(&mr, sizeof(mr));
    inet_pton(AF_INET6, (char *)group, &mr.ipv6mr_multiaddr);
    mr.ipv6mr_interface = ifindex;
    return setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mr, sizeof(mr));
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A link-local destination gets the link-local source. */
void testSRC6_LINKSCOPE_DESTINATION(void)
{
    struct in6_addr src;
    NEED_IF();
    CU_ASSERT_EQUAL(source_for("fe80::4d54:99", ifindex, &src), 0);
    CU_ASSERT(memcmp(&src, &linklocal, sizeof(src)) == 0);
}

/* A global destination gets the global source. */
void testSRC6_GLOBAL_DESTINATION(void)
{
    struct in6_addr src;
    NEED_IF();
    CU_ASSERT_EQUAL(source_for("fd00:4d54:61::2", 0, &src), 0);
    CU_ASSERT(memcmp(&src, &ula, sizeof(src)) == 0);
}

/* An MLD report for a link-scope group comes from the link-local address. */
void testSRC6_MLD_LINKSCOPE_GROUP(void)
{
    struct in6_addr src;
    int s;
    NEED_IF();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_EQUAL(join(s, "ff12::4d54:61"), 0);
    CU_ASSERT(mld_source("ff12::4d54:61", 3, &src));
    CU_ASSERT(memcmp(&src, &linklocal, sizeof(src)) == 0);
    CloseSocket(s);
}

/* ...and so does one for a group of wider scope. */
void testSRC6_MLD_WIDER_GROUP(void)
{
    struct in6_addr src;
    int s;
    NEED_IF();
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    CU_ASSERT_EQUAL(join(s, "ff15::4d54:61"), 0);
    CU_ASSERT(mld_source("ff15::4d54:61", 3, &src));
    CU_ASSERT(memcmp(&src, &linklocal, sizeof(src)) == 0);
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
    if (IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) != 0 ||
            !(ifr.ifr_flags & IFF_MULTICAST) || !(ifr.ifr_flags & IFF_UP) ||
            IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) != 0) {
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
    if (!found || bpf_start() != 0 || addr6(1, ULA) != 0)
        return 0;
    inet_pton(AF_INET6, ULA, &ula);
    Delay(3 * 50);      /* a new address is tentative until DAD completes */
    return 1;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SRC6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no multicast interface with a link-local address - skipping SRC6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SRC6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "a link-scope destination gets the link-local source", testSRC6_LINKSCOPE_DESTINATION) ||
        !CU_add_test(p, "a global destination gets a global source", testSRC6_GLOBAL_DESTINATION) ||
        !CU_add_test(p, "MLD for a link-scope group comes from the link-local", testSRC6_MLD_LINKSCOPE_GROUP) ||
        !CU_add_test(p, "MLD for a wider group comes from the link-local", testSRC6_MLD_WIDER_GROUP))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SRC6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (ready)
        addr6(0, ULA);
    if (bpf >= 0)
        bpf_close(bpf);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
