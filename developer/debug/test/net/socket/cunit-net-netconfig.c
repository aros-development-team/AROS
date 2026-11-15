/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for the configuration readers as a module of their own
    (module "netconfig", netconfig.atcp; bsdsocket/kern/
    netconfig_ops.h in the stack): the code that reads db/general.config,
    db/netdb and the files it includes is in memory only while a
    configuration is read.

     - after the start it is not loaded (NCMD_MODULE_QUERY: not known, or
       known and not started where it is part of the stack program);
     - a reload (NCMD_RELOAD) has it loaded, reads the files - a line added
       to db/netdb is in effect after the reload - and drops it again;
     - with ENV:AROSTCP/Without_netconfig set (an installation without the
       library) a reload is refused, nothing of the configuration changes,
       and the stack keeps running on what it has;
     - ten reloads leave free memory as it was.

    What a reload reads again is db/netdb (and db/services.d), not
    general.config: the visible change is a SERVICE line added to db/netdb,
    looked up with getservbyname(), which the core answers without any
    module. Every file this program changes is written back as it was, also
    when a test fails (clean_suite()).

    The configuration directory is the stack's: the variable AROSTCP/Config
    when it is set, else ENV:AROSTCP/db. Where db/netdb cannot be found or
    written, or the stack has no control port, the tests say SKIP.
*/

#include "socktest.h"

#include <proto/dos.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <dos/var.h>
#include <netdb.h>
#include <bsdsocket/netcontrol.h>

#include <stdlib.h>

struct Library *SocketBase = NULL;

#define PROGRAM             "cunit-net-netconfig"
#define NETCONFIG_LIBRARY   "netconfig.atcp"
#define NETCONFIG_MODULE    "netconfig"
#define NETCONFIG_SWITCH    "AROSTCP/Without_netconfig"

/* two services no installation has, on ports from the private range */
#define SVC_A               "netcfgtest-a"
#define SVC_A_PORT          49151
#define SVC_B               "netcfgtest-b"
#define SVC_B_PORT          49150

#define RELOADS             10
/*
 * A reload takes every interface's addresses, routes and DNS servers down
 * and makes them again, and cycles the rc and restart-policy daemons: the
 * pools it allocates from may keep a little more or less afterwards. One
 * reload is made before the count starts; the bound is for what ten more
 * may leave behind, against a library of about 10 KB that is loaded and
 * unloaded each time.
 */
#define DRIFT_MAX           16384

static char netdb_file[256];        /* the stack's db/netdb */
static UBYTE *netdb_saved = NULL;   /* its contents as they were */
static LONG netdb_saved_len = 0;
static int netdb_changed = 0;       /* written by this program and not yet restored */
/* exec's open count of the reader's library when this program started, before it sent anything */
static LONG lib_at_start = -2;

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

