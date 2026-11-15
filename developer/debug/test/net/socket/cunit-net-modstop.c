/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for stopping and starting the protocol modules of the stack
    while it runs, through its control port (bsdsocket/netcontrol.h): an
    address family - "inet", "inet6" - is not stopped while a socket of it
    exists, and is stopped when the last one is closed; stopped, the stack
    makes no socket of it and the other family goes on working; started
    again, its sockets work and its loopback address answers. The same for
    a family whose code is a library file of its own, which is unloaded and
    loaded again and is really gone from memory in between. The last family
    that runs is not stopped, and the transports are not stopped while a
    family runs. Done ten times over, free memory ends where it began.

    The program needs a stack with both families: with one, that one is the
    last and cannot be stopped. A family that other programs have sockets
    of, or that another module of the stack holds (the filter engine holds
    "inet"), cannot be stopped either: its tests are left out, and the line
    that says so begins with SKIP.

    Whatever a test stopped is started again before the program ends, also
    when an assertion failed, and the configuration is then read again
    (NCMD_RELOAD), which gives a family that was started while the stack
    ran the addresses and routes the configuration has for it.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;

#define PROGRAM         "cunit-net-modstop"

/* stop/start and unload/load cycles of the memory tests, and how far free
 * memory may be off after them */
#define CYCLES          10
#define DRIFT_MAX       (32 * 1024)

/* The two families, their modules, and the library files the modules are
 * in when they are not part of the stack program (working names). */
static const struct family {
    int         f_af;
    const char *f_module;
    const char *f_library;
    const char *f_what;
} fam4 = { AF_INET, "inet", "inet.prot", "IPv4" },
  fam6 = { AF_INET6, "inet6", "inet6.prot", "IPv6" };

static int stopped_something = 0;       /* the configuration is to be read again */

/*
 * Send one module command. `name' goes in and, when `out' is given, what
 * the stack left in the name field comes back (a load answers with the
 * module's name). The stack's answer, or -1 when it has no control port.
 */
static LONG module_cmd(ULONG cmd, const char *name, char *out, ULONG *flags, LONG *users)
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
    if (out) {
        mm.ncmm_Name[sizeof(mm.ncmm_Name) - 1] = '\0';
        strcpy(out, mm.ncmm_Name);
    }
    if (flags) *flags = mm.ncmm_Flags;
    if (users) *users = mm.ncmm_Users;
    return port ? mm.ncmm_Msg.ncm_Result : -1;
}

/* Read the configuration again, in place. */
static LONG net_reload(void)
{
    struct MsgPort *reply = CreateMsgPort(), *port;
    struct NetControlMsg msg;
    LONG rc = -1;

    if (reply == NULL)
        return -1;
    memset(&msg, 0, sizeof(msg));
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

/* Whether exec has a library of that name in memory, and how often it is
 * open: -1 when it is not there. */
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

/* Whether the module of that name is known and runs. */
static int module_started(const char *name)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, name, NULL, &flags, NULL) == 0 &&
           (flags & NCMMF_STARTED);
}

static int module_is_library(const char *name)
{
    ULONG flags = 0;

    return module_cmd(NCMD_MODULE_QUERY, name, NULL, &flags, NULL) == 0 &&
           (flags & NCMMF_LIBRARY);
}

static LONG module_users(const char *name)
{
    LONG users = -1;

    return module_cmd(NCMD_MODULE_QUERY, name, NULL, NULL, &users) == 0 ? users : -1;
}

/* Whether the stack makes a socket of the family NOW (st_have_family()
 * asks once and remembers, which is of no use here). */
static int family_works(int af)
{
    int s = socket(af, SOCK_DGRAM, 0);

    if (s < 0)
        return 0;
    CloseSocket(s);
    return 1;
}

