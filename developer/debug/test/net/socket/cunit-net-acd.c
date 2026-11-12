/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for IPv4 address conflict detection (RFC 5227): an address
    added to an ARP interface is tentative until it has been probed, then
    ready; one another host has is duplicated, and cannot be bound;
    SIOCGIFAFLAG_IN (NetBSD) reports its state; net.inet.ip.dad_count 0
    turns the probes off. The host on the other side of the interface
    (the tap's 192.168.166.1) answers a probe for its own address.
    Usage:  cunit-net-acd [IFNAME]  (default eth0). Without an IPv4
    address on the interface the tests are skipped.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <proto/miami.h>
#include <bsdsocket/netcontrol.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in_var.h>
#include <sys/sysctl.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

static const char *ifname = "eth0";
static struct in_addr ifaddr4;          /* the interface's own address */
static int ready;

#define NEED_IF() do { if (!ready) { CU_PASS("no IPv4 address on the interface - skipping"); return; } } while (0)

static int ipctl(int leaf, LONG *val, LONG *nval)
{
    LONG mib[4] = { CTL_NET, PF_INET, IPPROTO_IP, 0 };
    LONG len = sizeof(*val);

    mib[3] = leaf;

    return MiamiSysCtl(mib, 4, val, &len, nval, nval ? sizeof(*nval) : 0, 0);
}

/* The address's IN_IFF_* flags, or -1 and errno. */
static int aflags(struct in_addr a)
{
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr = a;
    r = IoctlSocket(s, SIOCGIFAFLAG_IN, (char *)&ifr);
    CloseSocket(s);
    return r == 0 ? ifr.ifr_addrflags : -1;
}

/* Add (add 1) or delete (add 0) the IPv4 alias a, with mask, on the
 * interface. */
static int aliasm(int add, struct in_addr a, ULONG mask)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct in_aliasreq ifra;

        bzero(&ifra, sizeof(ifra));
        snprintf(ifra.ifra_name, sizeof(ifra.ifra_name), "%s", ifname);
        ifra.ifra_addr.sin_len = sizeof(ifra.ifra_addr);
        ifra.ifra_addr.sin_family = AF_INET;
        ifra.ifra_addr.sin_addr = a;
        ifra.ifra_mask.sin_len = sizeof(ifra.ifra_mask);
        ifra.ifra_mask.sin_family = AF_INET;
        ifra.ifra_mask.sin_addr.s_addr = htonl(mask);
        r = IoctlSocket(s, SIOCAIFADDR, (char *)&ifra);
    } else {
        struct ifreq ifr;
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        sin->sin_len = sizeof(*sin);
        sin->sin_family = AF_INET;
        sin->sin_addr = a;
        r = IoctlSocket(s, SIOCDIFADDR, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* The alias a/24. */
static int alias4(int add, struct in_addr a)
{
    return aliasm(add, a, 0xffffff00);
}

/* Wait up to secs for the flags to leave IN_IFF_TENTATIVE; the time it took
 * in ticks, or -1. */
static long settle(struct in_addr a, int secs, int *flags)
{
    long t;

    for (t = 0; t <= secs * TICKS_PER_SECOND; t += 5) {
        *flags = aflags(a);
        if (*flags < 0 || !(*flags & IN_IFF_TENTATIVE))
            return t;
        Delay(5);
    }
    return -1;
}

static int bind4(struct in_addr a)
{
    struct sockaddr_in sin;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    bzero(&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr = a;
    r = bind(s, (struct sockaddr *)&sin, sizeof(sin));
    if (r != 0)
        r = Errno();
    CloseSocket(s);
    return r;
}

/* Delete (del 1) or look up the ARP entry for a; 1 if it is complete. */
static int arpent(int del, struct in_addr a)
{
    struct arpreq ar;
    struct sockaddr_in *sin = (struct sockaddr_in *)&ar.arp_pa;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return 0;
    bzero(&ar, sizeof(ar));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr = a;
    r = IoctlSocket(s, del ? SIOCDARP : SIOCGARP, (char *)&ar);
    CloseSocket(s);
    return r == 0 && !del && (ar.arp_flags & ATF_COM);
}

/* A datagram to a's discard port, from the interface's own address. */
static void send4(struct in_addr a)
{
    struct sockaddr_in sin;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr = ifaddr4;
    bind(s, (struct sockaddr *)&sin, sizeof(sin));
    sin.sin_addr = a;
    sin.sin_port = htons(9);
    sendto(s, "x", 1, 0, (struct sockaddr *)&sin, sizeof(sin));
    CloseSocket(s);
}

/* Reload the stack in place through its control port. */
static int net_reload(void)
{
    struct MsgPort *reply = CreateMsgPort(), *port;
    struct NetControlMsg msg;
    int rc = -1;

    if (reply == NULL)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.ncm_Msg.mn_ReplyPort = reply;
    msg.ncm_Msg.mn_Length = sizeof(msg);
    msg.ncm_Command = NCMD_RELOAD;
    Forbid();
    if ((port = FindPort((STRPTR)AROSTCP_CTRLPORT_NAME)) != NULL)
        PutMsg(port, &msg.ncm_Msg);
    Permit();
    if (port != NULL) {
        WaitPort(reply);
        GetMsg(reply);
        rc = msg.ncm_Result;
    }
    DeleteMsgPort(reply);
    return rc;
}

int init_suite(void) { return SocketBase != NULL && MiamiBase != NULL ? 0 : -1; }
int clean_suite(void) { return 0; }

/* The interface's own address has been probed by the time the tests run. */
void testACD_OWN_READY(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(aflags(ifaddr4), 0);
}

/* A free address is tentative at once, can be bound, and is ready after
 * the probes: a wait below 1 s, three probes 1-2 s apart, 2 s more. */
void testACD_FREE(void)
{
    struct in_addr a;
    long t;
    int f;

    NEED_IF();
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(77);
    CU_ASSERT_EQUAL_FATAL(alias4(1, a), 0);
    f = aflags(a);
    CU_ASSERT_EQUAL(f, IN_IFF_TENTATIVE);
    CU_ASSERT_EQUAL(bind4(a), 0);
    t = settle(a, 10, &f);
    printf("\n    %s: ready after %ld.%02ld s, flags %d\n", inet_ntoa(a),
           t / TICKS_PER_SECOND, t % TICKS_PER_SECOND * 2, f);
    CU_ASSERT(t >= 3 * TICKS_PER_SECOND && t <= 8 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(f, 0);
    CU_ASSERT_EQUAL(alias4(0, a), 0);
}

/* The host's own address: it answers a probe, and the address is
 * duplicated - not bound, not tentative - and does not catch what is
 * sent to the host: that goes out on the link (an ARP entry for the host
 * is made), not to this host itself. */
void testACD_HOST(void)
{
    struct in_addr a;
    long t;
    int f;

    NEED_IF();
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(1);
    arpent(1, a);
    CU_ASSERT_EQUAL_FATAL(alias4(1, a), 0);
    t = settle(a, 10, &f);
    printf("\n    %s (the host's): flags %d after %ld.%02ld s\n", inet_ntoa(a), f,
           t / TICKS_PER_SECOND, t % TICKS_PER_SECOND * 2);
    CU_ASSERT(t >= 0 && t <= 3 * TICKS_PER_SECOND);
    CU_ASSERT_EQUAL(f, IN_IFF_DUPLICATED);
    CU_ASSERT_EQUAL(bind4(a), EADDRNOTAVAIL);
    arpent(1, a);
    send4(a);
    Delay(TICKS_PER_SECOND);
    f = arpent(0, a);
    printf("    a datagram to it went %s\n", f ? "out on the link" : "nowhere on the link");
    CU_ASSERT(f);
    CU_ASSERT_EQUAL(alias4(0, a), 0);
}

/*
 * The host's own address as a host alias (/32), which has a route of its
 * own: duplicated, it must not cut the host off. A socket with no address
 * that sends to the host goes out from a ready address of the interface
 * (not the alias, as NetBSD's in_selectsrc()), and what goes from the
 * interface's address to the host - the reply to the host's ping - leaves
 * on the link.
 */
void testACD_HOST_ROUTE(void)
{
    struct sockaddr_in sin, me;
    socklen_t len = sizeof(me);
    struct in_addr a;
    int f, s;

    NEED_IF();
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(1);
    arpent(1, a);
    CU_ASSERT_EQUAL_FATAL(aliasm(1, a, 0xffffffff), 0);
    CU_ASSERT(settle(a, 10, &f) >= 0);
    CU_ASSERT_EQUAL(f, IN_IFF_DUPLICATED);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&sin, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_addr = a;
    sin.sin_port = htons(9);
    CU_ASSERT_EQUAL(connect(s, (struct sockaddr *)&sin, sizeof(sin)), 0);
    CU_ASSERT_EQUAL(getsockname(s, (struct sockaddr *)&me, &len), 0);
    printf("\n    to %s (duplicated here)", inet_ntoa(a));
    printf(" from %s\n", inet_ntoa(me.sin_addr));
    CU_ASSERT_EQUAL(me.sin_addr.s_addr, ifaddr4.s_addr);
    CU_ASSERT_EQUAL(send(s, "x", 1, 0), 1);
    CloseSocket(s);

    arpent(1, a);
    send4(a);
    Delay(TICKS_PER_SECOND);
    f = arpent(0, a);
    printf("    from %s to it: %s\n", inet_ntoa(ifaddr4), f ? "out on the link" : "nowhere");
    CU_ASSERT(f);
    CU_ASSERT_EQUAL(alias4(0, a), 0);
}

/*
 * SIOCAIFADDR_ORIGIN: an address added saying where it came from (here a
 * DHCP lease's) is probed and ready as any; an origin that is none of
 * IN_ORIGIN_* is refused.
 */
void testACD_ORIGIN(void)
{
    struct in_originreq ifro;
    struct in_addr a;
    int s, f;

    NEED_IF();
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(201);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&ifro, sizeof(ifro));
    snprintf(ifro.ifro_ifra.ifra_name, sizeof(ifro.ifro_ifra.ifra_name), "%s", ifname);
    ifro.ifro_ifra.ifra_addr.sin_len = sizeof(ifro.ifro_ifra.ifra_addr);
    ifro.ifro_ifra.ifra_addr.sin_family = AF_INET;
    ifro.ifro_ifra.ifra_addr.sin_addr = a;
    ifro.ifro_ifra.ifra_mask.sin_len = sizeof(ifro.ifro_ifra.ifra_mask);
    ifro.ifro_ifra.ifra_mask.sin_family = AF_INET;
    ifro.ifro_ifra.ifra_mask.sin_addr.s_addr = htonl(0xffffff00);
    ifro.ifro_origin = IN_ORIGIN_LINKLOCAL + 1;
    CU_ASSERT_EQUAL(IoctlSocket(s, SIOCAIFADDR_ORIGIN, (char *)&ifro), -1);
    CU_ASSERT_EQUAL(Errno(), EINVAL);
    CU_ASSERT(aflags(a) < 0);
    ifro.ifro_origin = IN_ORIGIN_DHCP;
    CU_ASSERT_EQUAL(IoctlSocket(s, SIOCAIFADDR_ORIGIN, (char *)&ifro), 0);
    CloseSocket(s);
    CU_ASSERT(settle(a, 10, &f) >= 0);
    CU_ASSERT_EQUAL(f, 0);
    CU_ASSERT_EQUAL(alias4(0, a), 0);
}

/* net.inet.ip.dad_count 0: no probes, ready at once. */
void testACD_OFF(void)
{
    LONG old = 0, zero = 0;
    struct in_addr a;

    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(ipctl(IPCTL_DAD_COUNT, &old, &zero), 0);
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(78);
    CU_ASSERT_EQUAL(alias4(1, a), 0);
    CU_ASSERT_EQUAL(aflags(a), 0);
    CU_ASSERT_EQUAL(alias4(0, a), 0);
    CU_ASSERT_EQUAL(ipctl(IPCTL_DAD_COUNT, &zero, &old), 0);
    printf("\n    dad_count was %ld\n", (long)old);
    CU_ASSERT_EQUAL(old, 3);
}

/* The sysctl takes 0 to 10; SIOCGIFAFLAG_IN of an address the interface
 * does not have is EADDRNOTAVAIL. */
void testACD_LIMITS(void)
{
    LONG v = 0, big = 11, neg = -1;
    struct in_addr a;

    CU_ASSERT_EQUAL(ipctl(IPCTL_DAD_COUNT, &v, NULL), 0);
    CU_ASSERT_NOT_EQUAL(ipctl(IPCTL_DAD_COUNT, &v, &big), 0);
    CU_ASSERT_NOT_EQUAL(ipctl(IPCTL_DAD_COUNT, &v, &neg), 0);
    NEED_IF();
    a.s_addr = (ifaddr4.s_addr & htonl(0xffffff00)) | htonl(79);
    CU_ASSERT_EQUAL(aflags(a), -1);
    CU_ASSERT_EQUAL(Errno(), EADDRNOTAVAIL);
}

/* An in-place reload deletes and adds the addresses again: the
 * interface's own address, in use before, is back ready at once, not
 * probed again (tentative for seconds). */
void testACD_RELOAD(void)
{
    int f;

    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(aflags(ifaddr4), 0);
    CU_ASSERT_EQUAL_FATAL(net_reload(), 0);
    f = aflags(ifaddr4);
    printf("\n    after a reload: flags %d\n", f);
    CU_ASSERT_EQUAL(f, 0);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping ACD tests\n");
        return 0;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    {
        struct ifreq ifr;
        int s = socket(AF_INET, SOCK_DGRAM, 0);

        bzero(&ifr, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        if (s >= 0 && IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0) {
            ifaddr4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
            ready = ifaddr4.s_addr != INADDR_ANY;
        }
        if (s >= 0)
            CloseSocket(s);
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("ACD_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "the interface's address is ready", testACD_OWN_READY) ||
        !CU_add_test(p, "a free address is tentative, then ready", testACD_FREE) ||
        !CU_add_test(p, "the host's address is duplicated", testACD_HOST) ||
        !CU_add_test(p, "the host's address as a host alias does not cut it off", testACD_HOST_ROUTE) ||
        !CU_add_test(p, "SIOCAIFADDR_ORIGIN", testACD_ORIGIN) ||
        !CU_add_test(p, "dad_count 0: ready at once", testACD_OFF) ||
        !st_add_test(p, "the sysctl's limits, an unknown address", testACD_LIMITS, ST_V4) ||
        !CU_add_test(p, "a reload keeps an address in use ready", testACD_RELOAD))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ACD");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (MiamiBase)
        CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
