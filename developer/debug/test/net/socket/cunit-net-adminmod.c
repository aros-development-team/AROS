/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the administration module of the stack ("admin",
    admin.atcp: bsdsocket/kern/admin_ops.h): the routing socket,
    the route, interface and name-server calls of bsdsocket.library and the
    control panel's glue, loaded on first use.

     - It is not loaded when the stack has started (unless the control
       panel is to be opened at start and is installed). Strict only with
       --first, when this program is the first to use the network after
       the stack started: any other program may have loaded it already.
     - socket(PF_ROUTE) loads it, and a route added with SIOCADDRT (IPv4) or
       an address added (IPv6 only) arrives on that socket as a routing
       message. It is not unloaded while the socket is open.
     - GetRouteInfo(), ObtainInterfaceList() and
       ObtainDomainNameServerList() load it, and it is not unloaded while
       the list they handed out is not given back.
     - An address change still signals the programs that asked for it
       (SBTC_SIG_ADDRESS_CHANGE_MASK) with the module not loaded, and does
       not load it.
     - With ENV:AROSTCP/Without_admin set (as if the library were not
       installed): what each call answers then (STAGE7-DESIGN.md 3.2), and
       the route and address ioctls work as before.

    The module may be part of the stack program (not a library file): it is
    then stopped instead of unloaded, and everything else is the same.
    Usage: cunit-net-adminmod [--first]
*/

#define __CONFIG_ROADSHOW__     /* the Roadshow calls, SocketBaseTags() */
#include "socktest.h"

#include <proto/dos.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <dos/var.h>
#include <libraries/bsdsocket.h>
#include <bsdsocket/socketbasetags.h>
#include <bsdsocket/netcontrol.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in_var.h>

#include <stddef.h>

/*
 * Vector 117 has a prototype (clib/bsdsocket_protos.h) but no call macro in
 * defines/bsdsocket.h (which stops short of several Roadshow vectors): the
 * test calls it directly.
 */
#ifndef GetDefaultDomainName
#define GetDefaultDomainName(buffer, size) \
        AROS_LC2(LONG, GetDefaultDomainName, \
                  AROS_LCA(STRPTR, (buffer), A0), \
                  AROS_LCA(LONG, (size), D0), \
        struct Library *, SocketBase, 117, BSDSocket)
#endif

struct Library *SocketBase = NULL;

#define PROGRAM         "cunit-net-adminmod"
#define ADMIN_LIBRARY   "admin.atcp"
#define ADMIN_MODULE    "admin"
#define ADMIN_SWITCH    "AROSTCP/Without_admin"
#define MSG_WAIT        3       /* s to wait for a routing message */

/* addresses of the documentation ranges: nothing real is reached through them */
#define TEST_HOST4      "192.0.2.88"
#define TEST_ADDR4      "192.0.2.77"
#define TEST_NS4        "192.0.2.53"
static const struct in6_addr test_addr6 =
    {{{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x77 }}};

static int first = 0;           /* --first: nothing used the network before this program */
static int running_at_entry;    /* the module ran when this program started */
static ULONG sigmask = 0;

/* ---- the control port ------------------------------------------------- */

/* One module command; the stack's answer, -1 without a control port. */
static LONG module_cmd(ULONG cmd, const char *name, ULONG *flags)
{
    struct NetControlModuleMsg mm;
    struct MsgPort *reply, *port;

    if ((reply = CreateMsgPort()) == NULL)
        return -1;
    memset(&mm, 0, sizeof(mm));
    mm.ncmm_Msg.ncm_Msg.mn_ReplyPort = reply;
    mm.ncmm_Msg.ncm_Msg.mn_Length = sizeof(mm);
    mm.ncmm_Msg.ncm_Command = cmd;
    mm.ncmm_Msg.ncm_Result = -1;
    strncpy(mm.ncmm_Name, name, sizeof(mm.ncmm_Name) - 1);

    Forbid();
    if ((port = FindPort(AROSTCP_CTRLPORT_NAME)) != NULL)
        PutMsg(port, &mm.ncmm_Msg.ncm_Msg);
    Permit();
    if (port != NULL) {
        WaitPort(reply);
        GetMsg(reply);
    }
    DeleteMsgPort(reply);
    if (flags) *flags = mm.ncmm_Flags;
    return port ? mm.ncmm_Msg.ncm_Result : -1;
}

