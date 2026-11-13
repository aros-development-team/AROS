/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for a module of the stack whose code is in a library file
    of its own, as seen through the stack's control port
    (bsdsocket/netcontrol.h): the library is loaded and its module started,
    the module is known, stopped and started like one built into the stack,
    it is not unloaded while it is in use, and once unloaded the library is
    really gone from memory - every time, without memory going astray.

    The module is demo.atcp (a working name), which carries no
    protocol: workbench/network/stacks/AROSTCP/modules/demo. Where that file
    is not installed, or the stack cannot load modules, the program says
    SKIP and leaves.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <proto/miami.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <sys/sysctl.h>
#include <bsdsocket/netcontrol.h>

struct Library *SocketBase = NULL;
struct Library *MiamiBase = NULL;

#define DEMO_LIBRARY    "demo.atcp"  /* the file, and the name exec lists it under */
#define DEMO_MODULE     "demo"                  /* the module it holds */
#define CORE_LIBRARY    "arostcp.library"       /* what the stack publishes for its modules */

/* The demo's variables under sysctl(): net.253.253.<id> (modules/demo/demo.c). */
#define DEMO_MIB_FAMILY 253
#define DEMO_MIB_PROTO  253
#define DEMO_MIB_HOLD   1
#define DEMO_MIB_STATS  2
#define DEMO_MAGIC      0x44454d4f

struct demo_stats {
    unsigned int    ds_magic;
    unsigned int    ds_starts;
    unsigned int    ds_stops;
    unsigned int    ds_mbufs;
    unsigned int    ds_interfaces;
    unsigned int    ds_inet;
};

/* load/unload cycles of the memory test, and how far free memory may be off after them */
#define CYCLES          20
#define DRIFT_MAX       4096

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
    if ((port = FindPort(AROSTCP_CTRLPORT_NAME)) != NULL)
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

static LONG load_demo(void)
{
    return module_cmd(NCMD_MODULE_LOAD, DEMO_LIBRARY, NULL, NULL, NULL);
}

static LONG unload_demo(void)
{
    return module_cmd(NCMD_MODULE_UNLOAD, DEMO_MODULE, NULL, NULL, NULL);
}

/*
 * Whether exec has a library of that name in memory, and how often it is
 * open: -1 when it is not there.
 */
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

/* One of the demo's variables; 0 or the error. */
static int demo_ctl(int id, void *val, LONG size, LONG *nval)
{
    LONG name[4] = { CTL_NET, DEMO_MIB_FAMILY, DEMO_MIB_PROTO, 0 };
    LONG len = size;

    name[3] = id;
    return MiamiSysCtl(name, 4, val, &len, nval, nval ? sizeof(*nval) : 0, 0);
}

static int demo_stats(struct demo_stats *ds)
{
    memset(ds, 0, sizeof(*ds));
    return demo_ctl(DEMO_MIB_STATS, ds, sizeof(*ds), NULL);
}

static int demo_hold(LONG on)
{
    LONG old = -1;

    return demo_ctl(DEMO_MIB_HOLD, &old, sizeof(old), &on);
}

static int init_suite(void)
{
    return MiamiBase == NULL ? -1 : 0;
}

/* Whatever a test left behind: let go of the module and take it out. */
static int clean_suite(void)
{
    if (lib_opencount(DEMO_LIBRARY) >= 0) {
        demo_hold(0);
        unload_demo();
    }
    return 0;
}

/*
 * Loaded: the library is in memory, held open by the stack alone; its
 * module runs, is not part of the stack program and has no users; it holds
 * the stack's services library open; what it registered when it started
 * can be read. Unloaded: all of that is gone again.
 */
