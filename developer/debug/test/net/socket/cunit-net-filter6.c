/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the packet filter with IPv6 rules (DEFECTS.md defect 4:
    IPv6 traffic was never filtered). Over the loopback:

    - a rule that blocks a TCP port refuses a connection to it over IPv4
      and, with the IPv6 rule beside it, over IPv6;
    - IPv6 rules by prefix (and negated), by a UDP port behind a
      destination-options header, by ICMPv6 type;
    - a fragment that is not the first carries no port: a port rule does
      not match it, in either family, and does match the first fragment;
    - rules of today's structure (struct ipf_rule) through today's ioctls
      work as they did - the view of an ipf built before IPv6 rules;
    - the rule count and the flush cover both families; the counters count;
    - on a stack with IPv6 alone the filter starts, filters, and does not
      bring IPv4 in.

    A rule that blocks outgoing packets makes the send fail with
    EHOSTUNREACH, which is what most cases look at. Where the packet
    services are not installed (or ENV:AROSTCP/Without_packet is set), and
    where somebody's firewall is running, the tests say so and pass.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <exec/ports.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/ipfilter.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;

#define PROGRAM "cunit-net-filter6"

static int fsock = -1;          /* the socket the filter requests go to */
static int usable = 0;          /* the filter can be had and is nobody's */

static LONG ipf_req(ULONG cmd, void *data)
{
    return IoctlSocket(fsock, cmd, (char *)data) == 0 ? 0 : Errno();
}

/* disabled, no rules, counters zero */
static void filter_reset(void)
{
    u_int off = 0;
    int all = IPF_FLUSH_ALL;
    struct ipf_state st;

    (void)ipf_req(SIOCFRENB, &off);
    (void)ipf_req(SIOCIPFFL, &all);
    (void)ipf_req(SIOCFRZST, &st);
}

static void filter_on(void)
{
    u_int on = 1;

    CU_ASSERT_EQUAL(ipf_req(SIOCFRENB, &on), 0);
}

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

/* rules: an IPv4 one and an IPv6 one, all fields "any" */
static void rule4_init(struct ipf_rule *r, int action, int dir, int proto)
{
    memset(r, 0, sizeof(*r));
    r->action = action;
    r->dir = dir;
    r->rflags = IPF_QUICK;
    r->proto = proto;
    r->icmp_type = r->icmp_code = -1;
}

static void rule6_init(struct ipf_rule6 *r, int action, int dir, int proto)
{
    memset(r, 0, sizeof(*r));
    r->size = sizeof(*r);
    r->action = action;
    r->dir = dir;
    r->rflags = IPF_QUICK;
    r->proto = proto;
    r->icmp_type = r->icmp_code = -1;
}

static const struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;

static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

#define NOT_USABLE() \
    do { if (!usable) { printf("\n    not tested here (see above)"); return; } } while (0)

/*
 * A connection to the port: established (1), or not within 2 s (0). The
 * listener is on the loopback of the family; the rule drops the SYN as it
 * arrives.
 */
static int tcp_reaches(int family, unsigned short nport, int lfd)
{
    struct sockaddr_storage ss;
    socklen_t len = st_loopback(family, &ss, nport);
    int c = socket(family, SOCK_STREAM, 0), on = 1, ok = 0, a;

    if (c < 0)
        return -1;
    IoctlSocket(c, FIONBIO, (char *)&on);
    if (connect(c, (struct sockaddr *)&ss, len) == 0 || Errno() == EINPROGRESS) {
        if (sock_wait(lfd, 0, 2) > 0) {
            len = sizeof(ss);
            if ((a = accept(lfd, (struct sockaddr *)&ss, &len)) >= 0) {
                ok = 1;
                CloseSocket(a);
            }
        }
    }
    CloseSocket(c);
    return ok;
}