/* A datagram from one loopback socket of the family to another arrives. */
static int udp_works(int af)
{
    struct sockaddr_storage ss;
    unsigned short port;
    socklen_t len;
    char c = 'x', d = 0;
    int r = -1, a, b;

    a = st_bound(af, SOCK_DGRAM, &port);
    b = socket(af, SOCK_DGRAM, 0);
    if (a >= 0 && b >= 0) {
        len = st_loopback(af, &ss, port);
        if (sendto(b, &c, 1, 0, (struct sockaddr *)&ss, len) == 1 &&
                sock_wait(a, 0, 3) > 0 && recv(a, &d, 1, 0) == 1 && d == c)
            r = 0;
    }
    if (a >= 0) CloseSocket(a);
    if (b >= 0) CloseSocket(b);
    return r;
}

/*
 * A TCP connection over the family's loopback address is made and carries
 * data: the address is there and answers. Both ends are reset when done,
 * so that no socket lingers in TIME_WAIT and keeps the family in use.
 */
static int tcp_works(int af)
{
    struct linger lg = { 1, 0 };
    int c = -1, s = -1, r;

    if (st_tcp_pair(af, &c, &s) != 0)
        return -1;
    r = st_oneway(c, s, 2000);
    setsockopt(c, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    setsockopt(s, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    CloseSocket(s);
    CloseSocket(c);
    Delay(10);                  /* the reset has passed: both sockets are gone */
    return r;
}

/*
 * A family is running again, whatever state a test left it in: started if
 * its module is loaded, loaded if its library went. 0 when it runs.
 */
static LONG family_restore(const struct family *f)
{
    LONG r;

    if (module_started(f->f_module))
        return 0;
    r = module_cmd(NCMD_MODULE_START, f->f_module, NULL, NULL, NULL);
    if (r == ENOENT)
        r = module_cmd(NCMD_MODULE_LOAD, f->f_library, NULL, NULL, NULL);
    stopped_something = 1;
    return module_started(f->f_module) ? 0 : (r ? r : -1);
}

static int init_suite(void) { return 0; }

/* Whatever the tests stopped runs again when the suite is left. */
static int clean_suite(void)
{
    (void)family_restore(&fam4);
    (void)family_restore(&fam6);
    return 0;
}

/* The transports are not stopped while a family runs: each one that runs is a user. */
static void testMODSTOP_TRANSPORT_REFUSED(void)
{
    ULONG flags = 0;
    LONG users = -1;

    CU_ASSERT_EQUAL_FATAL(module_cmd(NCMD_MODULE_QUERY, "transport", NULL, &flags, &users), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(users >= 2);                  /* both families */
    CU_ASSERT_FALSE(flags & NCMMF_PERMANENT);   /* it can stop - when nobody needs it */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "transport", NULL, &flags, &users), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(users >= 2);                  /* the refusal says who is in the way */
    CU_ASSERT_EQUAL(tcp_works(AF_INET), 0);
    CU_ASSERT_EQUAL(tcp_works(AF_INET6), 0);
}

/*
 * A family with a socket open is not stopped, and the answer says how
 * many users are in the way. With the socket closed it stops: the stack
 * makes no socket of it, the module says it is not started, and TCP and
 * UDP of the other family work as before. The other family is now the
 * last one, and is not stopped. Started again, the family's sockets work
 * and a TCP connection over its loopback address is answered.
 */
static void stop_start(const struct family *f, const struct family *o)
{
    ULONG flags = 0;
    LONG users = -1;
    int s, e;

    CU_ASSERT_FATAL(module_started(f->f_module));
    CU_ASSERT_FATAL(module_started(o->f_module));
    CU_ASSERT_EQUAL(module_users(f->f_module), 0);

    /* in use: refused */
    s = socket(f->f_af, SOCK_STREAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, f->f_module, NULL, &flags, &users), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(users, 1);
    CU_ASSERT(family_works(f->f_af));
    CloseSocket(s);
    CU_ASSERT_EQUAL(module_users(f->f_module), 0);

    /* not in use: stopped */
    flags = ~(ULONG)0;
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, f->f_module, NULL, &flags, &users), 0);
    stopped_something = 1;
    CU_ASSERT_FALSE(flags & NCMMF_STARTED);
    CU_ASSERT_FALSE(module_started(f->f_module));
    CU_ASSERT_EQUAL(module_users(f->f_module), 0);
    s = socket(f->f_af, SOCK_STREAM, 0);
    e = s < 0 ? Errno() : 0;
    CU_ASSERT(s < 0);
    CU_ASSERT(e == EAFNOSUPPORT || e == EPROTONOSUPPORT || e == EPFNOSUPPORT);
    if (s >= 0)
        CloseSocket(s);
    /* stopping what is stopped is no error */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, f->f_module, NULL, NULL, NULL), 0);

    /* the other family is untouched ... */
    CU_ASSERT(module_started(o->f_module));
    CU_ASSERT_EQUAL(tcp_works(o->f_af), 0);
    CU_ASSERT_EQUAL(udp_works(o->f_af), 0);
    /* ... and, being the last one, is not stopped - with or without users */
    flags = 0;
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, o->f_module, NULL, &flags, NULL), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(tcp_works(o->f_af), 0);
    /* the transports are still held, by the one family that runs */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "transport", NULL, &flags, &users), EBUSY);
    CU_ASSERT(users >= 1);

    /* started again */
    flags = 0;
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, f->f_module, NULL, &flags, &users), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(users, 0);
    CU_ASSERT(family_works(f->f_af));
    CU_ASSERT_EQUAL(tcp_works(f->f_af), 0);         /* its loopback address answers */
    CU_ASSERT_EQUAL(udp_works(f->f_af), 0);
    CU_ASSERT_EQUAL(tcp_works(o->f_af), 0);
    CU_ASSERT_EQUAL(module_users(f->f_module), 0);  /* nothing of the test's is left in it */
}

