/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the modules the stack is built from, as seen through
    its control port (bsdsocket/netcontrol.h): a module is known by name,
    says whether it runs and how many users keep it from stopping, and a
    stop is refused rather than forced. The address families are modules,
    either of which may be left out of a running stack; the transports are
    one that the families need; the packet services ("packet": the filter
    and NAT engine, packet capture) are one that is started - its library
    loaded - by the first filter request, needs no family, and is not
    stopped while the filter is enabled or holds rules. Where the packet
    services are not installed (or ENV:AROSTCP/Without_packet is set) the
    filter requests fail with ENOENT and those tests say so and pass.

    The tests of the families run for each family the stack has, so the
    same assertions hold on a stack with both, with IPv4 only and with
    IPv6 only.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <exec/ports.h>
#include <net/ipfilter.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;

/* Send one module command; the stack's answer, or -1 when it has no control port. */
static LONG module_cmd(ULONG cmd, const char *name, ULONG *flags, LONG *users)
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
    if (users) *users = mm.ncmm_Users;
    return port ? mm.ncmm_Msg.ncm_Result : -1;
}

#define PACKET_MODULE   "packet"

static int filter_state(struct ipf_state *st)
{
    int s = socket(st_any_family(), SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    memset(st, 0, sizeof(*st));
    r = IoctlSocket(s, SIOCGETFS, (char *)st);
    CloseSocket(s);
    return r;
}

static int filter_enable(unsigned on)
{
    int s = socket(st_any_family(), SOCK_DGRAM, 0), r;

    if (s < 0)
        return -1;
    r = IoctlSocket(s, SIOCFRENB, (char *)&on);
    CloseSocket(s);
    return r;
}

static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

/*
 * The tests of the families are not about one family: they run for each
 * family the running stack has - "inet" and "inet6" on a stack with both,
 * the one there is on a stack with one. What a module says of itself
 * (permanent or not, part of the stack program or a library file) differs
 * between builds and is asked, not assumed; what is asserted is what holds
 * in every build: a family that runs is known and started, counts its
 * sockets and gives them back, and is not stopped while it is in use or
 * when it can never be stopped.
 */
static const int families[2] = { AF_INET, AF_INET6 };
#define NFAMILIES 2

static const char *family_module(int family)
{
    return family == AF_INET6 ? "inet6" : "inet";
}

/* Whether the module of that name is known and runs. */
static int module_started(const char *name)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, name, &flags, NULL) == 0 && (flags & NCMMF_STARTED);
}

static int families_started(void)
{
    int i, n = 0;

    for (i = 0; i < NFAMILIES; i++)
        n += module_started(family_module(families[i]));
    return n;
}

/*
 * A stop that must be refused, of a module that is in use: EPERM from one
 * that says it is never stopped, EBUSY from any other. Sent only when one
 * of the two is certain - a stop that went through would take the module
 * out of the stack for the rest of the run. `atleast' is what the caller
 * knows to be the least number of its users.
 */
