/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the modules the stack loads on first use
    (bsdsocket/net/netdemand.h in the stack): a call that needs a module
    that is not running has it started - its library loaded - by the
    stack's own task and goes on, once; a module that cannot be had makes
    the call fail, and is not looked for again until the configuration is
    read again or the module is loaded by name; a module is not unloaded
    while a task is inside it; loading and unloading leave memory as it
    was; and the filter requests, which start the packet services this way,
    behave as they did.

    The module is demo.atcp (modules/demo), which serves two
    ioctl numbers of its own and is started by the stack when one of them
    is asked for. Where it is not installed, or the stack cannot load on
    demand, those tests say SKIP; the filter test runs on any stack.

    Some tests need a second task: this program, started again with an
    argument (--calls, --wait), which reports through a message port.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <proto/miami.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <dos/var.h>
#include <sys/sysctl.h>
#include <net/ipfilter.h>
#include <bsdsocket/netcontrol.h>

#include <stdlib.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define PROGRAM         "cunit-net-demand"
#define DEMO_LIBRARY    "demo.atcp"
#define DEMO_MODULE     "demo"
#define DEMO_SWITCH     "AROSTCP/Without_demo"

/* The demo's ioctl numbers and variables (modules/demo/demo.c). */
struct demo_wait {
    struct Task    *dw_task;
    ULONG           dw_inside;
    ULONG           dw_wait;
};
#define DEMOIOC_CALLS   _IOR('z', 208, ULONG)
#define DEMOIOC_WAIT    _IOW('z', 209, struct demo_wait)

#define DEMO_MIB_FAMILY 253
#define DEMO_MIB_PROTO  253
#define DEMO_MIB_STATS  2

struct demo_stats {
    unsigned int    ds_magic;
    unsigned int    ds_starts;
    unsigned int    ds_stops;
    unsigned int    ds_mbufs;
    unsigned int    ds_interfaces;
    unsigned int    ds_inet;
};

#define CYCLES          20
#define DRIFT_MAX       4096

/* how the second task reports */
#define HELPER_PORT     "cunit-net-demand.helper"
#define HELPER_WAIT     10      /* s to wait for a helper's message */

struct hmsg {
    struct Message  hm_Msg;
    struct Task    *hm_Task;    /* the helper */
    LONG            hm_Phase;   /* 0 ready, 1 done */
    LONG            hm_Result;  /* of its IoctlSocket() */
    LONG            hm_Errno;
    ULONG           hm_Value;   /* DEMOIOC_CALLS: the count it got */
    struct Task    *hm_Parent;  /* the answer to "ready": who to tell when it is inside */
    ULONG           hm_Inside;
};

static char progpath[256];
static struct MsgPort *hport = NULL;        /* the helpers report here (HELPER_PORT) */
static struct Task *waiting_helper = NULL;  /* inside the module until signalled */

/* ---- the control port ------------------------------------------------- */

/* One module command (or RELOAD with name NULL); the stack's answer, -1 without a control port. */
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