/* The test of DEFECTS.md, for one family: a blocked port refuses; flushed, it accepts. */
static void tcp_port_blocked(int family)
{
    unsigned short nport;
    int lfd = st_bound(family, SOCK_STREAM, &nport);
    struct ipf_state st;
    LONG e;

    CU_ASSERT_FATAL(lfd >= 0);
    CU_ASSERT_FATAL(listen(lfd, 5) == 0);
    filter_reset();
    if (family == AF_INET6) {
        struct ipf_rule6 r;

        rule6_init(&r, IPF_BLOCK, IPF_IN, IPPROTO_TCP);
        r.dport_op = IPF_PORT_EQ;
        r.dport_lo = ntohs(nport);
        e = ipf_req(SIOCADAFR6, &r);
    } else {
        struct ipf_rule r;

        rule4_init(&r, IPF_BLOCK, IPF_IN, IPPROTO_TCP);
        r.dport_op = IPF_PORT_EQ;
        r.dport_lo = ntohs(nport);
        e = ipf_req(SIOCADAFR, &r);
    }
    CU_ASSERT_EQUAL(e, 0);
    filter_on();

    CU_ASSERT_EQUAL(tcp_reaches(family, nport, lfd), 0);
    memset(&st, 0, sizeof(st));
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    printf("\n    port %u blocked: %lu packets blocked", ntohs(nport), (unsigned long)st.blocked);
    CU_ASSERT(st.blocked >= 1);

    filter_reset();
    CU_ASSERT_EQUAL(tcp_reaches(family, nport, lfd), 1);
    CloseSocket(lfd);
}

static void testFILTER_TCP_PORT_V4(void)
{
    NOT_USABLE();
    tcp_port_blocked(AF_INET);
}

static void testFILTER_TCP_PORT_V6(void)
{
    NOT_USABLE();
    tcp_port_blocked(AF_INET6);
}

/* one UDP datagram to ::1: 0 sent and received, else the errno of the send (or -1) */
static LONG udp6_send(void)
{
    unsigned short nport;
    struct sockaddr_storage ss;
    socklen_t len;
    int r = st_bound(AF_INET6, SOCK_DGRAM, &nport), w = socket(AF_INET6, SOCK_DGRAM, 0);
    LONG res = -1;
    char b = 'x';

    if (r >= 0 && w >= 0) {
        len = st_loopback(AF_INET6, &ss, nport);
        if (sendto(w, &b, 1, 0, (struct sockaddr *)&ss, len) != 1)
            res = Errno();
        else if (sock_wait(r, 0, 2) > 0 && recv(r, &b, 1, 0) == 1)
            res = 0;
    }
    if (r >= 0) CloseSocket(r);
    if (w >= 0) CloseSocket(w);
    return res;
}

/* IPv6 rules by prefix: ::1/128 matches, 2001:db8::/32 does not, its negation does. */
static void testFILTER6_PREFIX(void)
{
    struct ipf_rule6 r;
    static const struct in6_addr doc = {{{ 0x20, 0x01, 0x0d, 0xb8 }}};

    NOT_USABLE();
    filter_reset();
    filter_on();
    CU_ASSERT_EQUAL(udp6_send(), 0);                    /* enabled, no rule: passes */

    rule6_init(&r, IPF_BLOCK, IPF_OUT, IPPROTO_UDP);
    r.dst_addr = lo6;
    r.dst_plen = 128;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    CU_ASSERT_EQUAL(udp6_send(), EHOSTUNREACH);
    CU_ASSERT_EQUAL(ipf_req(SIOCRMAFR6, &r), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCRMAFR6, &r), ESRCH);

    r.dst_addr = doc;
    r.dst_plen = 32;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    CU_ASSERT_EQUAL(udp6_send(), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCRMAFR6, &r), 0);

    r.dst_not = 1;                                      /* ! 2001:db8::/32 */
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    CU_ASSERT_EQUAL(udp6_send(), EHOSTUNREACH);

    /* a rule of another size, or a prefix longer than 128, is refused */
    r.size = sizeof(r) - 1;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), EINVAL);
    r.size = sizeof(r);
    r.dst_plen = 129;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), EINVAL);
    filter_reset();
}

/* a raw IPv6 packet to ::1 with the given next header and payload: 0 or the errno */
static LONG raw6_send(int nxt, const void *payload, int len)
{
    struct sockaddr_storage ss;
    socklen_t sl = st_loopback(AF_INET6, &ss, 0);
    int s = socket(AF_INET6, SOCK_RAW, nxt);
    LONG res;

    if (s < 0)
        return -1;
    res = sendto(s, payload, len, 0, (struct sockaddr *)&ss, sl) == len ? 0 : Errno();
    CloseSocket(s);
    return res;
}