static void stop_refused(const char *name, LONG atleast)
{
    ULONG flags = 0, after = 0;
    LONG users = -1, r;

    CU_ASSERT_EQUAL_FATAL(module_cmd(NCMD_MODULE_QUERY, name, &flags, &users), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(users >= atleast);
    if (!(flags & NCMMF_PERMANENT) && users < 1) {
        CU_FAIL("the module has no user and is not permanent: not asking it to stop");
        return;
    }
    r = module_cmd(NCMD_MODULE_STOP, name, &after, NULL);
    printf("\n    stop of %s (flags 0x%lx, %ld users): error %ld", name,
           (unsigned long)flags, (long)users, (long)r);
    if (flags & NCMMF_PERMANENT) {
        CU_ASSERT_EQUAL(r, EPERM);
    } else {
        CU_ASSERT_EQUAL(r, EBUSY);
    }
    CU_ASSERT(after & NCMMF_STARTED);
    CU_ASSERT(module_started(name));
}

/* Each family the stack has is a module: known by name and started. So is the routing domain. */
static void testMODULE_FAMILY_KNOWN(void)
{
    ULONG flags = 0;
    LONG users = -1;
    int i, n = 0;

    for (i = 0; i < NFAMILIES; i++) {
        const char *name = family_module(families[i]);
        LONG r = module_cmd(NCMD_MODULE_QUERY, name, &flags, &users);

        printf("\n    %s: query %ld, flags 0x%lx, %ld users", name, (long)r,
               (unsigned long)flags, (long)users);
        /* what makes sockets of the family is the started module, and nothing else */
        if (st_have_family(families[i])) {
            CU_ASSERT_EQUAL(r, 0);
            CU_ASSERT(flags & NCMMF_STARTED);
            n++;
        } else {
            CU_ASSERT(r == ENOENT || (r == 0 && !(flags & NCMMF_STARTED)));
        }
    }
    printf("\n");
    CU_ASSERT(n >= 1);          /* a stack with no family does not run */
    CU_ASSERT_EQUAL(n, families_started());
    /* the routing socket's module is "admin" now, loaded on first use */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, "route", &flags, NULL), ENOENT);
}

static void testMODULE_UNKNOWN(void)
{
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, "no-such-module", NULL, NULL), ENOENT);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "no-such-module", NULL, NULL), ENOENT);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, "", NULL, NULL), ENOENT);
}

/* Stopping a family that is in use is refused and changes nothing: sockets still work. */
static void testMODULE_FAMILY_STOP_REFUSED(void)
{
    ULONG flags = 0;
    int i, s, t;

    for (i = 0; i < NFAMILIES; i++) {
        const char *name = family_module(families[i]);

        if (!module_started(name))
            continue;
        s = socket(families[i], SOCK_STREAM, 0);        /* a user, whatever else there is */
        CU_ASSERT(s >= 0);
        if (s < 0)
            continue;
        stop_refused(name, 1);
        t = socket(families[i], SOCK_STREAM, 0);
        CU_ASSERT(t >= 0);
        if (t >= 0)
            CloseSocket(t);
        CloseSocket(s);
        /* starting what already runs is no error */
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, name, &flags, NULL), 0);
        CU_ASSERT(flags & NCMMF_STARTED);
    }
    printf("\n");
}

/* A family counts its sockets: each one opened is one more user, each one closed one fewer. */
static void testMODULE_FAMILY_COUNTS_SOCKETS(void)
{
    int f, s[4], i, made;

    for (f = 0; f < NFAMILIES; f++) {
        const char *name = family_module(families[f]);
        LONG before = -1, with4 = -1, after = -1;

        if (!module_started(name))
            continue;
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &before), 0);
        for (i = 0, made = 0; i < 4; i++) {
            s[i] = socket(families[f], i & 1 ? SOCK_DGRAM : SOCK_STREAM, 0);
            made += s[i] >= 0;
        }
        CU_ASSERT_EQUAL(made, 4);
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &with4), 0);
        for (i = 0; i < 4; i++)
            if (s[i] >= 0)
                CloseSocket(s[i]);
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &after), 0);
        CU_ASSERT_EQUAL(with4, before + 4);
        CU_ASSERT_EQUAL(after, before);
    }
}

/*
 * Either family is a module that may be left out. Where it is part of the
 * running stack its sockets work; where it was left out it has no users
 * (it may still be known by name, or not be known at all), a socket of the
 * family is refused - and the other family, which then must be there, goes
 * on working.
 */