/* Whether the module runs now. */
static int admin_running(void)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, ADMIN_MODULE, &flags) == 0 && (flags & NCMMF_STARTED);
}

/* Whether the module is part of the stack program (it is stopped, not unloaded). */
static int admin_builtin(void)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, ADMIN_MODULE, &flags) == 0 && !(flags & NCMMF_LIBRARY);
}

/* Take the module out: 0 when it is out now, else the stack's refusal. */
static LONG admin_out(void)
{
    ULONG flags = 0;
    LONG r = module_cmd(NCMD_MODULE_QUERY, ADMIN_MODULE, &flags);

    if (r == ENOENT || (r == 0 && !(flags & NCMMF_STARTED)))
        return 0;
    if (r != 0)
        return r;
    return module_cmd((flags & NCMMF_LIBRARY) ? NCMD_MODULE_UNLOAD : NCMD_MODULE_STOP,
                      ADMIN_MODULE, NULL);
}

/* exec's open count of a library in memory, -1 when it is not there */
static LONG lib_opencount(const char *name)
{
    struct Library *lib;
    LONG count = -1;

    Forbid();
    if ((lib = (struct Library *)FindName(&SysBase->LibList, (STRPTR)name)) != NULL)
        count = lib->lib_OpenCnt;
    Permit();
    return count;
}

/* The stack loads the module at start when it opens the control panel: OPENGUI and an installed panel. */
static int panel_at_start(void)
{
    char line[160], panel[64], path[128];
    BPTR fh, lock;
    int opengui = 1;        /* the stack's default */

    panel[0] = '\0';
    if ((fh = Open("ENV:AROSTCP/db/general.config", MODE_OLDFILE)) == BNULL)
        return 0;
    while (FGets(fh, line, sizeof(line)) != NULL) {
        char *p = line, *e;

        while (*p == ' ' || *p == '\t')
            p++;
        if (strncasecmp(p, "OPENGUI", 7) == 0 && (e = strchr(p, '=')) != NULL)
            opengui = !(strncasecmp(e + 1, "NO", 2) == 0 || e[1] == '0');
        else if (strncasecmp(p, "GUI", 3) == 0 && (e = strstr(p, "PANEL=")) != NULL) {
            int n = 0;

            for (e += 6; *e && *e != ' ' && *e != '\t' && *e != '\n' && n < (int)sizeof(panel) - 1; e++)
                panel[n++] = *e;
            panel[n] = '\0';
        }
    }
    Close(fh);
    if (!opengui || panel[0] == '\0')
        return 0;
    snprintf(path, sizeof(path), "LIBS:%s.MiamiPanel", panel);
    if ((lock = Lock(path, ACCESS_READ)) == BNULL)
        return 0;
    UnLock(lock);
    return 1;
}

/* ---- routing messages ------------------------------------------------- */

/*
 * Read routing messages for up to MSG_WAIT seconds; 1 when one of `type'
 * came whose first address is `want' (sa_family and the address bytes
 * from `off' for `len').
 */
static int await_msg(int rs, int type, const struct sockaddr *want, int off, int len)
{
    char buf[512];
    int i;

    for (i = 0; i < MSG_WAIT * 4; i++) {
        struct rt_msghdr *rtm = (struct rt_msghdr *)buf;
        const struct sockaddr *sa;
        int n, hdr;

        if (!sock_wait(rs, 0, 1))
            continue;
        if ((n = recv(rs, buf, sizeof(buf), 0)) < (int)sizeof(struct ifa_msghdr))
            continue;
        if (rtm->rtm_type != type)
            continue;
        hdr = (type == RTM_NEWADDR || type == RTM_DELADDR) ? sizeof(struct ifa_msghdr)
                                                           : sizeof(struct rt_msghdr);
        if (n < hdr + off + len)
            continue;
        sa = (const struct sockaddr *)(buf + hdr);
        if (sa->sa_family == want->sa_family &&
                memcmp((const char *)sa + off, (const char *)want + off, len) == 0)
            return 1;
    }
    return 0;
}