/*
 * The same with the library file: in use it is not unloaded; not in use it
 * is, and is then gone from exec's list of libraries and unknown to the
 * stack; loaded again from its file it runs as before.
 */
static void unload_load(const struct family *f, const struct family *o)
{
    char name[NCM_MODULE_NAMELEN] = "";
    ULONG flags = 0;
    LONG users = -1;
    int s;

    CU_ASSERT_FATAL(module_started(f->f_module));
    CU_ASSERT_FATAL(module_started(o->f_module));
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), 1);        /* held open by the stack alone */

    s = socket(f->f_af, SOCK_DGRAM, 0);
    CU_ASSERT_FATAL(s >= 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, f->f_module, NULL, &flags, &users), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(users, 1);
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), 1);
    CloseSocket(s);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, f->f_module, NULL, NULL, NULL), 0);
    stopped_something = 1;
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), -1);       /* gone from memory */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, f->f_module, NULL, NULL, NULL), ENOENT);
    CU_ASSERT_FALSE(family_works(f->f_af));
    CU_ASSERT_EQUAL(tcp_works(o->f_af), 0);
    CU_ASSERT_EQUAL(udp_works(o->f_af), 0);
    /* what is not there cannot be started by name; it is loaded */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, f->f_module, NULL, NULL, NULL), ENOENT);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, f->f_library, name, &flags, &users), 0);
    CU_ASSERT_STRING_EQUAL(name, f->f_module);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), 1);
    CU_ASSERT(family_works(f->f_af));
    CU_ASSERT_EQUAL(tcp_works(f->f_af), 0);
    CU_ASSERT_EQUAL(udp_works(f->f_af), 0);
    CU_ASSERT_EQUAL(tcp_works(o->f_af), 0);

    /* stopped and left loaded, it is unloaded all the same, and comes back */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, f->f_module, NULL, &flags, NULL), 0);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), 1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, f->f_module, NULL, NULL, NULL), 0);
    CU_ASSERT_EQUAL(lib_opencount(f->f_library), -1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, f->f_library, NULL, NULL, NULL), 0);
    CU_ASSERT_EQUAL(tcp_works(f->f_af), 0);
}

/*
 * Stopped and started CYCLES times - and, where the family is a library
 * file, unloaded and loaded as often: it works after every round, and free
 * memory ends where it began. (Two rounds first, for what the stack and
 * the system allocate once and keep: pool and mbuf memory.)
 */