static void present_or_left_out(int family)
{
    int other = family == AF_INET6 ? AF_INET : AF_INET6;
    ULONG flags = 0;
    LONG users = -1, r;
    int s, o;

    r = module_cmd(NCMD_MODULE_QUERY, family_module(family), &flags, &users);
    CU_ASSERT(r == 0 || r == ENOENT);
    s = socket(family, SOCK_STREAM, 0);
    if (r == 0 && (flags & NCMMF_STARTED)) {
        CU_ASSERT(s >= 0);
    } else {
        CU_ASSERT(s < 0);
        if (r == 0)
            CU_ASSERT_EQUAL(users, 0);
        CU_ASSERT(module_started(family_module(other)));
        o = socket(other, SOCK_STREAM, 0);
        CU_ASSERT(o >= 0);
        if (o >= 0)
            CloseSocket(o);
    }
    if (s >= 0)
        CloseSocket(s);
}

static void testMODULE_IPV6_PRESENT_OR_LEFT_OUT(void) { present_or_left_out(AF_INET6); }
static void testMODULE_IPV4_PRESENT_OR_LEFT_OUT(void) { present_or_left_out(AF_INET); }

/* A connection accepted by a listener is a socket of the family like any other. */
static void counts_accepted(int family)
{
    const char *name = family_module(family);
    LONG before = -1, during = -1, after = -1;
    unsigned short port;
    struct sockaddr_storage ss;
    socklen_t len;
    int l, c, a = -1;

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &before), 0);
    l = st_bound(family, SOCK_STREAM, &port);
    CU_ASSERT_FATAL(l >= 0);
    CU_ASSERT_EQUAL(listen(l, 4), 0);
    c = socket(family, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(c >= 0);
    len = st_loopback(family, &ss, port);
    CU_ASSERT_EQUAL_FATAL(connect(c, (struct sockaddr *)&ss, len), 0);
    CU_ASSERT_FATAL(sock_wait(l, 0, 3) > 0);        /* else accept() waits for ever */
    a = accept(l, NULL, NULL);
    CU_ASSERT(a >= 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &during), 0);
    CU_ASSERT_EQUAL(during, before + 3);
    /*
     * A connection closed in the normal way keeps its socket for the
     * TIME_WAIT period, and is a user of the family for that long. Reset
     * both ends instead, which releases them at once.
     */
    {
        struct linger lg = { 1, 0 };

        setsockopt(c, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
        if (a >= 0)
            setsockopt(a, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    }
    if (a >= 0) CloseSocket(a);
    CloseSocket(c);
    CloseSocket(l);
    Delay(10);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, name, NULL, &after), 0);
    CU_ASSERT_EQUAL(after, before);
}

static void testMODULE_COUNTS_ACCEPTED(void)
{
    int i;

    for (i = 0; i < NFAMILIES; i++)
        if (module_started(family_module(families[i])))
            counts_accepted(families[i]);
}

/*
 * TCP and UDP are a module of their own, "transport", which the families
 * need: it is known, it runs whenever a family does, each family that runs
 * is one of its users, and so it cannot be stopped while one runs.
 */
static void testMODULE_TRANSPORT_HELD(void)
{
    ULONG flags = 0;
    LONG users = -1;
    int n = families_started();

    CU_ASSERT(n >= 1);
    CU_ASSERT_EQUAL_FATAL(module_cmd(NCMD_MODULE_QUERY, "transport", &flags, &users), 0);
    printf("\n    transport: flags 0x%lx, %ld users, %d families started",
           (unsigned long)flags, (long)users, n);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(users >= n);
    if (n >= 1)
        stop_refused("transport", n);
    printf("\n");
    /* and TCP and UDP go on working */
    {
        int family = st_any_family(), c = -1, s = -1, u;

        CU_ASSERT_EQUAL(st_tcp_pair(family, &c, &s), 0);
        if (c >= 0) CloseSocket(c);
        if (s >= 0) CloseSocket(s);
        u = socket(family, SOCK_DGRAM, 0);
        CU_ASSERT(u >= 0);
        if (u >= 0) CloseSocket(u);
    }
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, "transport", &flags, NULL), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
}

/*
 * The packet services. A filter request starts the module "packet" - and
 * loads packet.atcp - when it does not run; where it cannot be
 * had (not installed, or its test switch set) the request fails with
 * ENOENT and the tests below only say so. A module that is not loaded is
 * not known by name (ENOENT) - the same, for these tests, as known and
 * not started.
 */
