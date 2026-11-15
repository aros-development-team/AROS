/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the packet services as a module loaded on first use
    ("packet", packet.atcp: the filter and NAT engine and packet
    capture; bsdsocket net/packet_ops.h in the stack):

    - unloaded, it is not running; at the start of the program it says
      whether a program before it had it loaded (run first in a fresh boot,
      the line says "not loaded");
    - bpf_open() loads it; a capture on an interface with taps (a SANA-II
      one: the loopback has none) sees a packet of each family the stack
      has; it is not unloaded while the capture is open, and is after
      bpf_close();
    - a filter request loads it; it is not unloaded while the filter is
      enabled, while a filter rule exists, while a NAT rule exists - and is
      after they are disabled and flushed;
    - with ENV:AROSTCP/Without_packet set it cannot be had: the filter
      requests and bpf_open() fail with ENOENT, the other capture calls with
      EBADF, and traffic flows. It is not looked for again until it is
      loaded by name or the configuration is read again.

    Where the module cannot be unloaded because somebody uses it (a
    firewall, a capture) the tests say so and pass.
*/

/* the Roadshow capture calls of bsdsocket.library (bpf_open() ...) */
#define BSDSOCKET_ROADSHOW_BPF
#include "socktest.h"

#include <proto/dos.h>
#include <proto/miami.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <dos/var.h>
#include <net/if.h>
#include <net/bpf.h>
#include <net/ipfilter.h>
#include <netinet/in.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define PROGRAM         "cunit-net-packetmod"
#define PACKET_MODULE   "packet"
#define PACKET_LIBRARY  "packet.atcp"
#define PACKET_SWITCH   "AROSTCP/Without_packet"

static LONG module_cmd(ULONG cmd, const char *name, ULONG *flags)
{
    struct NetControlModuleMsg mm;
    struct MsgPort *reply, *port;

    if ((reply = CreateMsgPort()) == NULL)
        return -1;
    memset(&mm, 0, sizeof(mm));
    mm.ncmm_Msg.ncm_Msg.mn_ReplyPort = reply;
    mm.ncmm_Msg.ncm_Msg.mn_Length = name ? sizeof(mm) : sizeof(mm.ncmm_Msg);
    mm.ncmm_Msg.ncm_Command = cmd;
    mm.ncmm_Msg.ncm_Result = -1;
    if (name)
        strncpy(mm.ncmm_Name, name, sizeof(mm.ncmm_Name) - 1);
    Forbid();
    if ((port = FindPort((STRPTR)AROSTCP_CTRLPORT_NAME)) != NULL)
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

/* whether the module runs now */
static int packet_running(void)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, PACKET_MODULE, &flags) == 0 && (flags & NCMMF_STARTED);
}

/*
 * Take the module down: unloaded (a library file), or stopped (part of the
 * stack program, which UNLOAD answers with EPERM). 0, ENOENT when it was not
 * there at all, or why not (EBUSY).
 */
static LONG packet_down(void)
{
    LONG r = module_cmd(NCMD_MODULE_UNLOAD, PACKET_MODULE, NULL);

    if (r == EPERM)
        r = module_cmd(NCMD_MODULE_STOP, PACKET_MODULE, NULL);
    return r;
}

/* down, or say why not: then the test leaves */
static int packet_down_or_say(void)
{
    LONG r = packet_down();

    if (r != 0 && r != ENOENT) {
        printf("\n    the packet services are in use here (error %ld): not tested", (long)r);
        return 0;
    }
    CU_ASSERT_FALSE(packet_running());
    return 1;
}

static LONG filter_req(ULONG cmd, void *data)
{
    int s = socket(st_any_family(), SOCK_DGRAM, 0);
    LONG r;

    if (s < 0)
        return -1;
    r = IoctlSocket(s, cmd, (char *)data) == 0 ? 0 : Errno();
    CloseSocket(s);
    return r;
}