static LONG unload_demo(void)
{
    return module_cmd(NCMD_MODULE_UNLOAD, DEMO_MODULE, NULL);
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

static int demo_stats(struct demo_stats *ds)
{
    LONG name[4] = { CTL_NET, DEMO_MIB_FAMILY, DEMO_MIB_PROTO, DEMO_MIB_STATS };
    LONG len = sizeof(*ds);

    memset(ds, 0, sizeof(*ds));
    return MiamiSysCtl(name, 4, ds, &len, NULL, 0, 0);
}

/* The demo's request on a socket of its own: what IoctlSocket() answered; errno in *err. */
static LONG demo_call(ULONG *value, LONG *err)
{
    int s = socket(st_any_family(), SOCK_DGRAM, 0);
    ULONG v = 0;
    LONG r;

    *err = 0;
    if (s < 0) {
        *err = Errno();
        return -2;
    }
    r = IoctlSocket(s, DEMOIOC_CALLS, (char *)&v);
    if (r < 0)
        *err = Errno();
    CloseSocket(s);
    if (value)
        *value = v;
    return r;
}

/* The demo is not loaded, the test switch is not set, and nothing is remembered against it. */
static void demo_reset(void)
{
    DeleteVar(DEMO_SWITCH, GVF_GLOBAL_ONLY);
    if (lib_opencount(DEMO_LIBRARY) < 0 &&
            module_cmd(NCMD_MODULE_LOAD, DEMO_LIBRARY, NULL) != 0)
        return;
    /* a load by name forgets that it could not be had; then out again */
    unload_demo();
}

/* ---- the second task -------------------------------------------------- */

static BOOL to_parent(struct hmsg *m)
{
    struct MsgPort *port;

    Forbid();
    if ((port = FindPort(HELPER_PORT)) != NULL)
        PutMsg(port, &m->hm_Msg);
    Permit();
    if (port == NULL)
        return FALSE;
    WaitPort(m->hm_Msg.mn_ReplyPort);
    GetMsg(m->hm_Msg.mn_ReplyPort);
    return TRUE;
}

/* --calls: one DEMOIOC_CALLS when told to go. --wait: DEMOIOC_WAIT, inside the module until CTRL-E. */
static int helper(int wait)
{
    struct MsgPort *reply;
    struct hmsg m;
    struct demo_wait dw;
    ULONG v = 0;
    int s = -1;

    if ((reply = CreateMsgPort()) == NULL)
        return 20;
    if ((SocketBase = OpenLibrary("bsdsocket.library", 4)) != NULL)
        s = socket(st_any_family(), SOCK_DGRAM, 0);
    memset(&m, 0, sizeof(m));
    m.hm_Msg.mn_ReplyPort = reply;
    m.hm_Msg.mn_Length = sizeof(m);
    m.hm_Task = FindTask(NULL);
    m.hm_Result = -2;
    if (s >= 0 && to_parent(&m)) {
        if (wait) {
            dw.dw_task = m.hm_Parent;
            dw.dw_inside = m.hm_Inside;
            dw.dw_wait = SIGBREAKF_CTRL_E;
            m.hm_Result = IoctlSocket(s, DEMOIOC_WAIT, (char *)&dw);
        } else
            m.hm_Result = IoctlSocket(s, DEMOIOC_CALLS, (char *)&v);
        m.hm_Errno = m.hm_Result < 0 ? Errno() : 0;
        m.hm_Value = v;
        m.hm_Phase = 1;
        to_parent(&m);
    }
    if (s >= 0)
        CloseSocket(s);
    if (SocketBase)
        CloseLibrary(SocketBase);
    DeleteMsgPort(reply);
    return 0;
}

static int spawn(const char *args)
{
    char cmd[400];
    BPTR in = Open("NIL:", MODE_OLDFILE), out = Open("NIL:", MODE_NEWFILE);

    snprintf(cmd, sizeof(cmd), "\"%s\" %s", progpath, args);
    if (SystemTags(cmd, SYS_Input, in, SYS_Output, out, SYS_Asynch, TRUE, TAG_DONE) != 0) {
        Close(in);
        Close(out);
        return -1;
    }
    return 0;
}

static struct MsgPort *helper_port(void)
{
    struct MsgPort *port = CreateMsgPort();

    if (port != NULL) {
        port->mp_Node.ln_Name = HELPER_PORT;
        port->mp_Node.ln_Pri = 0;
        AddPort(port);
    }
    return port;
}

static void helper_port_end(struct MsgPort *port)
{
    struct Message *m;

    if (port == NULL)
        return;
    RemPort(port);
    while ((m = GetMsg(port)) != NULL)
        ReplyMsg(m);
    DeleteMsgPort(port);
}

/* A helper's next message, or NULL after HELPER_WAIT seconds. */
static struct hmsg *helper_msg(struct MsgPort *port)
{
    struct hmsg *m;
    int i;

    for (i = 0; i < HELPER_WAIT * 10; i++) {
        if ((m = (struct hmsg *)GetMsg(port)) != NULL)
            return m;
        Delay(5);
    }
    return NULL;
}

/* ---- the suite -------------------------------------------------------- */

static int init_suite(void) { return MiamiBase == NULL ? -1 : 0; }

static int clean_suite(void)
{
    if (waiting_helper != NULL) {
        Signal(waiting_helper, SIGBREAKF_CTRL_E);
        waiting_helper = NULL;
        Delay(25);
    }
    demo_reset();
    return 0;
}

/* The first call loads the library and starts the module; the next ones find it running. */
static void testDEMAND_FIRST_CALL_LOADS(void)
{
    struct demo_stats ds;
    ULONG flags = 0, v = 0;
    LONG err;

    demo_reset();
    CU_ASSERT_EQUAL_FATAL(lib_opencount(DEMO_LIBRARY), -1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, NULL), ENOENT);

    CU_ASSERT_EQUAL(demo_call(&v, &err), 0);
    CU_ASSERT_EQUAL(v, 1);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, &flags), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);

    /* not loaded again, not started again */
    CU_ASSERT_EQUAL(demo_call(&v, &err), 0);
    CU_ASSERT_EQUAL(v, 2);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_starts, 1);

    /* unloaded, the next call loads it again, with its data new */
    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
    CU_ASSERT_EQUAL(demo_call(&v, &err), 0);
    CU_ASSERT_EQUAL(v, 1);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(unload_demo(), 0);
}