/* A UDP port rule matches behind a destination-options header. */
static void testFILTER6_PORT_BEHIND_DSTOPTS(void)
{
    struct ipf_rule6 r;
    unsigned char pkt[8 + 8 + 1];
    unsigned short port = 7777;
    LONG e;

    NOT_USABLE();
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = IPPROTO_UDP;           /* destination options: next header, length 0 (8 bytes) */
    pkt[1] = 0;
    pkt[2] = 1;                     /* PadN, 4 bytes */
    pkt[3] = 4;
    pkt[8] = 0x04; pkt[9] = 0xd2;   /* UDP: source port 1234 */
    pkt[10] = port >> 8; pkt[11] = port & 0xff;
    pkt[13] = 9;                    /* length */

    filter_reset();
    rule6_init(&r, IPF_BLOCK, IPF_OUT, IPPROTO_UDP);
    r.dport_op = IPF_PORT_EQ;
    r.dport_lo = port;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    filter_on();
    e = raw6_send(IPPROTO_DSTOPTS, pkt, sizeof(pkt));
    if (e == -1) {
        printf("\n    no raw IPv6 socket of next header 60 here: not tested");
        filter_reset();
        return;
    }
    CU_ASSERT_EQUAL(e, EHOSTUNREACH);
    r.dport_lo = port + 1;          /* another port: passes */
    CU_ASSERT_EQUAL(ipf_req(SIOCIPFFL, &(int){ IPF_FLUSH_ALL }), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    CU_ASSERT_EQUAL(raw6_send(IPPROTO_DSTOPTS, pkt, sizeof(pkt)), 0);
    filter_reset();
}

/* An ICMPv6 type rule: echo requests blocked, echo replies not. */
static void testFILTER6_ICMP_TYPE(void)
{
    struct ipf_rule6 r;
    unsigned char m[8];

    NOT_USABLE();
    filter_reset();
    rule6_init(&r, IPF_BLOCK, IPF_OUT, IPPROTO_ICMPV6);
    r.dst_addr = lo6;
    r.dst_plen = 128;
    r.icmp_type = ICMP6_ECHO_REQUEST;
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
    filter_on();

    memset(m, 0, sizeof(m));
    m[0] = ICMP6_ECHO_REQUEST;
    CU_ASSERT_EQUAL(raw6_send(IPPROTO_ICMPV6, m, sizeof(m)), EHOSTUNREACH);
    m[0] = ICMP6_ECHO_REPLY;
    CU_ASSERT_EQUAL(raw6_send(IPPROTO_ICMPV6, m, sizeof(m)), 0);
    filter_reset();
}

/*
 * Fragments against a port rule. IPv6: a fragment header with offset 0
 * (more to come) before a UDP header with the port is blocked; one with
 * offset 8 whose bytes look the same is not - it carries no port. IPv4:
 * the same with a raw socket that writes its own header (IP_HDRINCL;
 * length and offset in host order, as BSD has them).
 */
static void testFILTER_LATER_FRAGMENT(void)
{
    unsigned short port = 7778;

    NOT_USABLE();
    filter_reset();
    if (st_have_family(AF_INET6)) {
        struct ipf_rule6 r;
        unsigned char pkt[8 + 8];
        LONG e;

        rule6_init(&r, IPF_BLOCK, IPF_OUT, IPPROTO_UDP);
        r.dport_op = IPF_PORT_EQ;
        r.dport_lo = port;
        CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r), 0);
        filter_on();
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = IPPROTO_UDP;
        pkt[7] = 42;                            /* identification */
        pkt[8] = 0x04; pkt[9] = 0xd2;
        pkt[10] = port >> 8; pkt[11] = port & 0xff;
        pkt[13] = 8;
        pkt[3] = 0x01;                          /* offset 0, more fragments */
        e = raw6_send(IPPROTO_FRAGMENT, pkt, sizeof(pkt));
        if (e == -1)
            printf("\n    no raw IPv6 socket of next header 44 here: IPv6 not tested");
        else {
            CU_ASSERT_EQUAL(e, EHOSTUNREACH);
            pkt[2] = 0x00; pkt[3] = 0x08;       /* offset 8, the last */
            CU_ASSERT_EQUAL(raw6_send(IPPROTO_FRAGMENT, pkt, sizeof(pkt)), 0);
        }
        filter_reset();
    }
    if (st_have_family(AF_INET)) {
        struct ipf_rule r;
        struct {
            struct ip       ip;
            unsigned char   udp[8];
        } pkt;
        struct sockaddr_in to;
        int s = socket(AF_INET, SOCK_RAW, IPPROTO_UDP), on = 1;
        LONG e;

        if (s < 0 || setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) != 0) {
            printf("\n    no raw IPv4 socket with IP_HDRINCL here: IPv4 not tested");
            if (s >= 0) CloseSocket(s);
            return;
        }
        rule4_init(&r, IPF_BLOCK, IPF_OUT, IPPROTO_UDP);
        r.dport_op = IPF_PORT_EQ;
        r.dport_lo = port;
        CU_ASSERT_EQUAL(ipf_req(SIOCADAFR, &r), 0);
        filter_on();

        memset(&pkt, 0, sizeof(pkt));
        pkt.ip.ip_v = IPVERSION;
        pkt.ip.ip_hl = sizeof(struct ip) >> 2;
        pkt.ip.ip_len = sizeof(pkt);
        pkt.ip.ip_ttl = 64;
        pkt.ip.ip_p = IPPROTO_UDP;
        pkt.ip.ip_src.s_addr = htonl(INADDR_LOOPBACK);
        pkt.ip.ip_dst.s_addr = htonl(INADDR_LOOPBACK);
        pkt.udp[0] = 0x04; pkt.udp[1] = 0xd2;
        pkt.udp[2] = port >> 8; pkt.udp[3] = port & 0xff;
        pkt.udp[5] = 8;
        memset(&to, 0, sizeof(to));
        to.sin_len = sizeof(to);
        to.sin_family = AF_INET;
        to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        pkt.ip.ip_off = IP_MF;                  /* the first fragment: has the port */
        e = sendto(s, &pkt, sizeof(pkt), 0, (struct sockaddr *)&to, sizeof(to)) == sizeof(pkt) ?
            0 : Errno();
        CU_ASSERT_EQUAL(e, EHOSTUNREACH);
        pkt.ip.ip_off = 1;                      /* offset 8: no port, however it looks */
        e = sendto(s, &pkt, sizeof(pkt), 0, (struct sockaddr *)&to, sizeof(to)) == sizeof(pkt) ?
            0 : Errno();
        if (e != 0)
            printf("\n    the later fragment was refused: errno %ld", (long)e);
        CU_ASSERT_EQUAL(e, 0);
        CloseSocket(s);
        filter_reset();
    }
}