static void sin_of(struct sockaddr_in *sin, const char *addr)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = inet_addr((char *)addr);
}

/* SIOCADDRT (add 1) or SIOCDELRT: a host route to TEST_HOST4 through 127.0.0.1 */
static int route4(int add)
{
    struct ortentry rt;
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    memset(&rt, 0, sizeof(rt));
    sin_of((struct sockaddr_in *)&rt.rt_dst, TEST_HOST4);
    sin_of((struct sockaddr_in *)&rt.rt_gateway, "127.0.0.1");
    rt.rt_flags = RTF_UP | RTF_HOST | RTF_GATEWAY;
    r = IoctlSocket(s, add ? SIOCADDRT : SIOCDELRT, (char *)&rt);
    CloseSocket(s);
    return r;
}

/* SIOCAIFADDR (add 1) or SIOCDIFADDR: TEST_ADDR4/32 on lo0 */
static int addr4(int add)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    if (add) {
        struct ifaliasreq ifra;

        memset(&ifra, 0, sizeof(ifra));
        strncpy(ifra.ifra_name, "lo0", sizeof(ifra.ifra_name) - 1);
        sin_of((struct sockaddr_in *)&ifra.ifra_addr, TEST_ADDR4);
        sin_of((struct sockaddr_in *)&ifra.ifra_mask, "255.255.255.255");
        r = IoctlSocket(s, SIOCAIFADDR, (char *)&ifra);
    } else {
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, "lo0", sizeof(ifr.ifr_name) - 1);
        sin_of((struct sockaddr_in *)&ifr.ifr_addr, TEST_ADDR4);
        r = IoctlSocket(s, SIOCDIFADDR, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* SIOCAIFADDR_IN6 (add 1) or SIOCDIFADDR_IN6: test_addr6/128 on lo0 */
static int addr6(int add)
{
    int s = socket(AF_INET6, SOCK_DGRAM, 0), r, i;

    if (s < 0)
        return -1;
    if (add) {
        struct in6_aliasreq ifra;

        memset(&ifra, 0, sizeof(ifra));
        strncpy(ifra.ifra_name, "lo0", sizeof(ifra.ifra_name) - 1);
        ifra.ifra_addr.sin6_len = sizeof(ifra.ifra_addr);
        ifra.ifra_addr.sin6_family = AF_INET6;
        ifra.ifra_addr.sin6_addr = test_addr6;
        ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
        ifra.ifra_prefixmask.sin6_family = AF_INET6;
        for (i = 0; i < 16; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
        ifra.ifra_lifetime.ia6t_vltime = ifra.ifra_lifetime.ia6t_pltime = 0xffffffffU;
        r = IoctlSocket(s, SIOCAIFADDR_IN6, (char *)&ifra);
    } else {
        struct in6_ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, "lo0", sizeof(ifr.ifr_name) - 1);
        ifr.ifr_ifru.ifru_addr.sin6_len = sizeof(struct sockaddr_in6);
        ifr.ifr_ifru.ifru_addr.sin6_family = AF_INET6;
        ifr.ifr_ifru.ifru_addr.sin6_addr = test_addr6;
        r = IoctlSocket(s, SIOCDIFADDR_IN6, (char *)&ifr);
    }
    CloseSocket(s);
    return r;
}

/* Whether the address change signal came within secs (and clear it). */
static int got_signal(int secs)
{
    int i;

    for (i = 0; i < secs * 10; i++) {
        if (SetSignal(0, sigmask) & sigmask) {
            SetSignal(0, sigmask);
            return 1;
        }
        Delay(5);
    }
    return 0;
}

/* The module out of the way for a test that starts from "not loaded"; 0 when it could not be. */
static int need_out(void)
{
    LONG r = admin_out();

    if (r != 0) {
        printf("(the module could not be taken out: error %ld - the panel open, "
               "or another program holds a routing socket or a list)\n", (long)r);
        return 0;
    }
    return 1;
}

/* ---- tests ------------------------------------------------------------ */

static int init_suite(void)  { return 0; }
static int clean_suite(void) { return 0; }

static void testADMIN_NOT_LOADED_AT_START(void)
{
    int running = running_at_entry;

    if (panel_at_start()) {
        printf("(OPENGUI is set and the panel is installed: the stack loads the module "
               "to open it - %s now)\n", running ? "loaded" : "not loaded");
        CU_PASS("the panel is opened at start");
        return;
    }
    if (!first) {
        /* another program may have loaded it since the start: proves nothing either way */
        if (running)
            printf("(loaded now; run with --first as the first network program to check the start)\n");
        CU_PASS("not strict without --first");
        return;
    }
    CU_ASSERT_FALSE(running);
}

static void testADMIN_ROUTE_SOCKET(void)
{
    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
        struct sockaddr_in6 sin6;
    } want;
    int rs, got;

    if (!need_out()) {
        CU_PASS("module in use - skipping");
        return;
    }
    CU_ASSERT_FALSE_FATAL(admin_running());

    rs = socket(PF_ROUTE, SOCK_RAW, 0);
    CU_ASSERT_FATAL(rs >= 0);
    CU_ASSERT(admin_running());

    if (st_have_family(AF_INET)) {
        /* the route of an ioctl, as the old interface reports it */
        sin_of(&want.sin, TEST_HOST4);
        CU_ASSERT_EQUAL_FATAL(route4(1), 0);
        got = await_msg(rs, RTM_OLDADD, &want.sa,
                        offsetof(struct sockaddr_in, sin_addr), sizeof(struct in_addr));
        CU_ASSERT(got);
        CU_ASSERT_EQUAL(route4(0), 0);
        CU_ASSERT(await_msg(rs, RTM_OLDDEL, &want.sa,
                            offsetof(struct sockaddr_in, sin_addr), sizeof(struct in_addr)));
    } else {
        /* no IPv4, no SIOCADDRT: an address added to lo0 */
        memset(&want, 0, sizeof(want));
        want.sin6.sin6_len = sizeof(want.sin6);
        want.sin6.sin6_family = AF_INET6;
        want.sin6.sin6_addr = test_addr6;
        CU_ASSERT_EQUAL_FATAL(addr6(1), 0);
        CU_ASSERT(await_msg(rs, RTM_NEWADDR, &want.sa,
                            offsetof(struct sockaddr_in6, sin6_addr), sizeof(struct in6_addr)));
        CU_ASSERT_EQUAL(addr6(0), 0);
        CU_ASSERT(await_msg(rs, RTM_DELADDR, &want.sa,
                            offsetof(struct sockaddr_in6, sin6_addr), sizeof(struct in6_addr)));
    }

    /* not while the socket is open */
    CU_ASSERT_EQUAL(admin_out(), EBUSY);
    CU_ASSERT(admin_running());
    CloseSocket(rs);
    CU_ASSERT_EQUAL(admin_out(), 0);
    CU_ASSERT_FALSE(admin_running());
}

