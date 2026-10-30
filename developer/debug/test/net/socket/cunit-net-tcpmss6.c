/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the TCP maximum segment size over IPv6: a segment and
    its IPv6 and TCP headers (60 bytes, RFC 6691) must fit the link MTU, so
    the MSS of a connection over an interface is at most its MTU less 60.
    Read with getsockopt(TCP_MAXSEG) on both ends of connections to the
    interface's own link-local and unique local addresses.

    The test adds fd00:4d54:68::1/64 to the interface. Usage:
    cunit-net-tcpmss6 [IFNAME]  (default eth0). Without the interface the
    tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <netinet/in_var.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static int mtu;
static struct in6_addr linklocal, ula;
static int ready;

#define ULA "fd00:4d54:68::1"

static void sa6(struct sockaddr_in6 *a, const struct in6_addr *addr, unsigned short port, unsigned scope)
{
    bzero(a, sizeof(*a));
    a->sin6_len = sizeof(*a);
    a->sin6_family = AF_INET6;
    if (addr)
        a->sin6_addr = *addr;
    a->sin6_port = port;
    a->sin6_scope_id = scope;
}

static int addr6(int add, const char *addr)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r;
    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;
        int i;

        bzero(&ifra, sizeof(ifra));
        strncpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name) - 1);
        sa6(&ifra.ifra_addr, NULL, 0, 0);
        inet_pton(AF_INET6, (char *)addr, &ifra.ifra_addr.sin6_addr);
        sa6(&ifra.ifra_prefixmask, NULL, 0, 0);
        for (i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;
        bzero(&ifr, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        sa6(&ifr.ifr_ifru.ifru_addr, NULL, 0, 0);
        inet_pton(AF_INET6, (char *)addr, &ifr.ifr_ifru.ifru_addr.sin6_addr);
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* Connect to [addr]:port%scope from a listener on [::]; *client_mss and
 * *server_mss get both ends' TCP_MAXSEG. */
static int pair_mss(const struct in6_addr *addr, unsigned scope, int *client_mss, int *server_mss)
{
    struct sockaddr_in6 a;
    socklen_t len = sizeof(a);
    int l = socket(AF_INET6, SOCK_STREAM, 0), c = -1, s = -1, r = -1;

    if (l < 0)
        return -1;
    sa6(&a, NULL, 0, 0);
    if (bind(l, (struct sockaddr *)&a, sizeof(a)) == 0 && listen(l, 1) == 0 &&
            getsockname(l, (struct sockaddr *)&a, &len) == 0) {
        unsigned short port = a.sin6_port;
        c = socket(AF_INET6, SOCK_STREAM, 0);
        sa6(&a, addr, port, scope);
        if (c >= 0 && connect(c, (struct sockaddr *)&a, sizeof(a)) == 0 && sock_wait(l, 0, 2) > 0 &&
                (s = accept(l, NULL, NULL)) >= 0) {
            socklen_t ol = sizeof(int);
            r = getsockopt(c, IPPROTO_TCP, TCP_MAXSEG, client_mss, &ol);
            ol = sizeof(int);
            if (r == 0)
                r = getsockopt(s, IPPROTO_TCP, TCP_MAXSEG, server_mss, &ol);
        }
    }
    if (s >= 0)
        CloseSocket(s);
    if (c >= 0)
        CloseSocket(c);
    CloseSocket(l);
    return r;
}

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* Over the interface's link-local address both ends' MSS fits the MTU. */
void testTCPMSS6_LINKLOCAL(void)
{
    int cm = 0, sm = 0;
    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(pair_mss(&linklocal, ifindex, &cm, &sm), 0);
    CU_ASSERT(cm > 0 && cm <= mtu - 60);
    CU_ASSERT(sm > 0 && sm <= mtu - 60);
}

/* ...and over its unique local address. */
void testTCPMSS6_ULA(void)
{
    int cm = 0, sm = 0;
    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(pair_mss(&ula, 0, &cm, &sm), 0);
    CU_ASSERT(cm > 0 && cm <= mtu - 60);
    CU_ASSERT(sm > 0 && sm <= mtu - 60);
}

static int setup(void)
{
    struct ifreq ifr;
    struct in6_ifreq req[16];
    struct in6_ifconf ifc;
    int s, found = 0;

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
    if (IoctlSocket(s, SIOCGIFMTU, (char *)&ifr) == 0)
        mtu = ifr.ifr_mtu;
    bzero(&ifc, sizeof(ifc));
    ifc.ifc6_len = sizeof(req);
    ifc.ifc6_req = req;
    if (IoctlSocket(s, SIOCGLIFADDR, (char *)&ifc) == 0) {
        int i, n = ifc.ifc6_len / sizeof(req[0]);
        for (i = 0; i < n; i++)
            if (strcmp(req[i].ifr_name, ifname) == 0 &&
                    IN6_IS_ADDR_LINKLOCAL(&req[i].ifr_ifru.ifru_addr.sin6_addr)) {
                linklocal = req[i].ifr_ifru.ifru_addr.sin6_addr;
                found = 1;
            }
    }
    CloseSocket(s);
    if (!found || mtu < 1280 || addr6(1, ULA) != 0)
        return 0;
    inet_pton(AF_INET6, ULA, &ula);
    Delay(3 * 50);      /* the new address is tentative until DAD completes */
    return 1;
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping TCPMSS6 tests\n");
        return 0;
    }
    ready = setup();
    if (!ready)
        printf("%s: no interface with a link-local address - skipping TCPMSS6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("TCPMSS6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "TCP MSS over the link-local address fits the MTU", testTCPMSS6_LINKLOCAL) ||
        !CU_add_test(p, "TCP MSS over a unique local address fits the MTU", testTCPMSS6_ULA))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-TCPMSS6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (ready)
        addr6(0, ULA);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