static int packet_missing(void)
{
    struct ipf_state st;
    int s = socket(st_any_family(), SOCK_DGRAM, 0);
    LONG r, err = 0;

    if (s < 0)
        return 1;
    memset(&st, 0, sizeof(st));
    r = IoctlSocket(s, SIOCGETFS, (char *)&st);
    if (r != 0)
        err = Errno();
    CloseSocket(s);
    if (r != 0) {
        printf("\n    the packet services cannot be had here (errno %ld): not tested",
               (long)err);
        CU_ASSERT_EQUAL(err, ENOENT);
        return 1;
    }
    return 0;
}

/* stop the module: done, or not even known (not loaded) */
static void packet_stop(void)
{
    LONG r = module_cmd(NCMD_MODULE_STOP, PACKET_MODULE, NULL, NULL);

    CU_ASSERT(r == 0 || r == ENOENT);
}

static int packet_started(void)
{
    return module_started(PACKET_MODULE);
}

/* Somebody's firewall runs here: the tests below must not take it down. */
static int filter_in_use(void)
{
    struct ipf_state st;

    if (packet_started() && filter_state(&st) == 0 && (st.enabled || st.rule_count != 0)) {
        printf("\n    the filter is in use here: not tested");
        return 1;
    }
    return 0;
}

/* The packet services: not running until asked for, started by a filter request, stopped on request. */
static void testMODULE_FILTER_ON_DEMAND(void)
{
    struct ipf_state st;
    ULONG flags = ~(ULONG)0;
    LONG r;

    if (filter_in_use() || packet_missing())
        return;
    packet_stop();
    r = module_cmd(NCMD_MODULE_QUERY, PACKET_MODULE, &flags, NULL);
    CU_ASSERT(r == 0 || r == ENOENT);
    if (r == 0) {
        CU_ASSERT_FALSE(flags & NCMMF_STARTED);
        CU_ASSERT_FALSE(flags & NCMMF_PERMANENT);
    }

    CU_ASSERT_EQUAL(filter_state(&st), 0);          /* first request starts it */
    CU_ASSERT_EQUAL(st.enabled, 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, PACKET_MODULE, &flags, NULL), 0);
    CU_ASSERT(flags & NCMMF_STARTED);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, PACKET_MODULE, &flags, NULL), 0);
    CU_ASSERT_FALSE(flags & NCMMF_STARTED);
    packet_stop();                                  /* stopping twice is no error */
}

/*
 * The packet services need no family: they hook the packet points of
 * whichever families run, so starting them adds no user to a family (and
 * on a stack with one family they start all the same).
 */
static void testMODULE_PACKET_NEEDS_NO_FAMILY(void)
{
    struct ipf_state st;
    const char *fam = family_module(st_any_family());
    LONG before = -1, during = -1, after = -1;

    if (filter_in_use() || packet_missing())
        return;
    packet_stop();
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, fam, NULL, &before), 0);
    CU_ASSERT_EQUAL(filter_state(&st), 0);                      /* starts the module */
    CU_ASSERT(packet_started());
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, fam, NULL, &during), 0);
    packet_stop();
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, fam, NULL, &after), 0);
    CU_ASSERT_EQUAL(during, before);
    CU_ASSERT_EQUAL(after, before);
}

/*
 * A filter that is enabled is not stopped with its module: the stop is
 * refused (EBUSY) and filtering stays on; disabled, the module stops, and
 * the stack passes traffic.
 */
