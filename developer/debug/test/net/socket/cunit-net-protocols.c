/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests of what a running configuration of the stack looks like
    from an application. Which protocol families a stack runs is
    configuration (PROTOCOLS=INET,INET6 / INET / INET6 in db/general.config,
    both when the item is absent), and every test here holds in all three:

    - at least one family runs, and the transports (TCP and UDP) with it;
    - of a family that runs: stream, datagram and raw sockets are made, a
      TCP connection and a UDP datagram go over its loopback address, and
      getaddrinfo() gives its numeric addresses;
    - of a family that does not run: every socket() is refused, with one
      error whatever the type, getaddrinfo() asked for that family fails,
      an address of that family is refused by a socket of the other one -
      and the other family goes on working;
    - an AF_INET6 listener with IPV6_V6ONLY off takes IPv4 connections
      when IPv4 runs too, and only then.

    Which families run is asked of the stack's control port (the modules
    "inet", "inet6" and "transport"), and printed in one line,
        CONFIG: inet=<0|1> inet6=<0|1> transport=<0|1>
    so that a log says what was tested. Usage:  cunit-net-protocols
*/

#define __CONFIG_ROADSHOW__     /* getaddrinfo() */
#include "socktest.h"

#include <netdb.h>
#include <exec/ports.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;

/*
 * What socket() fails with for a family that is not part of the running
 * stack. The family's domain is not attached, so socreate() finds no
 * protocol for the request (pffindtype()/pffindproto()) and says
 * EPROTONOSUPPORT, for every type and with or without a protocol number;
 * it never comes as far as the PCB layer, whose answer would be
 * EAFNOSUPPORT.
 */
#define NO_FAMILY_ERROR EPROTONOSUPPORT

static int have4, have6, havetr;        /* the modules that run: set in main() */

/* Whether the module of that name is known and runs; 0 also without a control port. */
static int module_started(const char *name)
{
    struct NetControlModuleMsg mm;
    struct MsgPort *reply, *port;

    if ((reply = CreateMsgPort()) == NULL)
        return 0;
    memset(&mm, 0, sizeof(mm));
    mm.ncmm_Msg.ncm_Msg.mn_ReplyPort = reply;
    mm.ncmm_Msg.ncm_Msg.mn_Length = sizeof(mm);
    mm.ncmm_Msg.ncm_Command = NCMD_MODULE_QUERY;
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
    return port != NULL && mm.ncmm_Msg.ncm_Result == 0 && (mm.ncmm_Flags & NCMMF_STARTED);
}

static int runs(int family)
{
    return family == AF_INET6 ? have6 : have4;
}

static int other_family(int family)
{
    return family == AF_INET6 ? AF_INET : AF_INET6;
}

static int icmp_of(int family)
{
    return family == AF_INET6 ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
}

static const char *literal_of(int family)
{
    return family == AF_INET6 ? "::1" : "127.0.0.1";
}