/*
 * Kept out by its test switch, the module cannot be had: the call fails
 * with ENOENT. It is not looked for again when the switch is gone - until
 * it is loaded by name, or the configuration is read again.
 */
static void testDEMAND_NOT_INSTALLED(void)
{
    ULONG v = 0;
    LONG err = 0, r;

    demo_reset();
    CU_ASSERT_FATAL(SetVar(DEMO_SWITCH, "1", 1, GVF_GLOBAL_ONLY) != 0);

    CU_ASSERT_EQUAL(demo_call(&v, &err), -1);
    CU_ASSERT_EQUAL(err, ENOENT);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_DEMAND, DEMO_MODULE, NULL), ENOENT);

    /* the switch is gone, but the stack does not look again */
    DeleteVar(DEMO_SWITCH, GVF_GLOBAL_ONLY);
    CU_ASSERT_EQUAL(demo_call(&v, &err), -1);
    CU_ASSERT_EQUAL(err, ENOENT);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);

    /* a load by name: it is to be had again */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, DEMO_LIBRARY, NULL), 0);
    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(demo_call(&v, &err), 0);
    CU_ASSERT_EQUAL(unload_demo(), 0);

    /* the same once more, and this time the configuration is read again */
    CU_ASSERT_FATAL(SetVar(DEMO_SWITCH, "1", 1, GVF_GLOBAL_ONLY) != 0);
    CU_ASSERT_EQUAL(demo_call(&v, &err), -1);
    CU_ASSERT_EQUAL(err, ENOENT);
    DeleteVar(DEMO_SWITCH, GVF_GLOBAL_ONLY);
    CU_ASSERT_EQUAL(demo_call(&v, &err), -1);
    r = module_cmd(NCMD_RELOAD, NULL, NULL);
    printf("\n    configuration read again (result %ld)", (long)r);
    Delay(4 * 50);              /* duplicate address detection, for the programs after this one */
    CU_ASSERT_EQUAL(demo_call(&v, &err), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(unload_demo(), 0);
}

/* Two tasks need the module at the same time: it is loaded and started once, and both go on. */
static void testDEMAND_TWO_TASKS_LOAD_ONCE(void)
{
    struct MsgPort *port = hport;
    struct hmsg *ready[2] = { NULL, NULL }, *done[2] = { NULL, NULL };
    struct demo_stats ds;
    int i;

    demo_reset();
    for (i = 0; i < 2; i++)
        CU_ASSERT(spawn("--calls") == 0);
    for (i = 0; i < 2; i++)
        ready[i] = helper_msg(port);
    CU_ASSERT(ready[0] != NULL && ready[1] != NULL);
    /* both at once */
    for (i = 0; i < 2; i++)
        if (ready[i] != NULL)
            ReplyMsg(&ready[i]->hm_Msg);
    for (i = 0; i < 2; i++)
        if (ready[i] != NULL)
            done[i] = helper_msg(port);
    for (i = 0; i < 2; i++) {
        if (done[i] == NULL)
            continue;
        CU_ASSERT_EQUAL(done[i]->hm_Result, 0);
        CU_ASSERT(done[i]->hm_Value == 1 || done[i]->hm_Value == 2);
    }
    CU_ASSERT(done[0] != NULL && done[1] != NULL);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_starts, 1);
    for (i = 0; i < 2; i++)
        if (done[i] != NULL)
            ReplyMsg(&done[i]->hm_Msg);
    Delay(25);                  /* the helpers close their bases and leave */
    CU_ASSERT_EQUAL(unload_demo(), 0);
}