/* The structure and requests of an ipf built before IPv6 rules. */
static void testFILTER_OLD_STRUCT(void)
{
    struct ipf_rule r;
    struct ipf_state st;

    NOT_USABLE();
    filter_reset();
    rule4_init(&r, IPF_PASS, IPF_IN, IPPROTO_UDP);
    r.src_addr.s_addr = htonl(0x0a000000);
    r.src_mask.s_addr = htonl(0xff000000);
    CU_ASSERT_EQUAL(ipf_req(SIOCADDFR, &r), 0);
    r.hits = 0;                                 /* insert at the head */
    r.action = IPF_BLOCK;
    CU_ASSERT_EQUAL(ipf_req(SIOCINAFR, &r), 0);
    memset(&st, 0, sizeof(st));
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    CU_ASSERT_EQUAL(st.rule_count, 2);
    CU_ASSERT_EQUAL(st.enabled, 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCDELFR, &r), 0);
    r.action = IPF_PASS;
    CU_ASSERT_EQUAL(ipf_req(SIOCDELFR, &r), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCDELFR, &r), ESRCH);
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    CU_ASSERT_EQUAL(st.rule_count, 0);
    filter_reset();
}

/* The count and the flush cover both families; the counters zero. */
static void testFILTER_COUNT_FLUSH(void)
{
    struct ipf_rule r4;
    struct ipf_rule6 r6;
    struct ipf_state st;
    int ff = IPF_FLUSH_ALL;

    NOT_USABLE();
    filter_reset();
    rule4_init(&r4, IPF_PASS, IPF_IN, IPF_PROTO_ANY);
    rule6_init(&r6, IPF_PASS, IPF_OUT, IPF_PROTO_ANY);
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR, &r4), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCADAFR6, &r6), 0);
    r6.dir = IPF_IN;
    r6.hits = 0;
    CU_ASSERT_EQUAL(ipf_req(SIOCINAFR6, &r6), 0);
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    CU_ASSERT_EQUAL(st.rule_count, 3);

    /* passed counted: a datagram of each family the stack has. The filter
     * is switched off before the counters are read and zeroed: with the
     * pass-all rules any packet of the system's own (background traffic on
     * the interface) is counted too, and must not land between the zeroing
     * and the read that checks it */
    filter_on();
    if (st_have_family(AF_INET6))
        CU_ASSERT_EQUAL(udp6_send(), 0);
    {
        u_int off = 0;

        CU_ASSERT_EQUAL(ipf_req(SIOCFRENB, &off), 0);
    }
    CU_ASSERT_EQUAL(ipf_req(SIOCFRZST, &st), 0);
    printf("\n    3 rules: %lu passed", (unsigned long)st.passed);
    if (st_have_family(AF_INET6))
        CU_ASSERT(st.passed >= 1);
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    CU_ASSERT_EQUAL(st.passed, 0);

    CU_ASSERT_EQUAL(ipf_req(SIOCIPFFL, &ff), 0);
    CU_ASSERT_EQUAL(ff, 3);
    CU_ASSERT_EQUAL(ipf_req(SIOCGETFS, &st), 0);
    CU_ASSERT_EQUAL(st.rule_count, 0);
    filter_reset();
}

