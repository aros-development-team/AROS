/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for Roadshow's interface list and query: ObtainInterfaceList()
    gives one node per interface, named as QueryInterfaceTagList() takes it
    (lo0, and IFNAME); ReleaseInterfaceList() frees it. QueryInterfaceTagList()
    fills IFQ_Address, IFQ_NetMask and IFQ_BroadcastAddress with the
    interface's IPv4 address, netmask and broadcast address, as the SIOCGIF*
    ioctls give them, and leaves IFQ_DestinationAddress (point-to-point
    only) zero on a broadcast interface - lo0's 127.0.0.1 and 255.0.0.0 among them - and refuses
    a name no interface has with ENXIO. Usage:  cunit-net-iflist [IFNAME]
    (default eth0; its own checks are skipped without an IPv4 address on it).
*/

#define __CONFIG_ROADSHOW__     /* the interface API */
#include "socktest.h"

#include <libraries/bsdsocket.h>
#include <exec/lists.h>
#include <net/if.h>

struct Library *SocketBase = NULL;

static const char *ifname = "eth0";

/* Whether the list has an interface of that name. */
static int listed(const struct List *l, const char *name)
{
    const struct Node *n;

    for (n = l->lh_Head; n->ln_Succ; n = n->ln_Succ)
        if (n->ln_Name && strcmp(n->ln_Name, name) == 0)
            return 1;
    return 0;
}

/* An ioctl's address for the interface, 0 if it has none. */
static ULONG ifioctl(int req)
{
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    ULONG a = 0;

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (s >= 0 && IoctlSocket(s, req, (char *)&ifr) == 0)
        a = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
    if (s >= 0)
        CloseSocket(s);
    return a;
}

/* QueryInterfaceTagList(name): MTU, address, netmask, broadcast. */
static struct sockaddr_in dest;      /* IFQ_DestinationAddress, every query */

static LONG query(const char *name, ULONG *mtu, struct sockaddr_in *addr,
                  struct sockaddr_in *mask, struct sockaddr_in *bcast)
{
    struct TagItem t[6];

    t[0].ti_Tag = IFQ_MTU;              t[0].ti_Data = (IPTR)mtu;
    t[1].ti_Tag = IFQ_Address;          t[1].ti_Data = (IPTR)addr;
    t[2].ti_Tag = IFQ_NetMask;          t[2].ti_Data = (IPTR)mask;
    t[3].ti_Tag = IFQ_BroadcastAddress; t[3].ti_Data = (IPTR)bcast;
    t[4].ti_Tag = IFQ_DestinationAddress; t[4].ti_Data = (IPTR)&dest;
    t[5].ti_Tag = TAG_DONE;
    memset(&dest, 0xee, sizeof(dest));
    *mtu = 0;
    memset(addr, 0xee, sizeof(*addr));
    memset(mask, 0xee, sizeof(*mask));
    memset(bcast, 0xee, sizeof(*bcast));
    return QueryInterfaceTagList((STRPTR)name, t);
}

int init_suite(void)
{
    return SocketBase == NULL ? -1 : 0;
}

int clean_suite(void)
{
    return 0;
}

/* The list: lo0, and the interface; released. */
void testIFLIST_LIST(void)
{
    struct List *l = ObtainInterfaceList();

    CU_ASSERT_PTR_NOT_NULL_FATAL(l);
    CU_ASSERT(listed(l, "lo0"));
    if (ifioctl(SIOCGIFADDR))
        CU_ASSERT(listed(l, ifname));
    ReleaseInterfaceList(l);
}

/* lo0: its MTU, 127.0.0.1 and 255.0.0.0. */
void testIFLIST_LOOPBACK(void)
{
    struct sockaddr_in a, m, b;
    ULONG mtu;

    CU_ASSERT_EQUAL(query("lo0", &mtu, &a, &m, &b), 0);
    CU_ASSERT(mtu > 0);
    CU_ASSERT_EQUAL(a.sin_family, AF_INET);
    CU_ASSERT_EQUAL(a.sin_len, sizeof(a));
    CU_ASSERT_EQUAL(a.sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    CU_ASSERT_EQUAL(m.sin_addr.s_addr, htonl(0xff000000));
    /* neither point-to-point nor broadcast: no such addresses */
    CU_ASSERT_EQUAL(b.sin_addr.s_addr, 0);
    CU_ASSERT_EQUAL(dest.sin_addr.s_addr, 0);
}

/* The interface: what the ioctls say. */
void testIFLIST_INTERFACE(void)
{
    struct sockaddr_in a, m, b;
    ULONG mtu, ia = ifioctl(SIOCGIFADDR);

    if (ia == 0) {
        CU_PASS("no IPv4 address on the interface - skipping");
        return;
    }
    CU_ASSERT_EQUAL(query(ifname, &mtu, &a, &m, &b), 0);
    CU_ASSERT(mtu >= 576);
    CU_ASSERT_EQUAL(a.sin_addr.s_addr, ia);
    CU_ASSERT_EQUAL(m.sin_addr.s_addr, ifioctl(SIOCGIFNETMASK));
    CU_ASSERT_EQUAL(b.sin_addr.s_addr, ifioctl(SIOCGIFBRDADDR));
    CU_ASSERT(b.sin_addr.s_addr != 0);
    /* a broadcast interface has no destination address */
    CU_ASSERT_EQUAL(dest.sin_addr.s_addr, 0);
}

/* A name no interface has: -1, ENXIO. */
void testIFLIST_UNKNOWN(void)
{
    struct sockaddr_in a, m, b;
    ULONG mtu;

    CU_ASSERT_EQUAL(query("nosuch9", &mtu, &a, &m, &b), -1);
    CU_ASSERT_EQUAL(Errno(), ENXIO);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        ifname = argv[1];
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping IFLIST tests\n");
        return 0;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("IFLIST_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !CU_add_test(p, "ObtainInterfaceList() lists the interfaces", testIFLIST_LIST) ||
        !CU_add_test(p, "QueryInterfaceTagList(lo0): MTU, address, netmask", testIFLIST_LOOPBACK) ||
        !CU_add_test(p, "QueryInterfaceTagList(): what the ioctls say", testIFLIST_INTERFACE) ||
        !CU_add_test(p, "an unknown name: ENXIO", testIFLIST_UNKNOWN))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-IFLIST");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