/* While a task is inside the module, it is neither unloaded nor stopped; once it is out, it is. */
static void testDEMAND_BUSY_REFUSED(void)
{
    struct MsgPort *port = hport;
    struct hmsg *m, *done = NULL;
    ULONG flags = 0, v;
    LONG err, sig;
    int i, inside = 0;

    demo_reset();
    CU_ASSERT_EQUAL_FATAL(demo_call(&v, &err), 0);      /* loaded */
    CU_ASSERT_FATAL((sig = AllocSignal(-1)) != -1);
    SetSignal(0, 1UL << sig);
    if (spawn("--wait") == 0 && (m = helper_msg(port)) != NULL) {
        m->hm_Parent = FindTask(NULL);
        m->hm_Inside = 1UL << sig;
        waiting_helper = m->hm_Task;
        ReplyMsg(&m->hm_Msg);
        for (i = 0; i < HELPER_WAIT * 10 && !inside; i++) {
            if (SetSignal(0, 1UL << sig) & (1UL << sig))
                inside = 1;
            else
                Delay(5);
        }
    }
    CU_ASSERT_FATAL(inside);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, DEMO_MODULE, &flags), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, DEMO_MODULE, &flags), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);

    /* let it out */
    Signal(waiting_helper, SIGBREAKF_CTRL_E);
    waiting_helper = NULL;
    done = helper_msg(port);
    CU_ASSERT(done != NULL);
    if (done != NULL) {
        CU_ASSERT_EQUAL(done->hm_Result, 0);
        ReplyMsg(&done->hm_Msg);
    }
    Delay(25);
    FreeSignal(sig);

    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
}

/* Loaded on demand and unloaded twenty times: free memory ends where it began. */
static void testDEMAND_CYCLES_NO_DRIFT(void)
{
    ULONG before, after, v;
    LONG drift, err;
    int i;

    demo_reset();
    for (i = 0; i < 2; i++) {
        CU_ASSERT_EQUAL_FATAL(demo_call(&v, &err), 0);
        CU_ASSERT_EQUAL_FATAL(unload_demo(), 0);
    }
    before = AvailMem(MEMF_ANY);
    for (i = 0; i < CYCLES; i++) {
        if (demo_call(&v, &err) != 0 || v != 1)
            break;
        if (unload_demo() != 0)
            break;
    }
    after = AvailMem(MEMF_ANY);
    drift = (LONG)before - (LONG)after;
    printf("\n    free memory after %d load-on-demand/unload cycles: %ld bytes less (limit %d)",
           i, (long)drift, DRIFT_MAX);
    CU_ASSERT_EQUAL(i, CYCLES);
    CU_ASSERT(drift < DRIFT_MAX);
    CU_ASSERT(drift > -DRIFT_MAX);
}

/*
 * The filter requests start the packet services: on any stack (they need
 * no family) the first request starts the module and is answered; where
 * the module cannot be had it fails with ENOENT.
 */
static void testDEMAND_FILTER_AS_BEFORE(void)
{
    struct ipf_state st;
    ULONG flags = 0;
    int s = socket(st_any_family(), SOCK_DGRAM, 0);
    LONG r, err;

    CU_ASSERT_FATAL(s >= 0);
    if (module_cmd(NCMD_MODULE_QUERY, "packet", &flags) == 0 && (flags & NCMMF_STARTED)) {
        memset(&st, 0, sizeof(st));
        if (IoctlSocket(s, SIOCGETFS, (char *)&st) == 0 && (st.enabled || st.rule_count != 0)) {
            /* somebody's firewall: not to be stopped by a test */
            printf("\n    the filter is in use here: only asked, not stopped");
            CloseSocket(s);
            return;
        }
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "packet", NULL), 0);
    }

    memset(&st, 0, sizeof(st));
    r = IoctlSocket(s, SIOCGETFS, (char *)&st);
    err = r < 0 ? Errno() : 0;
    flags = 0;
    module_cmd(NCMD_MODULE_QUERY, "packet", &flags);
    if (r == 0) {
        CU_ASSERT(flags & NCMMF_STARTED);
        CU_ASSERT_EQUAL(st.enabled, 0);
        CU_ASSERT_EQUAL(st.rule_count, 0);
        /* stopped, the next request starts it again */
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "packet", NULL), 0);
        CU_ASSERT_EQUAL(IoctlSocket(s, SIOCGETFS, (char *)&st), 0);
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, "packet", &flags), 0);
        CU_ASSERT(flags & NCMMF_STARTED);
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, "packet", NULL), 0);
    } else {
        CU_ASSERT_EQUAL(err, ENOENT);       /* not installed, or ENV:AROSTCP/Without_packet */
        CU_ASSERT_FALSE(flags & NCMMF_STARTED);
    }
    CloseSocket(s);
}