static void cycles(const struct family *f, int unload)
{
    ULONG before = 0, after;
    LONG drift;
    int i, works = 0, gone = 0;

    CU_ASSERT_FATAL(module_started(f->f_module));
    for (i = -2; i < CYCLES; i++) {
        if (i == 0)
            before = AvailMem(MEMF_ANY);
        if (unload) {
            if (module_cmd(NCMD_MODULE_UNLOAD, f->f_module, NULL, NULL, NULL) != 0)
                break;
            stopped_something = 1;
            if (i >= 0 && lib_opencount(f->f_library) == -1)
                gone++;
            if (module_cmd(NCMD_MODULE_LOAD, f->f_library, NULL, NULL, NULL) != 0)
                break;
        } else {
            if (module_cmd(NCMD_MODULE_STOP, f->f_module, NULL, NULL, NULL) != 0)
                break;
            stopped_something = 1;
            if (i >= 0 && !family_works(f->f_af))
                gone++;
            if (module_cmd(NCMD_MODULE_START, f->f_module, NULL, NULL, NULL) != 0)
                break;
        }
        /* every other round is stopped again at once, in the middle of
         * what a start sets going (duplicate address detection ...) */
        if (i >= 0 && tcp_works(f->f_af) == 0)
            works++;
        if (i & 1)
            Delay(10);
    }
    after = AvailMem(MEMF_ANY);
    drift = i > 0 ? (LONG)before - (LONG)after : 0;
    printf("\n    %s: free memory after %d %s cycles: %ld bytes less (limit %d)\n",
           f->f_module, i, unload ? "unload/load" : "stop/start", (long)drift, DRIFT_MAX);

    CU_ASSERT_EQUAL(i, CYCLES);
    CU_ASSERT_EQUAL(works, CYCLES);
    CU_ASSERT_EQUAL(gone, CYCLES);
    CU_ASSERT(drift < DRIFT_MAX);
    CU_ASSERT(drift > -DRIFT_MAX);
    CU_ASSERT(module_started(f->f_module));
    CU_ASSERT_EQUAL(module_users(f->f_module), 0);
}

static void testMODSTOP_INET6_STOP_START(void)  { stop_start(&fam6, &fam4); }
static void testMODSTOP_INET6_UNLOAD_LOAD(void) { unload_load(&fam6, &fam4); }
static void testMODSTOP_INET6_STOP_CYCLES(void) { cycles(&fam6, 0); }
static void testMODSTOP_INET6_LOAD_CYCLES(void) { cycles(&fam6, 1); }
static void testMODSTOP_INET_STOP_START(void)   { stop_start(&fam4, &fam6); }
static void testMODSTOP_INET_UNLOAD_LOAD(void)  { unload_load(&fam4, &fam6); }
static void testMODSTOP_INET_STOP_CYCLES(void)  { cycles(&fam4, 0); }
static void testMODSTOP_INET_LOAD_CYCLES(void)  { cycles(&fam4, 1); }

/*
 * The tests of one family, added if the family can be stopped at all here:
 * nobody else may have a socket of it, and no other module may hold it.
 * (Nothing is skipped from inside a test: what cannot run is not added,
 * and a SKIP line says so.) 0 only if CUnit fails.
 */