static void testADMIN_LISTS(void)
{
    struct rt_msghdr *rt;
    struct List *l;

    if (!need_out()) {
        CU_PASS("module in use - skipping");
        return;
    }

    /* the routing table: there are always the loopback routes */
    rt = GetRouteInfo(0, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(rt);
    CU_ASSERT(admin_running());
    CU_ASSERT(rt->rtm_msglen >= sizeof(struct rt_msghdr));
    CU_ASSERT_EQUAL(admin_out(), EBUSY);
    CU_ASSERT(admin_running());
    FreeRouteInfo(rt);
    CU_ASSERT_EQUAL(admin_out(), 0);

    /* the interfaces: lo0 at least */
    l = ObtainInterfaceList();
    CU_ASSERT_PTR_NOT_NULL_FATAL(l);
    CU_ASSERT(admin_running());
    CU_ASSERT_FALSE(IsListEmpty(l));
    CU_ASSERT_EQUAL(admin_out(), EBUSY);
    ReleaseInterfaceList(l);
    CU_ASSERT_EQUAL(admin_out(), 0);

    /* the name servers: a list, empty or not */
    l = ObtainDomainNameServerList();
    CU_ASSERT_PTR_NOT_NULL_FATAL(l);
    CU_ASSERT(admin_running());
    CU_ASSERT_EQUAL(admin_out(), EBUSY);
    ReleaseDomainNameServerList(l);
    CU_ASSERT_EQUAL(admin_out(), 0);
    CU_ASSERT_FALSE(admin_running());
}

static void testADMIN_ADDRSIG_WITHOUT_MODULE(void)
{
    int v4 = st_have_family(AF_INET);

    if (sigmask == 0) {
        CU_PASS("no signal - skipping");
        return;
    }
    if (!need_out()) {
        CU_PASS("module in use - skipping");
        return;
    }
    SetSignal(0, sigmask);
    CU_ASSERT_EQUAL_FATAL(v4 ? addr4(1) : addr6(1), 0);
    CU_ASSERT(got_signal(3));
    CU_ASSERT_EQUAL(v4 ? addr4(0) : addr6(0), 0);
    CU_ASSERT(got_signal(3));
    /* an address change is not a reason to load it */
    CU_ASSERT_FALSE(admin_running());
}

static void testADMIN_NOT_INSTALLED(void)
{
    struct TagItem tags[] = {
        { RTA_DestinationHost, 0 },
        { RTA_Gateway, 0 },
        { TAG_END, 0 }
    };
    char buf[64];
    int builtin, rs;

    if (!need_out()) {
        CU_PASS("module in use - skipping");
        return;
    }
    builtin = admin_builtin();
    tags[0].ti_Data = inet_addr(TEST_HOST4);
    tags[1].ti_Data = inet_addr("127.0.0.1");
    CU_ASSERT_FATAL(SetVar(ADMIN_SWITCH, "1", 1, GVF_GLOBAL_ONLY) != 0);

    /* the routing socket: as for a family the stack does not have */
    rs = socket(PF_ROUTE, SOCK_RAW, 0);
    CU_ASSERT_EQUAL(rs, -1);
    CU_ASSERT_EQUAL(Errno(), EPROTONOSUPPORT);
    if (rs >= 0)
        CloseSocket(rs);

    /* the calls: their failure value, ENOENT */
    CU_ASSERT_EQUAL(AddRouteTagList(tags), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_EQUAL(DeleteRouteTagList(tags), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_EQUAL(ChangeRouteTagList(tags), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_PTR_NULL(GetRouteInfo(0, 0));
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_PTR_NULL(ObtainInterfaceList());
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_PTR_NULL(ObtainDomainNameServerList());
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_EQUAL(AddDomainNameServer((STRPTR)TEST_NS4), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_EQUAL(RemoveDomainNameServer((STRPTR)TEST_NS4), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    strcpy(buf, "x");
    CU_ASSERT_EQUAL(GetDefaultDomainName((STRPTR)buf, sizeof(buf)), 0);
    CU_ASSERT_EQUAL(buf[0], '\0');
    /* giving back nothing is nothing */
    FreeRouteInfo(NULL);
    ReleaseInterfaceList(NULL);
    ReleaseDomainNameServerList(NULL);
    /* what is not implemented says so, as before, without the module */
    CU_ASSERT_EQUAL(AddInterfaceTagList((STRPTR)"x0", (STRPTR)"none.device", 0, NULL), -1);
    CU_ASSERT_EQUAL(Errno(), ENOSYS);
    CU_ASSERT_FALSE(admin_running());
    if (!builtin) {
        CU_ASSERT_EQUAL(lib_opencount(ADMIN_LIBRARY), -1);
    }

    /* the core's ioctls work without it */
    if (st_have_family(AF_INET)) {
        CU_ASSERT_EQUAL(route4(1), 0);
        CU_ASSERT_EQUAL(route4(0), 0);
        CU_ASSERT_EQUAL(addr4(1), 0);
        CU_ASSERT_EQUAL(addr4(0), 0);
    }
    if (st_have_family(AF_INET6)) {
        CU_ASSERT_EQUAL(addr6(1), 0);
        CU_ASSERT_EQUAL(addr6(0), 0);
    }
    CU_ASSERT_FALSE(admin_running());

    /*
     * Back as it was: the switch away, and what the stack remembered about
     * the module forgotten - a library loaded by name (and out again); a
     * module of the program started by name (it then stays running).
     */
    DeleteVar(ADMIN_SWITCH, GVF_GLOBAL_ONLY);
    if (builtin) {
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, ADMIN_MODULE, NULL), 0);
    } else {
        struct List *l;

        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, ADMIN_LIBRARY, NULL), 0);
        CU_ASSERT_EQUAL(admin_out(), 0);
        /* and it is loaded on demand again */
        l = ObtainInterfaceList();
        CU_ASSERT_PTR_NOT_NULL(l);
        CU_ASSERT(admin_running());
        if (l != NULL)
            ReleaseInterfaceList(l);
        CU_ASSERT_EQUAL(admin_out(), 0);
    }
}

/* The module can be had at all (installed, or part of the program) and the stack loads it on demand. */
static int admin_usable(void)
{
    ULONG flags = 0;

    DeleteVar(ADMIN_SWITCH, GVF_GLOBAL_ONLY);
    if (FindPort(AROSTCP_CTRLPORT_NAME) == NULL) {
        printf("SKIP %s: the stack has no control port\n", PROGRAM);
        return -1;
    }
    if (module_cmd(NCMD_MODULE_DEMAND, ADMIN_MODULE, NULL) != 0 ||
            module_cmd(NCMD_MODULE_QUERY, ADMIN_MODULE, &flags) != 0 || !(flags & NCMMF_STARTED)) {
        printf("SKIP %s: module %s cannot be had (%s not installed?)\n",
               PROGRAM, ADMIN_MODULE, ADMIN_LIBRARY);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    BYTE sig = -1;
    int was_running;

    first = argc > 1 && strcmp(argv[1], "--first") == 0;
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    if (st_skip_program(PROGRAM, ST_NONE)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    /* asked before anything here can load it */
    running_at_entry = admin_running();
    /* the module before this program: the library open, or nothing of it in memory */
    was_running = running_at_entry || lib_opencount(ADMIN_LIBRARY) >= 0;
    if (admin_usable() != 0) {
        CloseLibrary(SocketBase);
        return 0;
    }
    /* admin_usable() had it started: back to how the stack had it */
    if (!was_running)
        admin_out();

    if ((sig = AllocSignal(-1)) != -1) {
        sigmask = 1UL << sig;
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), sigmask, TAG_END);
    }

    if (CU_initialize_registry() != CUE_SUCCESS)
        goto out;
    p = CU_add_suite("ADMINMOD_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "it is not loaded when the stack has started", testADMIN_NOT_LOADED_AT_START, ST_NONE) ||
        !st_add_test(p, "socket(PF_ROUTE) loads it; a route change arrives as a message; "
                        "not unloaded while the socket is open", testADMIN_ROUTE_SOCKET, ST_ANY) ||
        !st_add_test(p, "GetRouteInfo() and the list calls load it; not unloaded while a list is out",
                     testADMIN_LISTS, ST_ANY) ||
        !st_add_test(p, "address changes signal programs without it, and do not load it",
                     testADMIN_ADDRSIG_WITHOUT_MODULE, ST_ANY) ||
        !st_add_test(p, "without it: the calls' answers, and the route and address ioctls work",
                     testADMIN_NOT_INSTALLED, ST_ANY))
    {
        CU_cleanup_registry();
        goto out;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-AdminMod");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
out:
    DeleteVar(ADMIN_SWITCH, GVF_GLOBAL_ONLY);
    if (sig != -1) {
        SocketBaseTags(SBTM_SETVAL(SBTC_SIG_ADDRESS_CHANGE_MASK), 0, TAG_END);
        FreeSignal(sig);
    }
    CloseLibrary(SocketBase);
    return CU_get_error();
}