static LONG reload(void)
{
    return module_cmd(NCMD_RELOAD, NULL, NULL);
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

/*
 * Whether the reader is in memory and running: not known to the stack (a
 * library that is not loaded), or known and not started (part of the
 * program), is "not loaded".
 */
static int reader_running(ULONG *flagsp)
{
    ULONG flags = 0;
    LONG r = module_cmd(NCMD_MODULE_QUERY, NETCONFIG_MODULE, &flags);

    if (flagsp)
        *flagsp = flags;
    if (r == ENOENT)
        return 0;
    return r == 0 && (flags & NCMMF_STARTED);
}

/* ---- the stack's db/netdb --------------------------------------------- */

static int netdb_find(void)
{
    char dir[200];
    BPTR lock;

    if (GetVar("AROSTCP/Config", dir, sizeof(dir), GVF_GLOBAL_ONLY) < 0)
        strcpy(dir, "ENV:AROSTCP/db");
    strncpy(netdb_file, dir, sizeof(netdb_file) - 1);
    if (!AddPart(netdb_file, "netdb", sizeof(netdb_file)))
        return -1;
    if ((lock = Lock(netdb_file, ACCESS_READ)) == BNULL)
        return -1;
    UnLock(lock);
    return 0;
}

static int netdb_save(void)
{
    BPTR fh;
    LONG len;

    if ((fh = Open(netdb_file, MODE_OLDFILE)) == BNULL)
        return -1;
    Seek(fh, 0, OFFSET_END);
    len = Seek(fh, 0, OFFSET_BEGINNING);
    if (len < 0 || (netdb_saved = AllocVec(len + 1, MEMF_ANY)) == NULL ||
            Read(fh, netdb_saved, len) != len) {
        Close(fh);
        return -1;
    }
    Close(fh);
    netdb_saved_len = len;
    return 0;
}

/* The file as it was, with `extra' (or nothing) at its end. 0: written. */
static int netdb_write(const char *extra)
{
    BPTR fh;
    int ok;

    if (netdb_saved == NULL || (fh = Open(netdb_file, MODE_NEWFILE)) == BNULL)
        return -1;
    netdb_changed = 1;
    ok = Write(fh, netdb_saved, netdb_saved_len) == netdb_saved_len;
    if (ok && extra != NULL) {
        if (netdb_saved_len > 0 && netdb_saved[netdb_saved_len - 1] != '\n')
            ok = Write(fh, "\n", 1) == 1;
        if (ok)
            ok = Write(fh, (APTR)extra, strlen(extra)) == (LONG)strlen(extra);
    }
    Close(fh);
    return ok ? 0 : -1;
}

/* Everything this program changed back as it was, and read so by the stack. */
static void restore_all(void)
{
    DeleteVar(NETCONFIG_SWITCH, GVF_GLOBAL_ONLY);
    if (netdb_changed && netdb_write(NULL) == 0) {
        netdb_changed = 0;
        reload();
    }
}

/* The port of a service by name over TCP, -1 when there is none. */
static int service_port(const char *name)
{
    struct servent *se = getservbyname((STRPTR)name, "tcp");

    return se ? ntohs(se->s_port) : -1;
}

/* ---- the suite -------------------------------------------------------- */

static int init_suite(void) { return 0; }

static int clean_suite(void)
{
    restore_all();
    return 0;
}

/*
 * After the start the reader's library is not in memory - looked at in
 * exec's library list before this program sent the stack anything (a
 * control message is answered from the stack's main loop, and could itself
 * be what has the library dropped).
 */
static void testNETCONFIG_NO_LIBRARY_BEFORE_ANY_MESSAGE(void)
{
    if (lib_at_start >= 0)
        printf("\n    %s is in memory (open %ld) as the program starts",
               NETCONFIG_LIBRARY, (long)lib_at_start);
    CU_ASSERT_EQUAL(lib_at_start, -1);
}

/* After the start the reader is not in memory. */
static void testNETCONFIG_NOT_LOADED_AFTER_START(void)
{
    ULONG flags = 0;

    if (reader_running(&flags))
        printf("\n    the reader is running: is an interface marked DEFER waiting for its device?");
    CU_ASSERT_FALSE(reader_running(&flags));
    if (!(flags & NCMMF_BUILTIN))
        CU_ASSERT_EQUAL(lib_opencount(NETCONFIG_LIBRARY), -1);
}

/* A reload reads db/netdb again - through the reader - and drops it after. */
static void testNETCONFIG_RELOAD_READS_AND_DROPS(void)
{
    char line[80];

    CU_ASSERT_EQUAL_FATAL(service_port(SVC_A), -1);
    snprintf(line, sizeof(line), "SERVICE %s %d/tcp\n", SVC_A, SVC_A_PORT);
    CU_ASSERT_EQUAL_FATAL(netdb_write(line), 0);

    CU_ASSERT_EQUAL(reload(), 0);
    CU_ASSERT_EQUAL(service_port(SVC_A), SVC_A_PORT);
    CU_ASSERT_FALSE(reader_running(NULL));
    CU_ASSERT_EQUAL(lib_opencount(NETCONFIG_LIBRARY), -1);

    /* and back */
    CU_ASSERT_EQUAL(netdb_write(NULL), 0);
    CU_ASSERT_EQUAL(reload(), 0);
    netdb_changed = 0;
    CU_ASSERT_EQUAL(service_port(SVC_A), -1);
    CU_ASSERT_FALSE(reader_running(NULL));
}

/*
 * Without the reader a reload is refused: the configuration stays as it
 * is - the line read by the last reload is still there, the one added
 * since is not - and the stack runs on.
 */
static void testNETCONFIG_RELOAD_REFUSED_WITHOUT(void)
{
    char line[160];
    LONG r;
    int s;

    /* a configuration with A, in effect */
    snprintf(line, sizeof(line), "SERVICE %s %d/tcp\n", SVC_A, SVC_A_PORT);
    CU_ASSERT_EQUAL_FATAL(netdb_write(line), 0);
    CU_ASSERT_EQUAL_FATAL(reload(), 0);
    CU_ASSERT_EQUAL_FATAL(service_port(SVC_A), SVC_A_PORT);

    /* the file now says B instead; the reader cannot be had */
    snprintf(line, sizeof(line), "SERVICE %s %d/tcp\n", SVC_B, SVC_B_PORT);
    CU_ASSERT_EQUAL_FATAL(netdb_write(line), 0);
    CU_ASSERT_FATAL(SetVar(NETCONFIG_SWITCH, "1", 1, GVF_GLOBAL_ONLY));
    r = reload();
    printf("\n    reload without the reader: %ld", (long)r);
    CU_ASSERT(r != 0 && r != -1);
    CU_ASSERT_EQUAL(service_port(SVC_A), SVC_A_PORT);
    CU_ASSERT_EQUAL(service_port(SVC_B), -1);
    CU_ASSERT_FALSE(reader_running(NULL));

    /* the stack runs on */
    s = socket(st_any_family(), SOCK_DGRAM, 0);
    CU_ASSERT(s >= 0);
    if (s >= 0)
        CloseSocket(s);
    CU_ASSERT_EQUAL(module_cmd(NCMD_NOP, NULL, NULL), 0);

    /* with the reader again, the file is read */
    DeleteVar(NETCONFIG_SWITCH, GVF_GLOBAL_ONLY);
    CU_ASSERT_EQUAL(reload(), 0);
    CU_ASSERT_EQUAL(service_port(SVC_B), SVC_B_PORT);
    CU_ASSERT_EQUAL(service_port(SVC_A), -1);

    CU_ASSERT_EQUAL(netdb_write(NULL), 0);
    CU_ASSERT_EQUAL(reload(), 0);
    netdb_changed = 0;
    CU_ASSERT_EQUAL(service_port(SVC_B), -1);
}

/* Ten reloads, each loading and dropping the reader, leave memory as it was. */
static void testNETCONFIG_RELOADS_NO_DRIFT(void)
{
    ULONG before, after;
    LONG drift;
    int i;

    CU_ASSERT_EQUAL_FATAL(reload(), 0);     /* what a first reload settles */
    before = AvailMem(MEMF_ANY);
    for (i = 0; i < RELOADS; i++)
        if (reload() != 0 || reader_running(NULL))
            break;
    after = AvailMem(MEMF_ANY);
    drift = (LONG)before - (LONG)after;
    printf("\n    free memory after %d reloads: %ld bytes less (limit %d)",
           i, (long)drift, DRIFT_MAX);
    CU_ASSERT_EQUAL(i, RELOADS);
    CU_ASSERT(drift < DRIFT_MAX);
    CU_ASSERT(drift > -DRIFT_MAX);
}

/* ---- main ------------------------------------------------------------- */

int main(int argc, char **argv)
{
    CU_pSuite p;
    int files;

    /* first of all: nothing has been asked of the stack yet */
    lib_at_start = lib_opencount(NETCONFIG_LIBRARY);

    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (SocketBase == NULL) {
        printf("no bsdsocket.library\n");
        return 20;
    }
    if (module_cmd(NCMD_NOP, NULL, NULL) != 0) {
        printf("SKIP %s: the stack has no control port\n", PROGRAM);
        CloseLibrary(SocketBase);
        return 0;
    }
    /* left over from an interrupted run */
    DeleteVar(NETCONFIG_SWITCH, GVF_GLOBAL_ONLY);
    files = netdb_find() == 0 && netdb_save() == 0;
    if (!files)
        printf("SKIP the reload tests: the stack's db/netdb was not found (%s)\n", netdb_file);

    if (CU_initialize_registry() != CUE_SUCCESS) {
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    p = CU_add_suite("NETCONFIG_Suite", init_suite, clean_suite);
    if (p == NULL ||
        !st_add_test(p, "the reader's library is not in memory before any message", testNETCONFIG_NO_LIBRARY_BEFORE_ANY_MESSAGE, ST_NONE) ||
        !st_add_test(p, "after the start the reader is not loaded", testNETCONFIG_NOT_LOADED_AFTER_START, ST_NONE) ||
        (files && (
        !st_add_test(p, "a reload reads db/netdb through the reader and drops it", testNETCONFIG_RELOAD_READS_AND_DROPS, ST_NONE) ||
        !st_add_test(p, "without the reader a reload is refused and the stack runs on", testNETCONFIG_RELOAD_REFUSED_WITHOUT, ST_ANY) ||
        !st_add_test(p, "ten reloads leave memory as it was", testNETCONFIG_RELOADS_NO_DRIFT, ST_NONE))))
    {
        CU_cleanup_registry();
        restore_all();
        CloseLibrary(SocketBase);
        return CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("NetUnitTests");
    CU_set_output_filename("Net-Netconfig");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();
    restore_all();
    if (netdb_saved)
        FreeVec(netdb_saved);
    CloseLibrary(SocketBase);
    return CU_get_error();
}
