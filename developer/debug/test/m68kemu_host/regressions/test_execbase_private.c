/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
   The actual production code for the parts of the program's ExecBase that
   exec keeps per CPU, for its scheduler alone, or behind its own locks,
   over a recording, standalone C fixture: the five empty lists and the
   scheduler fields, IDNestCnt and TDNestCnt, and FindName() on those
   lists. No AROS SDK, application, CPU core or C++ runtime is required. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uintptr_t IPTR;
typedef int BOOL;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0

struct Node { struct Node *ln_Succ, *ln_Pred; UBYTE ln_Type; BYTE ln_Pri; char *ln_Name; };
struct List { struct Node *lh_Head, *lh_Tail, *lh_TailPred; UBYTE lh_Type, l_pad; };
struct MsgPort { struct Node mp_Node; };
struct SignalSemaphore { int taken; };
struct M68KTaskState { LONG forbid, disable; };
struct M68KEmuContext
{
    UBYTE *mem; ULONG mem_size; BOOL low_guard;
    LONG forbid, disable;
    ULONG m68k_sysbase;
    struct SignalSemaphore gil;
    ULONG pubports[16];     /* the program's AddPort()ed ports: none here */
};
struct TestCPU { ULONG d[8], a[8]; };
#define THUNK_A(n) (((struct TestCPU *)cpu)->a[n])
#define THUNK_PTR(n) test_native(ctx, THUNK_A(n))

#include "constants.inc"

#define ARENA 8192u
#define BASE 2048u              /* the program's ExecBase */
#define EXECBASE_SIZE 632u
#define NAMES 4096u
static UBYTE arena[ARENA], saved[ARENA];
static struct M68KEmuContext context;
static unsigned checks, failures, dropped;
static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) { if (failures < 40) printf("FAIL: %s\n", label); ++failures; }
}
static void m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *who)
{ (void)ctx; (void)addr; (void)who; ++dropped; }
#include "accessors.inc"

static const UWORD private_lists[5] = { M68K_EXECBASE_INTRLIST, M68K_EXECBASE_PORTLIST,
    M68K_EXECBASE_TASKREADY, M68K_EXECBASE_TASKWAIT, M68K_EXECBASE_SEMAPHORELIST };
static const UBYTE private_types[5] = { NT_INTERRUPT, NT_MSGPORT, NT_TASK, NT_TASK, NT_SEMAPHORE };

/* What the bridge and exec would do, recorded. */
static struct List native_lists[5];        /* stand for the native ExecBase lists */
static BOOL lists_mapped;                  /* registered as shadows, as a tree that maps them does */
static unsigned lookups_of_private, native_findname, findport_calls, findsem_calls, shadows_made, native_pointers;
static const char *findport_name, *shadow_struct;
static void *shadow_native;
static struct MsgPort the_port;
static struct MsgPort *findport_result;
static IPTR findsem_result;

static void *shadow_lookup(struct M68KEmuContext *ctx, ULONG addr)
{
    (void)ctx;
    for (int i = 0; i < 5; i++)
        if (addr == BASE + private_lists[i])
        {
            ++lookups_of_private;
            return lists_mapped ? &native_lists[i] : NULL;
        }
    return NULL;
}
static void *test_native(struct M68KEmuContext *ctx, ULONG addr)
{
    void *shadow = shadow_lookup(ctx, addr);
    ++native_pointers;
    return shadow ? shadow : m68k_to_host(ctx, addr);
}
static struct Node *FindName(struct List *list, CONST_STRPTR name)
{ (void)list; (void)name; ++native_findname; return NULL; }
static struct MsgPort *FindPort(CONST_STRPTR name)
{ ++findport_calls; findport_name = name; return findport_result; }
static IPTR thunk_exec_FindSemaphore(struct M68KEmuContext *ctx, void *cpu)
{ (void)ctx; (void)cpu; ++findsem_calls; return findsem_result; }
static ULONG shadow_create_by_name(struct M68KEmuContext *ctx, const char *struct_name, void *native)
{ (void)ctx; ++shadows_made; shadow_struct = struct_name; shadow_native = native; return 0x00123456u; }
static ULONG openlibrary_calls;
static ULONG M68KEmu_OpenLibrary(struct M68KEmuContext *ctx, const char *name)
{ (void)ctx; (void)name; ++openlibrary_calls; return 0x00ABCDEFu; }
static ULONG M68KEmu_FindLibBase(struct M68KEmuContext *ctx, const char *name)
{ (void)ctx; (void)name; return 0; }
static struct M68KTaskState task_state;
static struct M68KTaskState *m68k_ts(struct M68KEmuContext *ctx) { (void)ctx; return &task_state; }
static void ObtainSemaphore(struct SignalSemaphore *sem) { ++sem->taken; }

