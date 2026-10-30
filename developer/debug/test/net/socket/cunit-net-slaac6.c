/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for stateless address autoconfiguration (RFC 4862) across an
    in-place reload of the stack: a Router Advertisement with an autonomous
    prefix gives the interface an address, and after a reload the same
    Router Advertisement must give it the address again.

    The test sends the Router Advertisement itself, from a raw ICMPv6 socket
    with hop limit 255 to the interface's own link-local address, so it loops
    back and is received on that interface without going out on the link;
    its router lifetime is 0, so it adds no default route. The reload goes
    through the stack's control port (<bsdsocket/netcontrol.h>).

    It needs an interface with no IPv6 router of its own (on hosted AROS,
    tap.device attached to a host tap that sends no Router Advertisements).
    Usage:  cunit-net-slaac6 [IFNAME]  (default eth0). Without such an
    interface the tests are skipped.
*/

#define __CONFIG_ROADSHOW__     /* inet_pton() */
#include "socktest.h"

#include <proto/dos.h>
#include <net/if.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";
static unsigned ifindex;
static struct in6_addr linklocal;
static int ready;

#define PREFIX  "2001:db8:4d54:20::"

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

/* Whether the interface has addr, usable (not tentative); with addr NULL,
 * finds its link-local address into *ll. */
static int have_addr(const struct in6_addr *addr, struct in6_addr *ll)
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
            struct in6_addr *a = &req[i].ifr_ifru.ifru_addr.sin6_addr;

            if (strcmp(req[i].ifr_name, ifname) != 0)
                continue;
            if (addr == NULL && IN6_IS_ADDR_LINKLOCAL(a)) {
                *ll = *a;
                found = 1;
            } else if (addr != NULL && memcmp(a, addr, sizeof(*a)) == 0)
                found = 1;
        }
    }
    CloseSocket(s);
    return found;
}

/* The address SLAAC forms from PREFIX and the link-local's interface id. */
static void slaac_addr(struct in6_addr *a)
{
    inet_pton(AF_INET6, PREFIX, a);
    memcpy(&a->s6_addr[8], &linklocal.s6_addr[8], 8);
}

/* A Router Advertisement with router lifetime 0 and one Prefix Information
 * Option for PREFIX/64 (on-link, autonomous), to our own link-local. */
static int send_ra(void)
{
    u_char pkt[sizeof(struct nd_router_advert) + sizeof(struct nd_opt_prefix_info)];
    struct nd_router_advert *ra = (struct nd_router_advert *)pkt;
    struct nd_opt_prefix_info *pi = (struct nd_opt_prefix_info *)(ra + 1);
    struct sockaddr_in6 a;
    int s, hops = 255, n;

    bzero(pkt, sizeof(pkt));
    ra->nd_ra_type = ND_ROUTER_ADVERT;
    pi->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
    pi->nd_opt_pi_len = 4;
    pi->nd_opt_pi_prefix_len = 64;
    pi->nd_opt_pi_flags_reserved = ND_OPT_PI_FLAG_ONLINK | ND_OPT_PI_FLAG_AUTO;
    pi->nd_opt_pi_valid_time = htonl(600);
    pi->nd_opt_pi_preferred_time = htonl(600);
    inet_pton(AF_INET6, PREFIX, &pi->nd_opt_pi_prefix);

    s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (s < 0)
        return -1;
    bzero(&a, sizeof(a));
    a.sin6_len = sizeof(a);
    a.sin6_family = AF_INET6;
    a.sin6_addr = linklocal;
    a.sin6_scope_id = ifindex;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 ||
            setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) != 0) {
        CloseSocket(s);
        return -1;
    }
    n = sendto(s, (char *)pkt, sizeof(pkt), 0, (struct sockaddr *)&a, sizeof(a));
    CloseSocket(s);
    return n == (int)sizeof(pkt) ? 0 : -1;
}

/* Wait up to secs for the SLAAC address to be there and usable. */
static int wait_addr(int secs)
{
    struct in6_addr a;
    int i;

    slaac_addr(&a);
    for (i = 0; i < secs * 5; i++) {
        if (have_addr(&a, NULL))
            return 1;
        Delay(10);
    }
    return 0;
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

#define NEED_IF() do { if (!ready) { CU_PASS("no suitable interface - skipping"); return; } } while (0)

/* ----------------------------------------------------------------- tests */

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

/* A Router Advertisement with an autonomous prefix gives an address. */
void testSLAAC6_ADDRESS(void)
{
    NEED_IF();
    CU_ASSERT_EQUAL(send_ra(), 0);
    CU_ASSERT(wait_addr(5));
}

/* After a reload, the same Router Advertisement gives it again. */
void testSLAAC6_ADDRESS_AFTER_RELOAD(void)
{
    struct in6_addr a;
    int i;
    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(net_reload(), 0);
    /* the interface comes back with its link-local after DAD */
    for (i = 0; i < 50 && !(index_of(ifname, &ifindex) == 0 && have_addr(NULL, &linklocal)); i++)
        Delay(10);
    Delay(3 * 50);
    slaac_addr(&a);
    CU_ASSERT(!have_addr(&a, NULL));        /* the reload took it away */
    CU_ASSERT_EQUAL(send_ra(), 0);
    CU_ASSERT(wait_addr(5));
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 3);
    if (SocketBase == NULL) {
        printf("bsdsocket.library not available - skipping SLAAC6 tests\n");
        return 0;
    }
    ready = index_of(ifname, &ifindex) == 0 && have_addr(NULL, &linklocal) &&
            FindPort((STRPTR)AROSTCP_CTRLPORT_NAME) != NULL;
    if (!ready)
        printf("%s: no interface with a link-local address - skipping SLAAC6 tests\n", ifname);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("SLAAC6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "an autonomous prefix gives an address", testSLAAC6_ADDRESS) ||
        !CU_add_test(p, "after a reload the prefix gives the address again", testSLAAC6_ADDRESS_AFTER_RELOAD))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-SLAAC6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
