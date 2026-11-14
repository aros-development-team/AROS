/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the IPv6 neighbour cache's limits: entries per
    interface (net.inet6.icmp6.nd6_maxnbrs) sized from the memory there is,
    16 to 256; packets held per entry (nd6_maxqlen, 16) and by all entries
    together (nd6_maxheld) - past the last, the oldest packet held gives
    way, and its sender is not told when resolution fails (the senders of
    the packets still held are, RFC 4861 section 7.2.2). Each netvar refuses
    a value out of its range.

    Usage:  cunit-net-nd6limits [IFNAME]  (default eth0). The held packets'
    test needs IFNAME up with IPv6; it sends to link-local addresses nobody
    has. Without it that test is skipped.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <proto/miami.h>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <sys/sysctl.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;

#define NEED_IF() do { if (ifindex == 0) { CU_PASS("no IPv6 interface - skipping"); return; } } while (0)

int init_suite(void) { return SocketBase != NULL && MiamiBase != NULL ? 0 : -1; }
int clean_suite(void) { return 0; }

/* net.inet6.icmp6.<leaf>: read into *val, and set to *nval if not NULL. */
static int icmp6ctl(int leaf, LONG *val, LONG *nval)
{
    LONG mib[4] = { CTL_NET, PF_INET6, IPPROTO_ICMPV6, 0 };
    LONG len = sizeof(*val);

    mib[3] = leaf;
    return MiamiSysCtl(mib, 4, val, &len, nval, nval ? sizeof(*nval) : 0, 0);
}

/* The limits as sized, and each refuses what is out of its range. */
void testND6_LIMITS(void)
{
    LONG v = 0, w = 0, bad;

    CU_ASSERT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXNBRS, &v, NULL), 0);
    printf("\n    nd6_maxnbrs %ld", (long)v);
    CU_ASSERT(v >= 16 && v <= 256);
    CU_ASSERT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXHELD, &w, NULL), 0);
    printf(", nd6_maxheld %ld", (long)w);
    CU_ASSERT(w >= 16 && w <= 256);
    CU_ASSERT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXQLEN, &w, NULL), 0);
    printf(", nd6_maxqlen %ld\n", (long)w);
    CU_ASSERT_EQUAL(w, 16);
    bad = 15;
    CU_ASSERT_NOT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXNBRS, &w, &bad), 0);
    bad = 0;
    CU_ASSERT_NOT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXHELD, &w, &bad), 0);
    CU_ASSERT_NOT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXQLEN, &w, &bad), 0);
    bad = 65;
    CU_ASSERT_NOT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXQLEN, &w, &bad), 0);
    CU_ASSERT_EQUAL(icmp6ctl(ICMPV6CTL_ND6_MAXNBRS, &w, NULL), 0);
    CU_ASSERT_EQUAL(w, v);              /* unchanged by what was refused */
}

/*
 * With room for two held packets in the whole stack, three datagrams to
 * three neighbours nobody is - one after the other - leave the first's to
 * give way when the third comes. When resolution fails (3 solicitations,
 * 1 s apart), the senders of the two still held are told; the first's is
 * not.
 */
void testND6_HELD_LIMIT(void)
{
    struct sockaddr_in6 d;
    LONG old = 0, two = 2;
    long on = 1;
    int s[3], err[3] = { 0, 0, 0 }, i, k;
    char buf[8];

    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(icmp6ctl(ICMPV6CTL_ND6_MAXHELD, &old, &two), 0);
    for (k = 0; k < 3; k++) {
        bzero(&d, sizeof(d));
        d.sin6_len = sizeof(d);
        d.sin6_family = AF_INET6;
        d.sin6_port = htons(9);
        d.sin6_scope_id = ifindex;
        d.sin6_addr.s6_addr[0] = 0xfe;      /* fe80::4d54:dead:10+k */
        d.sin6_addr.s6_addr[1] = 0x80;
        d.sin6_addr.s6_addr[10] = 0x4d;
        d.sin6_addr.s6_addr[11] = 0x54;
        d.sin6_addr.s6_addr[12] = 0xde;
        d.sin6_addr.s6_addr[13] = 0xad;
        d.sin6_addr.s6_addr[15] = 0x10 + k;
        s[k] = socket(AF_INET6, SOCK_DGRAM, 0);
        CU_ASSERT_EQUAL(connect(s[k], (struct sockaddr *)&d, sizeof(d)), 0);
        IoctlSocket(s[k], FIONBIO, (char *)&on);
        CU_ASSERT_EQUAL(send(s[k], "held", 4, 0), 4);
    }
    for (i = 0; i < 80; i++) {
        struct timeval tv = { 0, 100000 };

        WaitSelect(0, NULL, NULL, NULL, &tv, NULL);
        for (k = 0; k < 3; k++)
            if (err[k] == 0 && recv(s[k], buf, sizeof(buf), 0) < 0 && Errno() != EWOULDBLOCK)
                err[k] = Errno();
        if (err[1] != 0 && err[2] != 0 && i > 50)
            break;
    }
    printf("\n    the three senders told: %d %d %d\n", err[0], err[1], err[2]);
    CU_ASSERT_EQUAL(err[0], 0);         /* its packet gave way */
    CU_ASSERT(err[1] == EHOSTUNREACH || err[1] == EHOSTDOWN);
    CU_ASSERT(err[2] == EHOSTUNREACH || err[2] == EHOSTDOWN);
    for (k = 0; k < 3; k++)
        CloseSocket(s[k]);
    icmp6ctl(ICMPV6CTL_ND6_MAXHELD, &two, &old);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ND6 limits tests\n");
        return 0;
    }
    if (st_skip_program("cunit-net-nd6limits", ST_V6)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    {
        struct ifreq ifr;
        int s = socket(AF_INET6, SOCK_DGRAM, 0);

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        if (s >= 0 && IoctlSocket(s, SIOCGIFFLAGS, (char *)&ifr) == 0 &&
            (ifr.ifr_flags & (IFF_UP | IFF_MULTICAST)) == (IFF_UP | IFF_MULTICAST) &&
            IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
            ifindex = ifr.ifr_index;
        if (s >= 0)
            CloseSocket(s);
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ND6_LIMITS_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the cache's limits", testND6_LIMITS) ||
        !CU_add_test(p, "past the held packets' limit, the oldest gives way", testND6_HELD_LIMIT)) {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ND6LIMITS");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (MiamiBase != NULL)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