#if HAVE_EXECBASE_PRIVATE
#include "private.inc"
#else
/* A tree that maps the lists: setting up the ExecBase registers the five
   lists as shadows of the native ones and writes nothing of the kind into
   the program's memory, and no call keeps the nesting counts. */
static void m68k_execbase_private(struct M68KEmuContext *ctx, ULONG base)
{ (void)ctx; (void)base; lists_mapped = TRUE; }
static void m68k_nest_counts(struct M68KEmuContext *ctx) { (void)ctx; }
#endif
/* the thunks take the CPU state whether they read it or not */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "thunks.inc"
#pragma GCC diagnostic pop
#include "tasks.inc"

static void put32(ULONG at, ULONG v)
{
    assert((uint64_t)at + 4 <= ARENA);
    arena[at] = (UBYTE)(v >> 24); arena[at+1] = (UBYTE)(v >> 16); arena[at+2] = (UBYTE)(v >> 8); arena[at+3] = (UBYTE)v;
}
static ULONG get32(ULONG at)
{
    assert((uint64_t)at + 4 <= ARENA);
    return (ULONG)arena[at] << 24 | (ULONG)arena[at+1] << 16 | (ULONG)arena[at+2] << 8 | arena[at+3];
}
static UWORD get16(ULONG at) { return (UWORD)(arena[at] << 8 | arena[at+1]); }
/* s is a string literal: its size is known, terminator included */
#define name_at(at, s) (memcpy(arena + (at), (s), sizeof(s)), (ULONG)(at))
static void reset(void)
{
    memset(arena, 0xA5, sizeof(arena));
    memset(&context, 0, sizeof(context));
    context.mem = arena; context.mem_size = ARENA; context.m68k_sysbase = BASE;
    memset(&task_state, 0, sizeof(task_state));
    lists_mapped = FALSE; dropped = 0;
    lookups_of_private = native_findname = findport_calls = findsem_calls = shadows_made = native_pointers = 0;
    openlibrary_calls = 0;
    findport_name = shadow_struct = NULL; shadow_native = NULL; findport_result = NULL; findsem_result = 0;
}
/* the two counts are signed bytes */
static int nest_count(ULONG at) { int v = arena[at]; return v < 128 ? v : v - 256; }
static int id(void) { return nest_count(BASE + M68K_EXECBASE_IDNESTCNT); }
static int td(void) { return nest_count(BASE + M68K_EXECBASE_TDNESTCNT); }
static IPTR find_name(ULONG list, ULONG name)
{
    struct TestCPU cpu = {{0},{0}}; cpu.a[0] = list; cpu.a[1] = name;
    return thunk_exec_FindName(&context, &cpu);
}

/* The lists and the scheduler fields the program is given. */
static void layout(void)
{
    static const char *const names[5] = { "IntrList", "PortList", "TaskReady", "TaskWait", "SemaphoreList" };
    char label[96];
    unsigned i, at, before = checks, failed = failures;

    reset(); context.forbid = context.disable = 0;
    memcpy(saved, arena, ARENA);
    m68k_execbase_private(&context, BASE);
    for (i = 0; i < 5; i++)
    {
        ULONG l = BASE + private_lists[i];
        snprintf(label, sizeof(label), "%s: lh_Head is the tail node", names[i]);
        check(get32(l) == l + 4, label);
        snprintf(label, sizeof(label), "%s: lh_Tail is 0", names[i]);
        check(get32(l + 4) == 0, label);
        snprintf(label, sizeof(label), "%s: lh_TailPred is the head node, so the list is empty", names[i]);
        check(get32(l + 8) == l, label);
        snprintf(label, sizeof(label), "%s: its node type, and no padding left over", names[i]);
        check(arena[l + 12] == private_types[i] && arena[l + 13] == 0, label);
        snprintf(label, sizeof(label), "%s: not registered as a shadow of a native list", names[i]);
        check(shadow_lookup(&context, l) == NULL, label);
    }
    check(get32(BASE + M68K_EXECBASE_IDLECOUNT) == 0 && get32(BASE + M68K_EXECBASE_DISPCOUNT) == 0, "IdleCount and DispCount are the emulator's 0");
    check(get16(BASE + M68K_EXECBASE_QUANTUM) == 4 && get16(BASE + M68K_EXECBASE_ELAPSED) == 4, "Quantum and Elapsed are exec's time slice");
    check(get16(BASE + M68K_EXECBASE_SYSFLAGS) == 0 && get16(BASE + M68K_EXECBASE_ATTNRESCHED) == 0, "SysFlags and AttnResched are 0");
    check(id() == -1 && td() == -1, "IDNestCnt and TDNestCnt are -1 with no nesting");
    /* everything else in and around the ExecBase is as it was */
    for (at = 0; at < ARENA; at++)
    {
        BOOL mine = FALSE;
        for (i = 0; i < 5; i++)
            if (at >= BASE + private_lists[i] && at < BASE + private_lists[i] + 14u) mine = TRUE;
        if (at >= BASE + M68K_EXECBASE_IDLECOUNT && at < BASE + M68K_EXECBASE_ATTNFLAGS) mine = TRUE;
        if (at >= BASE + M68K_EXECBASE_ATTNRESCHED && at < BASE + M68K_EXECBASE_ATTNRESCHED + 2u) mine = TRUE;
        if (!mine && arena[at] != saved[at]) break;
    }
    check(at == ARENA, "nothing else is written: ThisTask, AttnFlags, the other lists, the rest of memory");
    check(!dropped, "no write was refused");
    printf("layout: %u checks, %u failed\n", checks - before, failures - failed);
}