/* n datagrams over the loopback, each received: how many */
static int udp_loop(int family, int n)
{
    unsigned short port;
    struct sockaddr_storage ss;
    socklen_t len;
    int r = st_bound(family, SOCK_DGRAM, &port), w = socket(family, SOCK_DGRAM, 0), i, got = 0;
    char b = 'x';

    if (r >= 0 && w >= 0) {
        len = st_loopback(family, &ss, port);
        for (i = 0; i < n; i++)
            if (sendto(w, &b, 1, 0, (struct sockaddr *)&ss, len) == 1 &&
                    sock_wait(r, 0, 2) > 0 && recv(r, &b, 1, 0) == 1)
                got++;
    }
    if (r >= 0) CloseSocket(r);
    if (w >= 0) CloseSocket(w);
    return got;
}

static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

/* Taken down, it is not running; a filter request brings it back. */
static void testPACKET_NOT_LOADED(void)
{
    struct ipf_state st;

    if (!packet_down_or_say())
        return;
    CU_ASSERT_EQUAL(filter_req(SIOCGETFS, &st), 0);
    CU_ASSERT(packet_running());
    CU_ASSERT_EQUAL(packet_down(), 0);
    CU_ASSERT_FALSE(packet_running());
}

/*
 * An interface with taps: the first whose name BIOCSETIF takes (the
 * loopback has none and is refused). Its name in ifname; 0 if none.
 */
static int capture_interface(long bpf, char *ifname)
{
    char buf[1024];
    struct ifconf ifc;
    char *p;
    int s = socket(st_any_family(), SOCK_DGRAM, 0), found = 0;

    CU_ASSERT(bpf_ioctl(bpf, BIOCSETIF, (char *)"lo0") != 0);  /* no taps on the loopback */
    if (s < 0)
        return 0;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (IoctlSocket(s, SIOCGIFCONF, (char *)&ifc) == 0) {
        for (p = buf; !found && p < buf + ifc.ifc_len; ) {
            struct ifreq *ifr = (struct ifreq *)p;
            int alen = ifr->ifr_addr.sa_len > sizeof(struct sockaddr) ?
                       ifr->ifr_addr.sa_len : sizeof(struct sockaddr);

            if (strncmp(ifr->ifr_name, "lo", 2) != 0 &&
                    bpf_ioctl(bpf, BIOCSETIF, ifr->ifr_name) == 0) {
                strncpy(ifname, ifr->ifr_name, IFNAMSIZ);
                ifname[IFNAMSIZ - 1] = '\0';
                found = 1;
            }
            p += sizeof(ifr->ifr_name) + alen;
        }
    }
    CloseSocket(s);
    return found;
}

/* a datagram of the family out of the interface: IPv4 to its broadcast, IPv6 to ff02::1 */
static void send_out(int family, const char *ifname)
{
    int s = socket(family, SOCK_DGRAM, 0), on = 1;
    char b = 'x';

    if (s < 0)
        return;
    if (family == AF_INET) {
        struct ifreq ifr;
        struct sockaddr_in to;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
        if (IoctlSocket(s, SIOCGIFBRDADDR, (char *)&ifr) == 0) {
            setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
            memcpy(&to, &ifr.ifr_broadaddr, sizeof(to));
            to.sin_port = htons(9);
            sendto(s, &b, 1, 0, (struct sockaddr *)&to, sizeof(to));
        }
    } else {
        struct sockaddr_in6 to;
        LONG idx = MiamiBase ? if_nametoindex((STRPTR)ifname) : 0;

        memset(&to, 0, sizeof(to));
        to.sin6_len = sizeof(to);
        to.sin6_family = AF_INET6;
        to.sin6_port = htons(9);
        to.sin6_addr.s6_addr[0] = 0xff;
        to.sin6_addr.s6_addr[1] = 0x02;
        to.sin6_addr.s6_addr[15] = 0x01;
        to.sin6_scope_id = idx;
        if (idx > 0) {
            setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &idx, sizeof(idx));
            sendto(s, &b, 1, 0, (struct sockaddr *)&to, sizeof(to));
        }
    }
    CloseSocket(s);
}