static void testMODULE_FILTER_STOP_REFUSED(void)
{
    struct ipf_state st;
    unsigned short port;
    struct sockaddr_storage ss;
    socklen_t len;
    int family = st_any_family();
    char b = 'x';
    int r, w;

    if (filter_in_use() || packet_missing())
        return;
    CU_ASSERT_EQUAL(filter_enable(1), 0);
    CU_ASSERT_EQUAL(filter_state(&st), 0);
    CU_ASSERT_EQUAL(st.enabled, 1);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, PACKET_MODULE, NULL, NULL), EBUSY);
    CU_ASSERT(packet_started());
    CU_ASSERT_EQUAL(filter_state(&st), 0);
    CU_ASSERT_EQUAL(st.enabled, 1);

    CU_ASSERT_EQUAL(filter_enable(0), 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, PACKET_MODULE, NULL, NULL), 0);

    r = st_bound(family, SOCK_DGRAM, &port);
    w = socket(family, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(r >= 0 && w >= 0);
    len = st_loopback(family, &ss, port);
    CU_ASSERT_EQUAL(sendto(w, &b, 1, 0, (struct sockaddr *)&ss, len), 1);
    {
        struct timeval tv = { 2, 0 };
        setsockopt(r, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    CU_ASSERT_EQUAL(recv(r, &b, 1, 0), 1);
    CloseSocket(w);
    CloseSocket(r);

    CU_ASSERT_EQUAL(filter_state(&st), 0);          /* starts it again, as new */
    CU_ASSERT_EQUAL(st.enabled, 0);
    CU_ASSERT_EQUAL(st.rule_count, 0);
    packet_stop();
}

/* The same request as filter_state(), on a socket of the given family. */
static int filter_state_on(int family, struct ipf_state *st, LONG *err)
{
    int s = socket(family, SOCK_DGRAM, 0), r;

    *err = 0;
    if (s < 0)
        return -2;
    memset(st, 0, sizeof(*st));
    r = IoctlSocket(s, SIOCGETFS, (char *)st);
    if (r != 0)
        *err = Errno();
    CloseSocket(s);
    return r;
}

/*
 * A module does not bring a family into a stack that was configured
 * without it. On a stack with one family, a filter request on a socket of
 * that family starts the packet services, which need no family; the
 * family that was left out is still not running and has no users, the
 * family that runs has the users it had - and the same again for a second
 * request. (Added to the suite only on a stack with one family.)
 */
static void testMODULE_LEFT_OUT_NOT_STARTED(void)
{
    int present = st_have_family(AF_INET) ? AF_INET : AF_INET6;
    int absent = present == AF_INET ? AF_INET6 : AF_INET;
    struct ipf_state st;
    ULONG flags = 0;
    LONG users = -1, before = -1, after = -1, err = 0, r;
    int i, s;

    CU_ASSERT_FALSE_FATAL(module_started(family_module(absent)));
    if (filter_in_use())
        return;
    packet_stop();
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, family_module(present), NULL, &before), 0);

    for (i = 0; i < 2; i++) {
        r = filter_state_on(present, &st, &err);
        printf("\n    filter request on a stack without %s: %ld, errno %ld",
               family_module(absent), (long)r, (long)err);
        if (r != 0) {
            /* the packet services cannot be had here: that is all */
            CU_ASSERT_EQUAL(err, ENOENT);
            CU_ASSERT_FALSE(packet_started());
        } else {
            /* they need neither family: the request is answered on either stack */
            packet_stop();
        }

        /* the family that was left out: not started, by name or by socket */
        flags = 0;
        users = -1;
        r = module_cmd(NCMD_MODULE_QUERY, family_module(absent), &flags, &users);
        CU_ASSERT(r == 0 || r == ENOENT);       /* known by name, or not even that */
        if (r == 0) {
            CU_ASSERT_FALSE(flags & NCMMF_STARTED);
            CU_ASSERT_EQUAL(users, 0);
        }
        s = socket(absent, SOCK_DGRAM, 0);
        CU_ASSERT(s < 0);
        if (s >= 0)
            CloseSocket(s);
    }
    printf("\n");

    /* and the family that runs was not left holding anything */
    CU_ASSERT(module_started(family_module(present)));
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, family_module(present), NULL, &after), 0);
    CU_ASSERT_EQUAL(after, before);
}