static void rcvtimeo(int s, int secs)
{
    struct timeval tv;

    tv.tv_sec = secs;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int init_suite(void) { return SocketBase != NULL ? 0 : -1; }
static int clean_suite(void) { return 0; }

/*
 * A stack with no family would have nothing a socket could be made of and
 * does not start, so one runs at least; the transports run because a
 * family needs them. What the modules say and what socket() does agree.
 */
static void testPROTOCOLS_ONE_AT_LEAST(void)
{
    CU_ASSERT(have4 || have6);
    CU_ASSERT(havetr);
    CU_ASSERT_EQUAL(st_have_family(AF_INET), have4);
    CU_ASSERT_EQUAL(st_have_family(AF_INET6), have6);
}

/*
 * socket() of a family: of one that runs, every type is made; of one that
 * does not, every request is refused with the same error, again and again,
 * and the family that does run is none the worse for it.
 */
static void sockets_of(int family)
{
    static const struct { int type, proto; const char *what; } want[] = {
        { SOCK_STREAM, 0, "stream" },
        { SOCK_DGRAM, 0, "datagram" },
        { SOCK_RAW, -1, "raw, the family's ICMP" },
        { SOCK_STREAM, IPPROTO_TCP, "stream, TCP by number" },
        { SOCK_DGRAM, IPPROTO_UDP, "datagram, UDP by number" },
    };
    int i, round, s, o;

    for (round = 0; round < (runs(family) ? 1 : 3); round++) {
        for (i = 0; i < (int)(sizeof(want) / sizeof(want[0])); i++) {
            int proto = want[i].proto < 0 ? icmp_of(family) : want[i].proto;
            LONG e = 0;

            s = socket(family, want[i].type, proto);
            if (s < 0)
                e = Errno();
            if (round == 0)
                printf("\n    socket(%s, %s): %s%ld", family == AF_INET6 ? "AF_INET6" : "AF_INET",
                       want[i].what, s >= 0 ? "descriptor " : "error ", (long)(s >= 0 ? s : e));
            if (runs(family)) {
                CU_ASSERT(s >= 0);
            } else {
                CU_ASSERT(s < 0);
                CU_ASSERT_EQUAL(e, NO_FAMILY_ERROR);
            }
            if (s >= 0)
                CloseSocket(s);
        }
    }
    printf("\n");
    if (runs(family))
        return;

    /* the other family: still there, and it does not take the absent one's addresses */
    o = socket(other_family(family), SOCK_DGRAM, 0);
    CU_ASSERT(o >= 0);
    if (o >= 0) {
        struct sockaddr_storage ss;
        socklen_t len = st_loopback(family, &ss, htons(9));
        LONG e;
        int r;

        r = sendto(o, "x", 1, 0, (struct sockaddr *)&ss, len);
        e = r < 0 ? Errno() : 0;
        printf("    a datagram to %s from a socket of the other family: result %d, error %ld\n",
               literal_of(family), r, (long)e);
        CU_ASSERT_EQUAL(r, -1);
        r = connect(o, (struct sockaddr *)&ss, len);
        CU_ASSERT_EQUAL(r, -1);
        CloseSocket(o);
    }
    o = socket(other_family(family), SOCK_STREAM, 0);
    CU_ASSERT(o >= 0);
    if (o >= 0) {
        struct sockaddr_storage ss;
        socklen_t len = st_loopback(family, &ss, 0);

        CU_ASSERT_EQUAL(bind(o, (struct sockaddr *)&ss, len), -1);
        CloseSocket(o);
    }
}

static void testPROTOCOLS_SOCKETS4(void) { sockets_of(AF_INET); }
static void testPROTOCOLS_SOCKETS6(void) { sockets_of(AF_INET6); }

/* A TCP connection over the family's loopback address: made, accepted, data both ways. */
static void tcp_over(int family)
{
    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    char b = 0;
    int c, s;

    CU_ASSERT_EQUAL_FATAL(st_tcp_pair(family, &c, &s), 0);
    rcvtimeo(c, 3);
    rcvtimeo(s, 3);
    bzero(&ss, sizeof(ss));
    CU_ASSERT_EQUAL(getpeername(s, (struct sockaddr *)&ss, &len), 0);
    CU_ASSERT_EQUAL(ss.ss_family, family);
    CU_ASSERT_EQUAL(send(c, "c", 1, 0), 1);
    CU_ASSERT_EQUAL(recv(s, &b, 1, 0), 1);
    CU_ASSERT_EQUAL(b, 'c');
    CU_ASSERT_EQUAL(send(s, "s", 1, 0), 1);
    CU_ASSERT_EQUAL(recv(c, &b, 1, 0), 1);
    CU_ASSERT_EQUAL(b, 's');
    CloseSocket(c);
    CloseSocket(s);
}

static void testPROTOCOLS_TCP4(void) { tcp_over(AF_INET); }
static void testPROTOCOLS_TCP6(void) { tcp_over(AF_INET6); }

/* A UDP datagram over the family's loopback address, and the answer to where it came from. */
static void udp_over(int family)
{
    struct sockaddr_storage ss, from;
    socklen_t len, flen = sizeof(from);
    unsigned short port = 0;
    char b = 0;
    int r, w;

    r = st_bound(family, SOCK_DGRAM, &port);
    w = socket(family, SOCK_DGRAM, 0);
    CU_ASSERT(r >= 0);
    CU_ASSERT(w >= 0);
    if (r >= 0 && w >= 0) {
        rcvtimeo(r, 3);
        rcvtimeo(w, 3);
        len = st_loopback(family, &ss, port);
        CU_ASSERT_EQUAL(sendto(w, "d", 1, 0, (struct sockaddr *)&ss, len), 1);
        bzero(&from, sizeof(from));
        CU_ASSERT_EQUAL(recvfrom(r, &b, 1, 0, (struct sockaddr *)&from, &flen), 1);
        CU_ASSERT_EQUAL(b, 'd');
        CU_ASSERT_EQUAL(from.ss_family, family);
        CU_ASSERT_EQUAL(sendto(r, "a", 1, 0, (struct sockaddr *)&from, flen), 1);
        CU_ASSERT_EQUAL(recv(w, &b, 1, 0), 1);
        CU_ASSERT_EQUAL(b, 'a');
    }
    if (r >= 0) CloseSocket(r);
    if (w >= 0) CloseSocket(w);
}

static void testPROTOCOLS_UDP4(void) { udp_over(AF_INET); }
static void testPROTOCOLS_UDP6(void) { udp_over(AF_INET6); }

/*
 * getaddrinfo() asked for one family, given a numeric address of it: the
 * address when the family runs; an error and no list when it does not - an
 * application must not be handed an address it cannot make a socket for.
 * An address of the other family is no address of this one, whatever runs.
 */
static void numeric_of(int family)
{
    struct addrinfo hints, *res = NULL;
    int flags, r;

    for (flags = 0; flags <= AI_NUMERICHOST; flags += AI_NUMERICHOST) {
        bzero(&hints, sizeof(hints));
        hints.ai_family = family;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = flags;
        res = NULL;
        r = getaddrinfo((char *)literal_of(family), "7", &hints, &res);
        printf("\n    getaddrinfo(\"%s\", %s%s): %d", literal_of(family),
               family == AF_INET6 ? "AF_INET6" : "AF_INET", flags ? ", AI_NUMERICHOST" : "", r);
        if (runs(family)) {
            CU_ASSERT_EQUAL(r, 0);
            CU_ASSERT_PTR_NOT_NULL(res);
            if (r == 0 && res != NULL) {
                CU_ASSERT_EQUAL(res->ai_family, family);
                CU_ASSERT_PTR_NULL(res->ai_next);
            }
        } else {
            CU_ASSERT_NOT_EQUAL(r, 0);
            CU_ASSERT_PTR_NULL(res);
        }
        if (r == 0 && res != NULL)
            freeaddrinfo(res);
    }
    printf("\n");

    bzero(&hints, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;
    res = NULL;
    r = getaddrinfo((char *)literal_of(other_family(family)), "7", &hints, &res);
    CU_ASSERT_NOT_EQUAL(r, 0);
    CU_ASSERT_PTR_NULL(res);
    if (r == 0 && res != NULL)
        freeaddrinfo(res);
}

static void testPROTOCOLS_GETADDRINFO4(void) { numeric_of(AF_INET); }
static void testPROTOCOLS_GETADDRINFO6(void) { numeric_of(AF_INET6); }

/*
 * An AF_INET6 socket listening on "any" with IPV6_V6ONLY off. With IPv4 in
 * the stack an IPv4 connection to it is accepted, its peer shown as an
 * IPv4-mapped address. Without IPv4 there is no IPv4 connection to be had:
 * no AF_INET socket to make one with, and an AF_INET6 socket that connects
 * to the mapped loopback address does not reach the listener either. An
 * IPv6 connection is accepted in both cases.
 */
static const unsigned char mapped_lo[16] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1 };

static void testPROTOCOLS_DUAL_LISTENER(void)
{
    struct sockaddr_storage ss;
    struct sockaddr_in6 a6;
    socklen_t len = sizeof(a6);
    unsigned short port;
    char b = 0;
    int l, c, a, r, on = 1, off = 0;

    l = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(l >= 0);
    setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    r = setsockopt(l, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
    printf("\n    IPV6_V6ONLY cleared on the listener: %s\n", r == 0 ? "yes" : "refused");
    if (have4)
        CU_ASSERT_EQUAL(r, 0);          /* refusing it is in order only without IPv4 */
    bzero(&a6, sizeof(a6));
    a6.sin6_family = AF_INET6;
    a6.sin6_len = sizeof(a6);
    CU_ASSERT_EQUAL_FATAL(bind(l, (struct sockaddr *)&a6, sizeof(a6)), 0);
    CU_ASSERT_EQUAL_FATAL(listen(l, 4), 0);
    CU_ASSERT_EQUAL_FATAL(getsockname(l, (struct sockaddr *)&a6, &len), 0);
    port = a6.sin6_port;

    c = socket(AF_INET, SOCK_STREAM, 0);
    if (have4) {
        CU_ASSERT_FATAL(c >= 0);
        IoctlSocket(c, FIONBIO, (char *)&on);
        len = st_loopback(AF_INET, &ss, port);
        r = connect(c, (struct sockaddr *)&ss, len);
        CU_ASSERT(r == 0 || Errno() == EINPROGRESS);
        CU_ASSERT(sock_wait(l, 0, 3) > 0);
        if (sock_wait(l, 0, 0) > 0) {
            len = sizeof(a6);
            bzero(&a6, sizeof(a6));
            a = accept(l, (struct sockaddr *)&a6, &len);
            CU_ASSERT(a >= 0);
            if (a >= 0) {
                CU_ASSERT_EQUAL(a6.sin6_family, AF_INET6);
                CU_ASSERT(memcmp(&a6.sin6_addr, mapped_lo, 16) == 0);
                CU_ASSERT(sock_wait(c, 1, 3) > 0);
                rcvtimeo(a, 3);
                CU_ASSERT_EQUAL(send(c, "4", 1, 0), 1);
                CU_ASSERT_EQUAL(recv(a, &b, 1, 0), 1);
                CU_ASSERT_EQUAL(b, '4');
                CloseSocket(a);
            }
        }
        CloseSocket(c);
    } else {
        CU_ASSERT(c < 0);
        if (c < 0) {
            CU_ASSERT_EQUAL(Errno(), NO_FAMILY_ERROR);
        } else
            CloseSocket(c);

        /* IPv4 by way of an AF_INET6 socket: it must not arrive */
        c = socket(AF_INET6, SOCK_STREAM, 0);
        CU_ASSERT_FATAL(c >= 0);
        setsockopt(c, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
        IoctlSocket(c, FIONBIO, (char *)&on);
        bzero(&a6, sizeof(a6));
        a6.sin6_family = AF_INET6;
        a6.sin6_len = sizeof(a6);
        a6.sin6_port = port;
        memcpy(&a6.sin6_addr, mapped_lo, 16);
        r = connect(c, (struct sockaddr *)&a6, sizeof(a6));
        printf("    connect() to ::ffff:127.0.0.1 without IPv4: result %d, error %ld\n",
               r, (long)(r != 0 ? Errno() : 0));
        CU_ASSERT_NOT_EQUAL(r, 0);
        r = sock_wait(l, 0, 2);
        CU_ASSERT(r <= 0);              /* nothing came to the listener */
        if (r > 0 && (a = accept(l, NULL, NULL)) >= 0)
            CloseSocket(a);             /* so that the IPv6 one below is the next */
        CloseSocket(c);
    }

    /* an IPv6 connection to the same listener */
    c = socket(AF_INET6, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    IoctlSocket(c, FIONBIO, (char *)&on);
    len = st_loopback(AF_INET6, &ss, port);
    r = connect(c, (struct sockaddr *)&ss, len);
    CU_ASSERT(r == 0 || Errno() == EINPROGRESS);
    CU_ASSERT(sock_wait(l, 0, 3) > 0);
    if (sock_wait(l, 0, 0) > 0) {
        len = sizeof(a6);
        bzero(&a6, sizeof(a6));
        a = accept(l, (struct sockaddr *)&a6, &len);
        CU_ASSERT(a >= 0);
        if (a >= 0) {
            CU_ASSERT(IN6_IS_ADDR_LOOPBACK(&a6.sin6_addr));
            CloseSocket(a);
        }
    }
    CloseSocket(c);
    CloseSocket(l);
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    have4 = module_started("inet");
    have6 = module_started("inet6");
    havetr = module_started("transport");
    printf("CONFIG: inet=%d inet6=%d transport=%d\n", have4, have6, havetr);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("PROTOCOLS_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "one family runs at least, and the transports with it", testPROTOCOLS_ONE_AT_LEAST, ST_NONE) ||
        !st_add_test(p, "IPv4 sockets: every type made, or every one refused alike", testPROTOCOLS_SOCKETS4, ST_NONE) ||
        !st_add_test(p, "IPv6 sockets: every type made, or every one refused alike", testPROTOCOLS_SOCKETS6, ST_NONE) ||
        !st_add_test(p, "a TCP connection over the IPv4 loopback", testPROTOCOLS_TCP4, ST_V4) ||
        !st_add_test(p, "a TCP connection over the IPv6 loopback", testPROTOCOLS_TCP6, ST_V6) ||
        !st_add_test(p, "a UDP datagram over the IPv4 loopback", testPROTOCOLS_UDP4, ST_V4) ||
        !st_add_test(p, "a UDP datagram over the IPv6 loopback", testPROTOCOLS_UDP6, ST_V6) ||
        !st_add_test(p, "getaddrinfo() for IPv4: its address, or none without the family", testPROTOCOLS_GETADDRINFO4, ST_NONE) ||
        !st_add_test(p, "getaddrinfo() for IPv6: its address, or none without the family", testPROTOCOLS_GETADDRINFO6, ST_NONE) ||
        !st_add_test(p, "an AF_INET6 listener takes IPv4 connections only when IPv4 runs", testPROTOCOLS_DUAL_LISTENER, ST_V6))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Protocols");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