/* IDNestCnt and TDNestCnt follow the program's own Disable() and Forbid(). */
static void nesting(void)
{
    static const struct { LONG forbid, disable; int id, td; } table[] =
    {
        { 0, 0, -1, -1 }, { 1, 0, -1, 0 }, { 3, 0, -1, 2 },
        { 1, 1, 0, -1 },        /* one Disable(): it counts as a Forbid() inside the emulator only */
        { 2, 1, 0, 0 }, { 3, 2, 1, 0 }, { 5, 2, 1, 2 },
        { 0, 1, 0, -1 },        /* a Permit() too many under Disable(): TDNestCnt does not go below -1 */
    };
    struct TestCPU cpu = {{0},{0}};
    char label[96];
    unsigned i, before = checks, failed = failures;

    for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        reset(); m68k_execbase_private(&context, BASE);
        context.forbid = table[i].forbid; context.disable = table[i].disable;
        m68k_nest_counts(&context);
        snprintf(label, sizeof(label), "forbid %d, disable %d give IDNestCnt %d and TDNestCnt %d",
                 (int)table[i].forbid, (int)table[i].disable, table[i].id, table[i].td);
        check(id() == table[i].id && td() == table[i].td, label);
    }
    reset(); m68k_execbase_private(&context, BASE);
    thunk_exec_Forbid(&context, &cpu);     check(td() == 0 && id() == -1, "Forbid() raises TDNestCnt to 0");
    thunk_exec_Forbid(&context, &cpu);     check(td() == 1 && id() == -1, "a nested Forbid() raises it to 1");
    thunk_exec_Permit(&context, &cpu);     check(td() == 0, "Permit() lowers it");
    thunk_exec_Permit(&context, &cpu);     check(td() == -1, "the last Permit() gives -1");
    thunk_exec_Permit(&context, &cpu);     check(td() == -1 && id() == -1, "a Permit() too many changes nothing");
    thunk_exec_Disable_m(&context, &cpu);  check(id() == 0 && td() == -1, "Disable() raises IDNestCnt and leaves TDNestCnt");
    thunk_exec_Forbid(&context, &cpu);     check(id() == 0 && td() == 0, "Forbid() under Disable() shows in TDNestCnt");
    thunk_exec_Disable_m(&context, &cpu);  check(id() == 1 && td() == 0, "a nested Disable() raises IDNestCnt to 1");
    thunk_exec_Enable_m(&context, &cpu);   check(id() == 0 && td() == 0, "Enable() lowers it");
    thunk_exec_Enable_m(&context, &cpu);   check(id() == -1 && td() == 0, "the last Enable() gives -1, the Forbid() stays");
    thunk_exec_Enable_m(&context, &cpu);   check(id() == -1 && td() == 0, "an Enable() too many changes nothing");
    thunk_exec_Permit(&context, &cpu);     check(id() == -1 && td() == -1, "and the Permit() ends the nesting");
    /* a task of the program that takes the lock brings its own nesting */
    reset(); m68k_execbase_private(&context, BASE);
    task_state.forbid = 2; task_state.disable = 1;
    m68k_gil_obtain(&context);
    check(context.forbid == 2 && context.disable == 1 && !task_state.forbid && !task_state.disable && context.gil.taken == 1,
          "the task that takes the lock gets its nesting back");
    check(id() == 0 && td() == 0, "and the ExecBase shows that task's nesting");
    task_state.forbid = task_state.disable = 0;
    m68k_gil_obtain(&context);
    check(id() == -1 && td() == -1, "a task without nesting shows -1 again");
    /* no ExecBase yet, or no memory: nothing is written */
    reset(); memcpy(saved, arena, ARENA);
    context.m68k_sysbase = 0; context.forbid = 3;
    m68k_nest_counts(&context);
    check(!memcmp(saved, arena, ARENA), "without an ExecBase the counts are written nowhere");
    printf("nesting: %u checks, %u failed\n", checks - before, failures - failed);
}