static int add_family(CU_pSuite p, const struct family *f,
                      CU_TestFunc stopstart, CU_TestFunc unloadload,
                      CU_TestFunc stopcycles, CU_TestFunc loadcycles,
                      const char *n1, const char *n2, const char *n3, const char *n4)
{
    ULONG flags = 0;
    LONG users = -1;

    if (module_cmd(NCMD_MODULE_QUERY, f->f_module, NULL, &flags, &users) != 0)
        return 1;
    if (users != 0) {
        printf("SKIP %s: %s has %ld user(s) in the running stack (sockets of other "
               "programs, or a module that needs it)\n", n1, f->f_module, (long)users);
        printf("SKIP %s: %s has %ld user(s) in the running stack\n", n2, f->f_module, (long)users);
        printf("SKIP %s: %s has %ld user(s) in the running stack\n", n3, f->f_module, (long)users);
        printf("SKIP %s: %s has %ld user(s) in the running stack\n", n4, f->f_module, (long)users);
        return 1;
    }
    if (flags & NCMMF_PERMANENT) {
        printf("SKIP %s: the running stack cannot stop %s\n", n1, f->f_module);
        printf("SKIP %s: the running stack cannot stop %s\n", n2, f->f_module);
        printf("SKIP %s: the running stack cannot stop %s\n", n3, f->f_module);
        printf("SKIP %s: the running stack cannot stop %s\n", n4, f->f_module);
        return 1;
    }
    if (CU_add_test(p, n1, stopstart) == NULL || CU_add_test(p, n3, stopcycles) == NULL)
        return 0;
    if (!(flags & NCMMF_LIBRARY)) {
        printf("SKIP %s: %s is part of the stack program, not a library file\n", n2, f->f_module);
        printf("SKIP %s: %s is part of the stack program, not a library file\n", n4, f->f_module);
        return 1;
    }
    return CU_add_test(p, n2, unloadload) != NULL && CU_add_test(p, n4, loadcycles) != NULL;
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int rc;

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    /* with one family, that one is the last and stays: nothing to test */
    if (st_skip_program(PROGRAM, ST_BOTH)) {
        CloseLibrary(SocketBase);
        return 0;
    }
    if (module_cmd(NCMD_MODULE_QUERY, "transport", NULL, NULL, NULL) != 0 ||
            !module_started(fam4.f_module) || !module_started(fam6.f_module)) {
        printf("SKIP %s: the stack has no control port, or does not know its modules\n", PROGRAM);
        CloseLibrary(SocketBase);
        return 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MODSTOP_Suite", init_suite, clean_suite);
    if (p == NULL ||
        CU_add_test(p, "the transports are not stopped while a family runs", testMODSTOP_TRANSPORT_REFUSED) == NULL ||
        !add_family(p, &fam6, testMODSTOP_INET6_STOP_START, testMODSTOP_INET6_UNLOAD_LOAD,
                    testMODSTOP_INET6_STOP_CYCLES, testMODSTOP_INET6_LOAD_CYCLES,
                    "inet6 stops when it has no socket, and starts again",
                    "inet6's library is unloaded and loaded again",
                    "ten stop/start cycles of inet6 leave memory as it was",
                    "ten unload/load cycles of inet6 leave memory as it was") ||
        !add_family(p, &fam4, testMODSTOP_INET_STOP_START, testMODSTOP_INET_UNLOAD_LOAD,
                    testMODSTOP_INET_STOP_CYCLES, testMODSTOP_INET_LOAD_CYCLES,
                    "inet stops when it has no socket, and starts again",
                    "inet's library is unloaded and loaded again",
                    "ten stop/start cycles of inet leave memory as it was",
                    "ten unload/load cycles of inet leave memory as it was"))
    {
        CU_cleanup_registry();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    /*
     * As every program of the suite: once for the console, once for the
     * XML report. The suite's clean-up starts again whatever a test left
     * stopped, also after an assertion that ended a test early.
     */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ModStop");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    rc = CU_get_error();

    /*
     * The stack as it was found: both families run (the clean-up saw to
     * it; once more, for a suite that never ran), and what the
     * configuration gives them - a family started while the stack runs
     * has its loopback address and what it makes by itself, the
     * interfaces' configured addresses and routes come with a reload.
     * Then a moment for duplicate address detection, so that the program
     * that runs next finds usable addresses.
     */
    if (family_restore(&fam4) != 0 || family_restore(&fam6) != 0) {
        printf("%s: a family could NOT be started again - the stack is not as it was\n", PROGRAM);
        rc = 20;
    }
    if (stopped_something) {
        LONG r = net_reload();

        printf("%s: configuration read again (result %ld)\n", PROGRAM, (long)r);
        Delay(4 * 50);
    }
    CloseLibrary(SocketBase);
    return rc;
}