static void testMODLIB_LOAD_UNLOAD(void)
{
    struct demo_stats ds;
    char name[NCM_MODULE_NAMELEN] = "";
    ULONG flags = 0;
    LONG users = -1, core = lib_opencount(CORE_LIBRARY);

    CU_ASSERT(core >= 0);                           /* the stack publishes its services */
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);

    CU_ASSERT_EQUAL_FATAL(module_cmd(NCMD_MODULE_LOAD, DEMO_LIBRARY, name, &flags, &users), 0);
    CU_ASSERT_STRING_EQUAL(name, DEMO_MODULE);      /* the answer names the module */
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_FALSE(flags & NCMMF_BUILTIN);
    CU_ASSERT_FALSE(flags & NCMMF_PERMANENT);
    CU_ASSERT_EQUAL(users, 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(lib_opencount(CORE_LIBRARY), core + 1);

    flags = 0;
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, NULL, &flags, &users), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(users, 0);

    /* its own data, and what it did with the core's services when it started */
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_magic, DEMO_MAGIC);
    CU_ASSERT_EQUAL(ds.ds_starts, 1);
    CU_ASSERT_EQUAL(ds.ds_stops, 0);
    CU_ASSERT_EQUAL(ds.ds_mbufs, 2);
    CU_ASSERT(ds.ds_interfaces >= 1);               /* the loopback at least */
    CU_ASSERT_EQUAL(ds.ds_inet, st_have_family(AF_INET) ? 1 : 0);

    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);       /* gone from memory */
    CU_ASSERT_EQUAL(lib_opencount(CORE_LIBRARY), core);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, NULL, NULL, NULL), ENOENT);
    CU_ASSERT_NOT_EQUAL(demo_stats(&ds), 0);                /* its registration went with it */
}

/* A second load of what is loaded is refused and changes nothing; so is a second unload. */
static void testMODLIB_LOAD_TWICE(void)
{
    struct demo_stats ds;
    ULONG flags = 0;

    CU_ASSERT_EQUAL_FATAL(load_demo(), 0);
    CU_ASSERT_EQUAL(load_demo(), EEXIST);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, NULL, &flags, NULL), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_starts, 1);               /* not started a second time */

    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
    CU_ASSERT_EQUAL(unload_demo(), ENOENT);
}

/*
 * Stopped, the module stays loaded and known and can be started again -
 * with the data it had, because the library never left memory. Unloaded
 * and loaded again, its data is as the file has it.
 */
static void testMODLIB_STOP_START(void)
{
    struct demo_stats ds;
    ULONG flags = ~(ULONG)0;

    CU_ASSERT_EQUAL_FATAL(load_demo(), 0);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, DEMO_MODULE, NULL, &flags, NULL), 0);
    CU_ASSERT_FALSE(flags & NCMMF_STARTED);
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);        /* still loaded */
    CU_ASSERT_NOT_EQUAL(demo_stats(&ds), 0);                /* but nothing registered */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, DEMO_MODULE, NULL, NULL, NULL), 0);   /* twice is no error */

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_START, DEMO_MODULE, NULL, &flags, NULL), 0);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_starts, 2);
    CU_ASSERT_EQUAL(ds.ds_stops, 1);

    /* a module that is loaded but stopped is unloaded all the same */
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, DEMO_MODULE, NULL, NULL, NULL), 0);
    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);

    CU_ASSERT_EQUAL_FATAL(load_demo(), 0);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);
    CU_ASSERT_EQUAL(ds.ds_starts, 1);               /* a new copy of its data */
    CU_ASSERT_EQUAL(ds.ds_stops, 0);
    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
}

/* In use, the module is neither stopped nor unloaded; let go of, it is. */
static void testMODLIB_IN_USE_REFUSED(void)
{
    struct demo_stats ds;
    ULONG flags = 0;

    CU_ASSERT_EQUAL_FATAL(load_demo(), 0);
    CU_ASSERT_EQUAL(demo_hold(1), 0);

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, DEMO_MODULE, NULL, &flags, NULL), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);                       /* the refusal says how it stands */
    CU_ASSERT(flags & NCMMF_LIBRARY);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), 1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_STOP, DEMO_MODULE, NULL, &flags, NULL), EBUSY);
    CU_ASSERT(flags & NCMMF_STARTED);
    CU_ASSERT_EQUAL(demo_stats(&ds), 0);                    /* and it works on */
    CU_ASSERT_EQUAL(ds.ds_stops, 0);

    CU_ASSERT_EQUAL(demo_hold(0), 0);
    CU_ASSERT_EQUAL(unload_demo(), 0);
    CU_ASSERT_EQUAL(lib_opencount(DEMO_LIBRARY), -1);
}

/* What cannot be loaded or unloaded is refused with the reason, and nothing stays behind. */
static void testMODLIB_REFUSALS(void)
{
    ULONG flags = 0;

    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, "nosuch.atcp", NULL, NULL, NULL), ENOENT);
    CU_ASSERT_EQUAL(lib_opencount("nosuch.atcp"), -1);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_LOAD, "", NULL, NULL, NULL), EINVAL);
    CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, "no-such-module", NULL, NULL, NULL), ENOENT);
    /* a module that is part of the stack program is not a library to unload:
     * "admin" is one in a build that keeps it in the program, a library
     * otherwise (and then not this test's business) */
    if(module_cmd(NCMD_MODULE_QUERY, "admin", NULL, &flags, NULL) == 0 &&
       (flags & NCMMF_STARTED) && !(flags & NCMMF_LIBRARY)) {
        CU_ASSERT_EQUAL(module_cmd(NCMD_MODULE_UNLOAD, "admin", NULL, &flags, NULL), EPERM);
    } else
        printf("SKIP a module of the stack program is not unloaded: no such module in this build\n");
}