/* FindName() on the lists: never the native list, the locking calls where there is one. */
static void lookup(void)
{
    ULONG rexx, absent, list, n1, n2;
    IPTR r;
    unsigned i, before = checks, failed = failures;

    reset(); m68k_execbase_private(&context, BASE);
    rexx = name_at(NAMES, "REXX");
    findport_result = &the_port;
    r = find_name(BASE + M68K_EXECBASE_PORTLIST, rexx);
    check(findport_calls == 1 && findport_name && !strcmp(findport_name, "REXX"), "PortList: the name goes to FindPort() once");
    check(shadows_made == 1 && shadow_struct && !strcmp(shadow_struct, "MsgPort") && shadow_native == &the_port,
          "PortList: the port found is given as a MsgPort shadow");
    check(r == 0x00123456u, "PortList: FindName() returns that shadow");
    check(!native_findname && !native_pointers, "PortList: the native list is neither walked nor handed to a native call");
    reset(); m68k_execbase_private(&context, BASE);
    absent = name_at(NAMES + 16, "no such port");
    r = find_name(BASE + M68K_EXECBASE_PORTLIST, absent);
    check(r == 0 && findport_calls == 1 && !shadows_made && !native_findname, "PortList: a name FindPort() does not know gives 0 and no shadow");
    r = find_name(BASE + M68K_EXECBASE_PORTLIST, 0);
    check(r == 0 && findport_calls == 1 && !native_findname, "PortList: no name gives 0 without a call");
    reset(); m68k_execbase_private(&context, BASE);
    rexx = name_at(NAMES, "REXX");
    findsem_result = 0x00777000u;
    r = find_name(BASE + M68K_EXECBASE_SEMAPHORELIST, rexx);
    check(findsem_calls == 1 && r == 0x00777000u, "SemaphoreList: the lookup is FindSemaphore()'s, with its result");
    check(!findport_calls && !native_findname && !native_pointers, "SemaphoreList: no other lookup and no native list");
    for (i = 0; i < 5; i++)
    {
        static const char *const labels[5] = {
            "IntrList: FindName() finds nothing and touches no native list",
            "PortList", "TaskReady: FindName() finds nothing and touches no native list",
            "TaskWait: FindName() finds nothing and touches no native list", "SemaphoreList" };
        if (i == 1 || i == 4) continue;
        reset(); m68k_execbase_private(&context, BASE);
        rexx = name_at(NAMES, "REXX");
        r = find_name(BASE + private_lists[i], rexx);
        check(r == 0 && !native_findname && !native_pointers && !findport_calls && !findsem_calls, labels[i]);
    }
    /* the lists that hold the program's own bases, and a list of its own, are as before */
    reset(); m68k_execbase_private(&context, BASE);
    rexx = name_at(NAMES, "REXX");
    r = find_name(BASE + M68K_EXECBASE_LIBLIST, rexx);
    check(r == 0x00ABCDEFu && openlibrary_calls == 1 && !native_findname, "LibList still opens the library for the program");
    check(find_name(BASE + M68K_EXECBASE_DEVICELIST, rexx) == 0 && find_name(BASE + M68K_EXECBASE_RESOURCELIST, rexx) == 0 && !native_findname,
          "DeviceList and ResourceList still give 0");
    list = 6144; n1 = 6200; n2 = 6240;
    put32(list, n1); put32(list + 4, 0); put32(list + 8, n2);
    put32(n1, n2); put32(n1 + 4, list); put32(n1 + 10, name_at(NAMES + 64, "alpha"));
    put32(n2, list + 4); put32(n2 + 4, n1); put32(n2 + 10, name_at(NAMES + 80, "beta"));
    check(find_name(list, name_at(NAMES + 96, "beta")) == n2 && find_name(list, name_at(NAMES + 112, "gamma")) == 0 && !native_findname,
          "a list in the program's memory is still searched there");
    printf("lookup: %u checks, %u failed\n", checks - before, failures - failed);
}

int main(void)
{
    layout();
    nesting();
    lookup();
    printf("execbase private: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