/* ---- main ------------------------------------------------------------- */

/* 0: the demo can be loaded on demand here; else why not, said in a SKIP line. */
static int demo_usable(void)
{
    ULONG flags = 0;
    LONG r;

    DeleteVar(DEMO_SWITCH, GVF_GLOBAL_ONLY);
    if (lib_opencount(DEMO_LIBRARY) >= 0)
        unload_demo();
    if ((r = module_cmd(NCMD_MODULE_LOAD, DEMO_LIBRARY, NULL)) != 0) {
        if (r == -1)
            printf("SKIP %s (demo): the stack has no control port\n", PROGRAM);
        else
            printf("SKIP %s (demo): %s is not installed (error %ld)\n", PROGRAM, DEMO_LIBRARY, (long)r);
        return -1;
    }
    unload_demo();
    /* a stack that does not know the command answers 0 and does nothing */
    if (module_cmd(NCMD_MODULE_DEMAND, DEMO_MODULE, NULL) != 0 ||
            module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, &flags) != 0 || !(flags & NCMMF_STARTED)) {
        printf("SKIP %s (demo): the running stack does not load modules on demand\n", PROGRAM);
        unload_demo();
        return -1;
    }
    unload_demo();
    return 0;
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    int demo;

    if (argc > 1 && strcmp(argv[1], "--calls") == 0)
        return helper(0);
    if (argc > 1 && strcmp(argv[1], "--wait") == 0)
        return helper(1);

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    MiamiBase = OpenLibrary("miami.library", 0);
    if (SocketBase == NULL || MiamiBase == NULL) {
        printf("no bsdsocket.library or no miami.library\n");
        if (MiamiBase)
            CloseLibrary(MiamiBase);
        if (SocketBase)
            CloseLibrary(SocketBase);
        return 20;
    }
    if (!NameFromLock(GetProgramDir(), progpath, sizeof(progpath)) ||
            !AddPart(progpath, FilePart((STRPTR)argv[0]), sizeof(progpath)) ||
            (hport = helper_port()) == NULL)
        progpath[0] = '\0';
    demo = demo_usable() == 0;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        helper_port_end(hport);
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("DEMAND_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "the filter requests start the packet services", testDEMAND_FILTER_AS_BEFORE, ST_ANY) ||
        (demo && (
        !st_add_test(p, "the first call loads the module, the next ones do not", testDEMAND_FIRST_CALL_LOADS, ST_NONE) ||
        !st_add_test(p, "a module that cannot be had is not looked for again until told", testDEMAND_NOT_INSTALLED, ST_NONE) ||
        (progpath[0] != '\0' &&
         !st_add_test(p, "two tasks that need it at once load it once", testDEMAND_TWO_TASKS_LOAD_ONCE, ST_NONE)) ||
        (progpath[0] != '\0' &&
         !st_add_test(p, "it is not unloaded while a task is inside it", testDEMAND_BUSY_REFUSED, ST_NONE)) ||
        !st_add_test(p, "twenty load-on-demand/unload cycles leave memory as it was", testDEMAND_CYCLES_NO_DRIFT, ST_NONE))))
    {
        CU_cleanup_registry();
        helper_port_end(hport);
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    if (demo && progpath[0] == '\0') {
        printf("SKIP two tasks that need it at once load it once: cannot find this program\n");
        printf("SKIP it is not unloaded while a task is inside it: cannot find this program\n");
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Demand");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    helper_port_end(hport);
    CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