/* bpf_open() loads it; the capture sees each family; open, it stays; closed, it goes. */
static void testPACKET_CAPTURE(void)
{
    static unsigned char buf[16384];
    char ifname[IFNAMSIZ];
    u_int one = 1, dlt = 0;
    int v4 = 0, v6 = 0, i;
    long bpf, n;

    if (!packet_down_or_say())
        return;
    bpf = bpf_open(0);
    CU_ASSERT_FATAL(bpf >= 0);
    CU_ASSERT(packet_running());
    CU_ASSERT_EQUAL(packet_down(), EBUSY);          /* a capture is open */
    CU_ASSERT(packet_running());

    if (!capture_interface(bpf, ifname)) {
        printf("\n    no interface with taps (only the loopback): nothing captured");
    } else {
        bpf_ioctl(bpf, BIOCIMMEDIATE, (char *)&one);
        bpf_ioctl(bpf, BIOCGDLT, (char *)&dlt);
        for (i = 0; i < 3 && !((v4 || !st_have_family(AF_INET)) &&
                                (v6 || !st_have_family(AF_INET6))); i++) {
            if (st_have_family(AF_INET))
                send_out(AF_INET, ifname);
            if (st_have_family(AF_INET6))
                send_out(AF_INET6, ifname);
            Delay(25);
            while ((n = bpf_read(bpf, buf, sizeof(buf))) > 0) {
                unsigned char *p = buf;

                while (p < buf + n) {
                    struct bpf_hdr *h = (struct bpf_hdr *)p;
                    unsigned char *pkt = p + h->bh_hdrlen;
                    int type = -1;

                    if (dlt == DLT_EN10MB && h->bh_caplen >= 14)
                        type = (pkt[12] << 8) | pkt[13];
                    else if (h->bh_caplen >= 1)
                        type = (pkt[0] >> 4) == 6 ? 0x86dd : (pkt[0] >> 4) == 4 ? 0x0800 : -1;
                    if (type == 0x0800) v4++;
                    if (type == 0x86dd) v6++;
                    p += BPF_WORDALIGN(h->bh_hdrlen + h->bh_caplen);
                }
            }
        }
        printf("\n    %s (link type %u): %d IPv4, %d IPv6 captured", ifname, dlt, v4, v6);
        if (st_have_family(AF_INET))
            CU_ASSERT(v4 > 0);
        if (st_have_family(AF_INET6))
            CU_ASSERT(v6 > 0);
    }
    CU_ASSERT_EQUAL(bpf_close(bpf), 0);
    CU_ASSERT_EQUAL(packet_down(), 0);
    CU_ASSERT_FALSE(packet_running());
    /* not running: a handle is none */
    CU_ASSERT_EQUAL(bpf_close(bpf), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_FALSE(packet_running());
}

/* A filter request loads it; enabled, or with a rule, it stays. */
static void testPACKET_FILTER_KEEPS_IT(void)
{
    struct ipf_state st;
    struct ipf_rule r;
    struct ipnat_rule nr;
    u_int on = 1, off = 0;
    int ff = IPF_FLUSH_ALL, cleared = 0;

    if (!packet_down_or_say())
        return;
    CU_ASSERT_EQUAL(filter_req(SIOCGETFS, &st), 0);
    CU_ASSERT(packet_running());

    CU_ASSERT_EQUAL(filter_req(SIOCFRENB, &on), 0);
    CU_ASSERT_EQUAL(packet_down(), EBUSY);
    CU_ASSERT_EQUAL(filter_req(SIOCFRENB, &off), 0);

    memset(&r, 0, sizeof(r));
    r.action = IPF_PASS;
    r.dir = IPF_IN;
    r.icmp_type = r.icmp_code = -1;
    CU_ASSERT_EQUAL(filter_req(SIOCADAFR, &r), 0);
    CU_ASSERT_EQUAL(packet_down(), EBUSY);
    CU_ASSERT_EQUAL(filter_req(SIOCIPFFL, &ff), 0);

    memset(&nr, 0, sizeof(nr));
    nr.type = IPNAT_MAP;
    CU_ASSERT_EQUAL(filter_req(SIOCADNAT, &nr), 0);
    CU_ASSERT_EQUAL(packet_down(), EBUSY);
    CU_ASSERT_EQUAL(filter_req(SIOCFLNAT, &cleared), 0);
    CU_ASSERT_EQUAL(cleared, 1);

    CU_ASSERT(packet_running());
    CU_ASSERT_EQUAL(packet_down(), 0);
    CU_ASSERT_FALSE(packet_running());
}

/* With the test switch set it cannot be had; traffic flows; it comes back when asked by name. */
static void testPACKET_NOT_INSTALLED(void)
{
    struct ipf_state st;
    char b[4];
    LONG r;

    if (!packet_down_or_say())
        return;
    CU_ASSERT_FATAL(SetVar((STRPTR)PACKET_SWITCH, (STRPTR)"1", 1, GVF_GLOBAL_ONLY) != 0);

    CU_ASSERT_EQUAL(filter_req(SIOCGETFS, &st), ENOENT);
    CU_ASSERT_EQUAL(bpf_open(0), -1);
    CU_ASSERT_EQUAL(Errno(), ENOENT);
    CU_ASSERT_EQUAL(bpf_close(0), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_read(0, b, sizeof(b)), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_write(0, b, sizeof(b)), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_set_notify_mask(0, 0), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_set_interrupt_mask(0, 0), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_ioctl(0, BIOCFLUSH, NULL), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_EQUAL(bpf_data_waiting(0), -1);
    CU_ASSERT_EQUAL(Errno(), EBADF);
    CU_ASSERT_FALSE(packet_running());

    /* packets pass, one tested word each */
    CU_ASSERT_EQUAL(udp_loop(st_any_family(), 4), 4);

    /* the switch is gone, but the stack does not look again ... */
    DeleteVar((STRPTR)PACKET_SWITCH, GVF_GLOBAL_ONLY);
    CU_ASSERT_EQUAL(filter_req(SIOCGETFS, &st), ENOENT);

    /* ... until it is loaded by name, or (part of the program) the configuration is read again */
    r = module_cmd(NCMD_MODULE_LOAD, PACKET_LIBRARY, NULL);
    printf("\n    loaded by name: %ld", (long)r);
    if (r != 0) {
        r = module_cmd(NCMD_RELOAD, NULL, NULL);
        printf(", configuration read again: %ld", (long)r);
        Delay(4 * 50);          /* duplicate address detection, for the programs after this one */
    }
    CU_ASSERT_EQUAL(filter_req(SIOCGETFS, &st), 0);
    CU_ASSERT(packet_running());
    CU_ASSERT_EQUAL(packet_down(), 0);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    ULONG flags = 0;
    LONG r;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    MiamiBase = OpenLibrary("miami.library", 0);
    DeleteVar((STRPTR)PACKET_SWITCH, GVF_GLOBAL_ONLY);

    r = module_cmd(NCMD_MODULE_QUERY, PACKET_MODULE, &flags);
    printf("%s: at program start the packet services are %s\n", PROGRAM,
           r == ENOENT ? "not loaded" :
           r != 0 ? "not to be asked about (no control port)" :
           (flags & NCMMF_STARTED) ? "running (a program before this one used them)" :
           "known and not running");

    if (CU_initialize_registry() != CUE_SUCCESS) {
        if (MiamiBase) CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PACKETMOD_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "taken down it is not running; a request brings it", testPACKET_NOT_LOADED,
                     ST_ANY) ||
        !st_add_test(p, "bpf_open() loads it; an open capture keeps it", testPACKET_CAPTURE,
                     ST_ANY) ||
        !st_add_test(p, "an enabled filter or a rule keeps it", testPACKET_FILTER_KEEPS_IT, ST_ANY) ||
        !st_add_test(p, "not installed: the calls fail as designed, traffic flows",
                     testPACKET_NOT_INSTALLED, ST_ANY))
    {
        CU_cleanup_registry();
        if (MiamiBase) CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Packetmod");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    DeleteVar((STRPTR)PACKET_SWITCH, GVF_GLOBAL_ONLY);
    if (MiamiBase) CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
