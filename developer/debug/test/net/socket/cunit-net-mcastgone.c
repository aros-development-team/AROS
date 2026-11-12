/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for group memberships whose interface goes away: a socket
    that joined a group on an interface - IPv4 by the interface's address,
    IPv6 by its index - can name it no more once it is gone, so the stack
    leaves those memberships itself (as FreeBSD's in_pcbpurgeif0()), and
    the socket has every one of its IP_MAX_MEMBERSHIPS (IPV6_MAX_MEMBERSHIPS)
    for other groups again. The interface is taken away the way the
    Network preferences do it: its line leaves the "interfaces" file and
    the stack reloads its configuration in place (NCMD_RELOAD); the line is
    put back and the stack reloaded again at the end.

    Usage:  cunit-net-mcastgone [IFNAME [STAY [INTERFACES]]]  (default eth1,
    eth0, and the "interfaces" file in ENV:AROSTCP/Config, else in
    SYS:System/Network/AROSTCP/db). IFNAME goes away, STAY carries the new
    groups. Without both interfaces the tests are skipped.
*/

#include "socktest.h"

#include <bsdsocket/netcontrol.h>
#include <dos/var.h>
#include <net/if.h>
#include <proto/dos.h>

struct Library *SocketBase = NULL;

#define MAX_MEMBERSHIPS     20      /* IP_MAX_MEMBERSHIPS, IPV6_MAX_MEMBERSHIPS */

static const char *gone = "eth1", *stay = "eth0";
static char config[256];
static char *saved;                 /* the "interfaces" file as it was */
static LONG savedLength;

static unsigned index_of(const char *name)
{
    struct ifreq ifr;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    unsigned idx = 0;

    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (s >= 0 && IoctlSocket(s, SIOCGIFINDEX, (char *)&ifr) == 0)
        idx = ifr.ifr_index;
    if (s >= 0)
        CloseSocket(s);
    return idx;
}

static struct in_addr address_of(const char *name)
{
    struct ifreq ifr;
    struct in_addr a;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    a.s_addr = INADDR_ANY;
    bzero(&ifr, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", name);
    if (s >= 0 && IoctlSocket(s, SIOCGIFADDR, (char *)&ifr) == 0 &&
            ((struct sockaddr_in *)&ifr.ifr_addr)->sin_family == AF_INET)
        a = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    if (s >= 0)
        CloseSocket(s);
    return a;
}

/* The stack reconfigures itself in place, as the Network preferences ask. */
static LONG reload(void)
{
    struct MsgPort *reply = CreateMsgPort(), *ctrl;
    struct NetControlMsg msg;
    LONG result = -1;

    if (reply == NULL)
        return -1;
    bzero(&msg, sizeof(msg));
    msg.ncm_Msg.mn_Node.ln_Type = NT_MESSAGE;
    msg.ncm_Msg.mn_ReplyPort = reply;
    msg.ncm_Msg.mn_Length = sizeof(msg);
    msg.ncm_Command = NCMD_RELOAD;
    Forbid();
    ctrl = FindPort(AROSTCP_CTRLPORT_NAME);
    if (ctrl != NULL)
        PutMsg(ctrl, &msg.ncm_Msg);
    Permit();
    if (ctrl != NULL) {
        WaitPort(reply);
        GetMsg(reply);
        result = msg.ncm_Result;
    }
    DeleteMsgPort(reply);
    return result;
}

static int write_file(const char *text, LONG length)
{
    BPTR fh = Open(config, MODE_NEWFILE);
    LONG n;

    if (fh == BNULL)
        return -1;
    n = Write(fh, (APTR)text, length);
    Close(fh);
    return n == length ? 0 : -1;
}

/* The "interfaces" file without the lines of IFNAME. */
static int drop_interface(void)
{
    size_t namelen = strlen(gone);
    char *copy = AllocVec(savedLength + 1, MEMF_ANY);
    LONG i = 0, n = 0;
    int r;

    if (copy == NULL)
        return -1;
    while (i < savedLength) {
        LONG end = i;

        while (end < savedLength && saved[end] != '\n')
            end++;
        if (end < savedLength)
            end++;
        if (!(end - i > (LONG)namelen && strncmp(saved + i, gone, namelen) == 0 &&
                (saved[i + namelen] == ' ' || saved[i + namelen] == '\t'))) {
            memcpy(copy + n, saved + i, end - i);
            n += end - i;
        }
        i = end;
    }
    r = write_file(copy, n);
    FreeVec(copy);
    return r;
}

static int load_config(void)
{
    BPTR fh;

    if (config[0] == '\0') {
        if (GetVar("AROSTCP/Config", config, sizeof(config) - 16, GVF_GLOBAL_ONLY) < 0)
            strcpy(config, "SYS:System/Network/AROSTCP/db");
        AddPart(config, "interfaces", sizeof(config));
    }
    fh = Open(config, MODE_OLDFILE);
    if (fh == BNULL)
        return -1;
    saved = AllocVec(65536, MEMF_ANY);
    savedLength = saved != NULL ? Read(fh, saved, 65536) : -1;
    Close(fh);
    return savedLength > 0 ? 0 : -1;
}

static int ready;
#define NEED_IF() do { if (!ready) { CU_PASS("no two interfaces or no configuration - skipping"); return; } } while (0)

/* Joins up to MAX_MEMBERSHIPS groups on STAY; how many it could. */
static int fill4(int s, struct in_addr on)
{
    struct ip_mreq mr;
    int i;

    for (i = 0; i < MAX_MEMBERSHIPS; i++) {
        bzero(&mr, sizeof(mr));
        mr.imr_multiaddr.s_addr = htonl(0xef4d0200 + i);    /* 239.77.2.i */
        mr.imr_interface = on;
        if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) != 0)
            break;
    }
    return i;
}