/*
 * A running stack keeps one protocol family: its start-up refuses to go on
 * without one, and so the last family that runs is not stopped - EBUSY,
 * whether it has users or not - and goes on making sockets. (Added to the
 * suite only on a stack with one family, where that family is the last;
 * with both, cunit-net-modstop stops one and then asks the same of the
 * other. A stack that cannot stop a family at all says EPERM.)
 */
static void testMODULE_LAST_FAMILY_STAYS(void)
{
    int present = st_have_family(AF_INET) ? AF_INET : AF_INET6;
    const char *name = family_module(present);
    ULONG flags = 0, after = 0;
    LONG users = -1, r;
    int s;

    CU_ASSERT_EQUAL_FATAL(module_cmd(NCMD_MODULE_QUERY, name, &flags, &users), 0);
    CU_ASSERT_FATAL(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(families_started(), 1);
    r = module_cmd(NCMD_MODULE_STOP, name, &after, NULL);
    printf("\n    stop of %s, the only family (flags 0x%lx, %ld users): error %ld\n", name,
           (unsigned long)flags, (long)users, (long)r);
    if (r == 0)         /* it went: bring it back before anything else is said */
        (void)module_cmd(NCMD_MODULE_START, name, NULL, NULL);
    CU_ASSERT_EQUAL(r, (flags & NCMMF_PERMANENT) ? EPERM : EBUSY);
    CU_ASSERT(after & NCMMF_STARTED);
    CU_ASSERT(module_started(name));
    s = socket(present, SOCK_STREAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CloseSocket(s);
    /* and the transports, which only the family holds, stay with it */
    CU_ASSERT(module_started("transport"));
}

int main(int argc, char **argv)
{
    CU_pSuite p;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MODULE_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "the families are modules: known, started", testMODULE_FAMILY_KNOWN, ST_ANY) ||
        !st_add_test(p, "an unknown module is reported as such", testMODULE_UNKNOWN, ST_NONE) ||
        !st_add_test(p, "stopping a family in use is refused", testMODULE_FAMILY_STOP_REFUSED, ST_ANY) ||
        !st_add_test(p, "a family counts its sockets", testMODULE_FAMILY_COUNTS_SOCKETS, ST_ANY) ||
        !st_add_test(p, "accepted connections are counted and released", testMODULE_COUNTS_ACCEPTED, ST_ANY) ||
        !st_add_test(p, "IPv6 is a module: present, or left out and refused", testMODULE_IPV6_PRESENT_OR_LEFT_OUT, ST_NONE) ||
        !st_add_test(p, "IPv4 is a module: present, or left out and refused", testMODULE_IPV4_PRESENT_OR_LEFT_OUT, ST_NONE) ||
        !st_add_test(p, "the transports are a module the families hold", testMODULE_TRANSPORT_HELD, ST_ANY) ||
        !st_add_test(p, "the packet services start on first use and stop", testMODULE_FILTER_ON_DEMAND, ST_ANY) ||
        !st_add_test(p, "the packet services need no family", testMODULE_PACKET_NEEDS_NO_FAMILY, ST_ANY) ||
        !st_add_test(p, "an enabled filter is not stopped with its module", testMODULE_FILTER_STOP_REFUSED, ST_ANY) ||
        /* on a stack with one family only; with both there is nothing left out */
        (st_have_family(AF_INET) != st_have_family(AF_INET6) &&
         !st_add_test(p, "a family left out is not started because a module needs it",
                      testMODULE_LEFT_OUT_NOT_STARTED, ST_ANY)) ||
        /* ... and that one family is the last, which stays */
        (st_have_family(AF_INET) != st_have_family(AF_INET6) &&
         !st_add_test(p, "the last family that runs is not stopped",
                      testMODULE_LAST_FAMILY_STAYS, ST_ANY)))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Module");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(SocketBase);
    return CU_get_error();
}