/* With IPv6 alone the filter runs, and IPv4 stays out. */
static void testFILTER_IPV6_ONLY(void)
{
    ULONG flags = 0;
    LONG r;

    NOT_USABLE();
    tcp_port_blocked(AF_INET6);
    r = module_cmd(NCMD_MODULE_QUERY, "inet", &flags);
    CU_ASSERT(r == ENOENT || (r == 0 && !(flags & NCMMF_STARTED)));
    CU_ASSERT_FALSE(st_have_family(AF_INET));
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    struct ipf_state st;
    LONG e;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    if (st_skip_program(PROGRAM, ST_ANY)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    fsock = socket(st_any_family(), SOCK_DGRAM, 0);
    memset(&st, 0, sizeof(st));
    e = fsock >= 0 ? ipf_req(SIOCGETFS, &st) : -1;
    if (e != 0)
        printf("%s: the filter cannot be had here (errno %ld): its tests only say so\n",
               PROGRAM, (long)e);
    else if (st.enabled || st.rule_count != 0)
        printf("%s: the filter is in use here (somebody's firewall): its tests only say so\n",
               PROGRAM);
    else
        usable = 1;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("FILTER6_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "a blocked TCP port refuses over IPv4", testFILTER_TCP_PORT_V4, ST_V4) ||
        !st_add_test(p, "a blocked TCP port refuses over IPv6", testFILTER_TCP_PORT_V6, ST_V6) ||
        !st_add_test(p, "IPv6 rules by prefix", testFILTER6_PREFIX, ST_V6) ||
        !st_add_test(p, "an IPv6 port rule behind a destination-options header",
                     testFILTER6_PORT_BEHIND_DSTOPTS, ST_V6) ||
        !st_add_test(p, "an ICMPv6 type rule", testFILTER6_ICMP_TYPE, ST_V6) ||
        !st_add_test(p, "a later fragment has no port", testFILTER_LATER_FRAGMENT, ST_ANY) ||
        !st_add_test(p, "rules of the IPv4 structure as before", testFILTER_OLD_STRUCT, ST_ANY) ||
        !st_add_test(p, "count, counters and flush cover both families", testFILTER_COUNT_FLUSH,
                     ST_ANY) ||
        (!st_have_family(AF_INET) &&
         !st_add_test(p, "with IPv6 alone the filter runs without IPv4", testFILTER_IPV6_ONLY,
                      ST_V6)))
    {
        CU_cleanup_registry();
        if (fsock >= 0) CloseSocket(fsock);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Filter6");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    if (usable)
        filter_reset();
    if (fsock >= 0) CloseSocket(fsock);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