static int fill6(int s, unsigned on)
{
    struct ipv6_mreq mr;
    int i;

    for (i = 0; i < MAX_MEMBERSHIPS; i++) {
        bzero(&mr, sizeof(mr));
        mr.ipv6mr_multiaddr.s6_addr[0] = 0xff;
        mr.ipv6mr_multiaddr.s6_addr[1] = 0x02;
        mr.ipv6mr_multiaddr.s6_addr[13] = 0x4d;
        mr.ipv6mr_multiaddr.s6_addr[14] = 0x02;
        mr.ipv6mr_multiaddr.s6_addr[15] = i;               /* ff02::4d:2xx */
        mr.ipv6mr_interface = on;
        if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mr, sizeof(mr)) != 0)
            break;
    }
    return i;
}

/*
 * One socket of each family joins the mDNS group on IFNAME, which then goes
 * away: each can still join MAX_MEMBERSHIPS groups on STAY.
 */
void testMCASTGONE(void)
{
    struct ip_mreq mr;
    struct ipv6_mreq mr6;
    struct in_addr goneaddr, stayaddr;
    unsigned goneindex, stayindex;
    int s4, s6, have6 = st_have_family(AF_INET6);

    NEED_IF();
    goneaddr = address_of(gone);
    goneindex = index_of(gone);
    stayindex = index_of(stay);
    s4 = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s4 >= 0);
    bzero(&mr, sizeof(mr));
    mr.imr_multiaddr.s_addr = htonl(0xe00000fb);            /* 224.0.0.251 */
    mr.imr_interface = goneaddr;
    CU_ASSERT_EQUAL(setsockopt(s4, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)), 0);
    s6 = have6 ? socket(AF_INET6, SOCK_DGRAM, 0) : -1;
    if (s6 >= 0) {
        bzero(&mr6, sizeof(mr6));
        mr6.ipv6mr_multiaddr.s6_addr[0] = 0xff;
        mr6.ipv6mr_multiaddr.s6_addr[1] = 0x02;
        mr6.ipv6mr_multiaddr.s6_addr[15] = 0xfb;            /* ff02::fb */
        mr6.ipv6mr_interface = goneindex;
        CU_ASSERT_EQUAL(setsockopt(s6, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mr6, sizeof(mr6)), 0);
    }

    CU_ASSERT_EQUAL_FATAL(drop_interface(), 0);
    CU_ASSERT_EQUAL(reload(), 0);
    CU_ASSERT_EQUAL(index_of(gone), 0);
    printf("\n    %s gone (index %u was)\n", gone, goneindex);

    stayaddr = address_of(stay);
    CU_ASSERT_EQUAL(fill4(s4, stayaddr), MAX_MEMBERSHIPS);
    /* leaving it again by its old address finds no interface */
    CU_ASSERT_NOT_EQUAL(setsockopt(s4, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mr, sizeof(mr)), 0);
    if (s6 >= 0) {
        CU_ASSERT_EQUAL(fill6(s6, stayindex), MAX_MEMBERSHIPS);
        CloseSocket(s6);
    }
    CloseSocket(s4);
}

/* IFNAME comes back: a socket joins the group on it again. */
void testMCASTGONE_BACK(void)
{
    struct ip_mreq mr;
    int s;

    NEED_IF();
    CU_ASSERT_EQUAL_FATAL(write_file(saved, savedLength), 0);
    CU_ASSERT_EQUAL(reload(), 0);
    CU_ASSERT_NOT_EQUAL(index_of(gone), 0);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    bzero(&mr, sizeof(mr));
    mr.imr_multiaddr.s_addr = htonl(0xe00000fb);
    mr.imr_interface = address_of(gone);
    CU_ASSERT_EQUAL(setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)), 0);
    CloseSocket(s);
}

int init_suite(void) { return (SocketBase != NULL) ? 0 : -1; }
int clean_suite(void) { return 0; }

int main(int argc, char **argv)
{
    CU_pSuite p;

    if (argc > 1)
        gone = argv[1];
    if (argc > 2)
        stay = argv[2];
    if (argc > 3)
        snprintf(config, sizeof(config), "%s", argv[3]);
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("bsdsocket.library 4 not available - skipping MCASTGONE tests\n");
        return 0;
    }
    ready = index_of(gone) != 0 && index_of(stay) != 0 &&
            address_of(gone).s_addr != INADDR_ANY && address_of(stay).s_addr != INADDR_ANY &&
            load_config() == 0;
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MCASTGONE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "memberships on an interface that goes away are left", testMCASTGONE, ST_V4) ||
        !st_add_test(p, "the interface comes back", testMCASTGONE_BACK, ST_V4))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-MCASTGONE");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (saved != NULL)
        FreeVec(saved);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