/*
 * Loaded and unloaded twenty times: the library is gone from memory after
 * every round, its data is new in every round, and free memory ends where
 * it began. (Two rounds first, for what the system allocates once.)
 */
static void testMODLIB_CYCLES_NO_DRIFT(void)
{
    struct demo_stats ds;
    ULONG before, after;
    LONG drift, core = lib_opencount(CORE_LIBRARY);
    int i, gone = 0, fresh = 0;

    for (i = 0; i < 2; i++) {
        CU_ASSERT_EQUAL_FATAL(load_demo(), 0);
        CU_ASSERT_EQUAL_FATAL(unload_demo(), 0);
    }
    before = AvailMem(MEMF_ANY);
    for (i = 0; i < CYCLES; i++) {
        if (load_demo() != 0)
            break;
        if (demo_stats(&ds) == 0 && ds.ds_starts == 1 && ds.ds_magic == DEMO_MAGIC)
            fresh++;
        if (unload_demo() != 0)
            break;
        if (lib_opencount(DEMO_LIBRARY) == -1)
            gone++;
    }
    after = AvailMem(MEMF_ANY);
    drift = (LONG)before - (LONG)after;
    printf("free memory after %d load/unload cycles: %ld bytes less (limit %d)\n",
           i, (long)drift, DRIFT_MAX);

    CU_ASSERT_EQUAL(i, CYCLES);
    CU_ASSERT_EQUAL(gone, CYCLES);
    CU_ASSERT_EQUAL(fresh, CYCLES);
    CU_ASSERT(drift < DRIFT_MAX);
    CU_ASSERT(drift > -DRIFT_MAX);
    CU_ASSERT_EQUAL(lib_opencount(CORE_LIBRARY), core);
}

int main(int argc, char **argv)
{
    CU_pSuite p;
    LONG r;

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

    /*
     * Can it be done here at all? An earlier run that was cut short may
     * have left the module loaded: take it out first.
     */
    if (lib_opencount(DEMO_LIBRARY) >= 0) {
        demo_hold(0);
        unload_demo();
    }
    r = load_demo();
    if (r == 0 && module_cmd(NCMD_MODULE_QUERY, DEMO_MODULE, NULL, NULL, NULL) != 0)
        r = -2;                 /* a stack that does not know the command answers 0 to anything */
    if (r == 0)
        r = unload_demo() == 0 ? 0 : -3;
    if (r != 0) {
        if (r == ENOENT)
            printf("SKIP cunit-net-modlib: %s is not installed\n", DEMO_LIBRARY);
        else if (r == -1)
            printf("SKIP cunit-net-modlib: the stack has no control port\n");
        else if (r == -2)
            printf("SKIP cunit-net-modlib: the running stack cannot load module libraries\n");
        else
            printf("cunit-net-modlib: %s could not be %s (error %ld)\n", DEMO_LIBRARY,
                   r == -3 ? "unloaded again" : "loaded", (long)r);
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return (r == -3 || r > 0) && r != ENOENT ? 20 : 0;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("MODLIB_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "a module library is loaded, started, and gone when unloaded", testMODLIB_LOAD_UNLOAD, ST_NONE) ||
        !st_add_test(p, "loading it twice is refused", testMODLIB_LOAD_TWICE, ST_NONE) ||
        !st_add_test(p, "stopped it stays loaded; reloaded its data is new", testMODLIB_STOP_START, ST_NONE) ||
        !st_add_test(p, "in use it is neither stopped nor unloaded", testMODLIB_IN_USE_REFUSED, ST_NONE) ||
        !st_add_test(p, "what cannot be loaded or unloaded is refused", testMODLIB_REFUSALS, ST_NONE) ||
        !st_add_test(p, "twenty load/unload cycles leave memory as it was", testMODLIB_CYCLES_NO_DRIFT, ST_NONE))
    {
        CU_cleanup_registry();
        CloseLibrary(MiamiBase);
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-ModLib");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    CloseLibrary(MiamiBase);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
