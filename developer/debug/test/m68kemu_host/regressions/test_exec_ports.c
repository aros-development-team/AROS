/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The exec thunks for AddPort, RemPort, FindPort, CreateIORequest,
    DeleteIORequest, AddTask, RemTask, CopyMem/CopyMemQuick and NewMinList,
    with the accessors they use, taken verbatim from rom/m68kemu by
    run_exec_ports.py. This file supplies the context fields, the CPU
    registers, the heap and the native calls they reach, and records them.
*/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef int BOOL;
typedef uintptr_t IPTR;
typedef void *APTR;
typedef uintptr_t BPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#define NT_MSGPORT 4
#define NT_MESSAGE 5
#define NT_REPLYMSG 7
#define MEMF_PUBLIC (1UL << 0)
#define MEMF_CLEAR (1UL << 16)

#include "constants.inc"

struct MsgPort;
struct MinList;
struct Task;
struct IORequest;
struct M68KStructLayout { int unused; };

struct M68KShadowEntry
{
    ULONG m68k_addr;
    void *native_ptr;
    UWORD type;
};

struct M68KEmuContext
{
    UBYTE *mem;
    ULONG mem_size;
    BOOL low_guard;
    BPTR *handles;
    ULONG num_handles;
    UWORD num_shadows;
    struct M68KShadowEntry shadow_map[8];
    ULONG pubports[16];
    char program_name[256];
};

struct Cpu { ULONG d[8], a[8]; };
static ULONG M68KEmu_GetD(void *cpu, int n) { return ((struct Cpu *)cpu)->d[n]; }
static ULONG M68KEmu_GetA(void *cpu, int n) { return ((struct Cpu *)cpu)->a[n]; }
#define THUNK_D(n)    M68KEmu_GetD(cpu, n)
#define THUNK_A(n)    M68KEmu_GetA(cpu, n)
#define THUNK_PTR(n)  m68k_to_host_or_shadow(ctx, M68KEmu_GetA(cpu, n))
#define THUNK_DPTR(n) m68k_to_host_or_shadow(ctx, M68KEmu_GetD(cpu, n))

/* what the thunks reach, recorded */
static unsigned logs, low_traps, stops, native_findport, native_deleteio, native_newminlist, shadows_made, shadows_destroyed;
static ULONG heap_next = 0x8000, last_alloc_size, last_alloc_flags, last_free_addr, last_free_size;
static UBYTE native_port[64], native_io[64], native_list[16], native_task[16];
static ULONG own_task_m68k = 0x700;

static void m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *what) { (void)ctx; (void)addr; (void)what; low_traps++; }
static void m68k_log(struct M68KEmuContext *ctx, const char *fmt, ...) { (void)ctx; (void)fmt; logs++; }

#include "accessors.inc"

static void *shadow_lookup(struct M68KEmuContext *ctx, ULONG addr)
{
    for (UWORD i = 0; i < ctx->num_shadows; i++)
        if (ctx->shadow_map[i].m68k_addr == addr)
            return ctx->shadow_map[i].native_ptr;
    return NULL;
}

static const struct M68KStructLayout io_layout;
static const struct M68KStructLayout *shadow_find_layout(const char *name) { (void)name; return &io_layout; }
static void shadow_destroy(struct M68KEmuContext *ctx, const struct M68KStructLayout *l, ULONG m) { (void)ctx; (void)l; (void)m; shadows_destroyed++; }
static ULONG shadow_create_by_name(struct M68KEmuContext *ctx, const char *name, void *native) { (void)ctx; (void)name; (void)native; shadows_made++; return 0x6000; }

static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG size, ULONG flags)
{
    ULONG a = heap_next;
    last_alloc_size = size;
    last_alloc_flags = flags;
    heap_next += (size + 7) & ~7u;
    if (flags & MEMF_CLEAR)
        memset(ctx->mem + a, 0, size);
    else
        memset(ctx->mem + a, 0xEE, size);
    return a;
}
static void M68KEmu_HeapFree(struct M68KEmuContext *ctx, ULONG addr, ULONG size) { (void)ctx; last_free_addr = addr; last_free_size = size; }

static struct MsgPort *FindPort(CONST_STRPTR name)
{
    native_findport++;
    return strcmp(name, "system port") == 0 ? (struct MsgPort *)native_port : NULL;
}
static void DeleteIORequest(APTR io) { (void)io; native_deleteio++; }
static void NewMinList(struct MinList *l) { (void)l; native_newminlist++; }
static struct Task *FindTask(CONST_STRPTR name) { (void)name; return (struct Task *)native_task; }
static ULONG m68k_process_of(struct M68KEmuContext *ctx, struct Task *t) { (void)ctx; return t == (struct Task *)native_task ? own_task_m68k : 0; }
static void M68KEmu_StopCpu(struct M68KEmuContext *ctx, void *cpu) { (void)ctx; (void)cpu; stops++; }

#include "thunks.inc"

static unsigned checks, failures;
static void check(int ok, const char *what)
{
    checks++;
    if (!ok)
    {
        failures++;
        printf("FAIL: %s\n", what);
    }
}

static UBYTE arena[0x10000];
static struct M68KEmuContext ctx_storage;

static void reset(struct M68KEmuContext *ctx)
{
    memset(arena, 0, sizeof(arena));
    memset(ctx, 0, sizeof(*ctx));
    ctx->mem = arena;
    ctx->mem_size = sizeof(arena);
    ctx->low_guard = TRUE;
    heap_next = 0x8000;
    logs = low_traps = stops = native_findport = native_deleteio = native_newminlist = shadows_made = shadows_destroyed = 0;
}

static void put_string(ULONG at, const char *s) { memcpy(arena + at, s, strlen(s) + 1); }

static void ports(struct M68KEmuContext *ctx)
{
    struct Cpu cpu = {{0}, {0}};
    const ULONG P = 0x1000, name = 0x1100, other = 0x1200, none = 0x1300, sysname = 0x1400, S = 0x2000;
    ULONG r;

    reset(ctx);
    put_string(name, "m68kemu test port");
    put_string(none, "no such port");
    put_string(sysname, "system port");
    m68k_write32(ctx, P + 10, name);
    cpu.a[1] = P;
    thunk_exec_AddPort(ctx, &cpu);
    check(arena[P + 8] == NT_MSGPORT, "AddPort: ln_Type is NT_MSGPORT");
    check(m68k_read32(ctx, P + 20) == P + 24 && m68k_read32(ctx, P + 24) == 0 && m68k_read32(ctx, P + 28) == P + 20,
          "AddPort: mp_MsgList is an empty list in the program's memory");
    check(arena[P + 32] == NT_MESSAGE, "AddPort: the list holds messages");
    check(ctx->pubports[0] == P, "AddPort: the port is in the table for FindPort");
    thunk_exec_AddPort(ctx, &cpu);
    check(ctx->pubports[1] == 0, "AddPort twice: one entry");

    cpu.a[1] = name;
    r = thunk_exec_FindPort(ctx, &cpu);
    check(r == P && native_findport == 0, "FindPort finds the program's port first, without asking the system");
    cpu.a[1] = sysname;
    r = thunk_exec_FindPort(ctx, &cpu);
    check(r == 0x6000 && native_findport == 1 && shadows_made == 1, "FindPort of a system port: a shadow of it");
    cpu.a[1] = none;
    check(thunk_exec_FindPort(ctx, &cpu) == 0, "FindPort of an unknown name: 0");
    cpu.a[1] = 0;
    check(thunk_exec_FindPort(ctx, &cpu) == 0, "FindPort(NULL): 0");

    cpu.a[1] = P;
    thunk_exec_RemPort(ctx, &cpu);
    cpu.a[1] = name;
    check(thunk_exec_FindPort(ctx, &cpu) == 0, "after RemPort FindPort does not find it");

    /* a port the program got from the system is left as it is */
    ctx->shadow_map[0].m68k_addr = S;
    ctx->shadow_map[0].native_ptr = native_port;
    ctx->num_shadows = 1;
    cpu.a[1] = S;
    thunk_exec_AddPort(ctx, &cpu);
    check(arena[S + 8] == 0 && m68k_read32(ctx, S + 20) == 0 && ctx->pubports[0] == 0, "AddPort of a shadow: untouched, not named");

    /* sixteen named ports at most */
    for (int i = 0; i < 17; i++)
    {
        cpu.a[1] = other + 0x40 * i;
        thunk_exec_AddPort(ctx, &cpu);
    }
    check(ctx->pubports[15] == other + 0x40 * 15 && logs == 1, "the seventeenth port: a line in the log");

    /* a port that does not fit the program's memory */
    reset(ctx);
    cpu.a[1] = sizeof(arena) - 20;
    thunk_exec_AddPort(ctx, &cpu);
    check(ctx->pubports[0] == 0 && low_traps == 0, "AddPort of a port past the end of the program's memory: nothing");
}

static void requests(struct M68KEmuContext *ctx)
{
    struct Cpu cpu = {{0}, {0}};
    const ULONG P = 0x1000, S = 0x3000;
    ULONG io;

    reset(ctx);
    cpu.a[0] = P;
    cpu.d[0] = 48;
    io = thunk_exec_CreateIORequest_m(ctx, &cpu);
    check(io >= 0x8000 && last_alloc_size == 48 && (last_alloc_flags & MEMF_CLEAR), "CreateIORequest: 48 cleared bytes in the program's memory");
    check(arena[io + 8] == NT_REPLYMSG && m68k_read32(ctx, io + 14) == P && m68k_read16(ctx, io + 18) == 48,
          "CreateIORequest: ln_Type, mn_ReplyPort and mn_Length set");
    check(m68k_read32(ctx, io + 40) == 0 && m68k_read32(ctx, io + 44) == 0, "CreateIORequest: the fields past 32 bytes are the program's too");
    cpu.a[0] = io;
    thunk_exec_DeleteIORequest_m(ctx, &cpu);
    check(last_free_addr == io && last_free_size == 48, "DeleteIORequest frees the size of mn_Length");

    cpu.a[0] = 0;
    cpu.d[0] = 48;
    check(thunk_exec_CreateIORequest_m(ctx, &cpu) == 0, "CreateIORequest without a port: NULL");
    cpu.a[0] = P;
    cpu.d[0] = 20;
    check(thunk_exec_CreateIORequest_m(ctx, &cpu) == 0, "CreateIORequest smaller than an IORequest: NULL");

    ctx->shadow_map[0].m68k_addr = S;
    ctx->shadow_map[0].native_ptr = native_io;
    ctx->num_shadows = 1;
    cpu.a[0] = S;
    last_free_addr = 0;
    thunk_exec_DeleteIORequest_m(ctx, &cpu);
    check(native_deleteio == 1 && shadows_destroyed == 1 && last_free_addr == 0, "DeleteIORequest of a system request: back to the system");
    cpu.a[0] = 0;
    thunk_exec_DeleteIORequest_m(ctx, &cpu);
    check(native_deleteio == 1, "DeleteIORequest(NULL): nothing");
}

static void tasks(struct M68KEmuContext *ctx)
{
    struct Cpu cpu = {{0}, {0}};

    reset(ctx);
    cpu.a[1] = 0x1000;
    cpu.a[2] = 0x2000;
    check(thunk_exec_AddTask_m(ctx, &cpu) == 0 && logs == 1 && stops == 0, "AddTask: NULL and a line in the log");
    cpu.a[1] = 0;
    thunk_exec_RemTask_m(ctx, &cpu);
    check(stops == 1, "RemTask(NULL) ends the calling task");
    cpu.a[1] = own_task_m68k;
    thunk_exec_RemTask_m(ctx, &cpu);
    check(stops == 2, "RemTask of the calling task ends it");
    cpu.a[1] = 0x1234;
    thunk_exec_RemTask_m(ctx, &cpu);
    check(stops == 2 && logs == 2, "RemTask of another task: not removed, a line in the log");
}

static void copies(struct M68KEmuContext *ctx)
{
    struct Cpu cpu = {{0}, {0}};
    const ULONG end = sizeof(arena);

    reset(ctx);
    memcpy(arena + 0x1000, "0123456789", 10);
    cpu.a[0] = 0x1000;
    cpu.a[1] = 0x2000;
    cpu.d[0] = 10;
    thunk_exec_CopyMem(ctx, &cpu);
    check(memcmp(arena + 0x2000, "0123456789", 10) == 0, "CopyMem inside the program's memory copies");
    cpu.a[0] = 0x1000;
    cpu.a[1] = 0x1002;
    cpu.d[0] = 8;
    thunk_exec_CopyMem(ctx, &cpu);
    check(memcmp(arena + 0x1000, "0101234567", 10) == 0, "CopyMem of overlapping spans copies as memmove");
    cpu.a[0] = 0x1000;
    cpu.a[1] = end - 4;
    cpu.d[0] = 10;
    thunk_exec_CopyMem(ctx, &cpu);
    check(m68k_read32(ctx, end - 4) == 0 && logs == 1, "CopyMem past the end of the program's memory: refused, a line in the log");
    cpu.a[0] = end - 4;
    cpu.a[1] = 0x3000;
    cpu.d[0] = 10;
    thunk_exec_CopyMem(ctx, &cpu);
    check(m68k_read32(ctx, 0x3000) == 0 && logs == 2, "CopyMem from past the end: refused");
    cpu.a[0] = 0x1000;
    cpu.a[1] = 0x3000;
    cpu.d[0] = 0;
    thunk_exec_CopyMem(ctx, &cpu);
    check(logs == 2 && m68k_read32(ctx, 0x3000) == 0, "CopyMem of 0 bytes: nothing");
}

static void lists(struct M68KEmuContext *ctx)
{
    struct Cpu cpu = {{0}, {0}};
    const ULONG L = 0x1000, S = 0x4000;

    reset(ctx);
    memset(arena + L, 0xEE, 12);
    cpu.a[0] = L;
    thunk_exec_NewMinList_m(ctx, &cpu);
    check(m68k_read32(ctx, L) == L + 4 && m68k_read32(ctx, L + 4) == 0 && m68k_read32(ctx, L + 8) == L,
          "NewMinList: an empty list with 32-bit links in the program's memory");
    check(arena[L + 12] == 0, "NewMinList writes 12 bytes, no more");
    ctx->shadow_map[0].m68k_addr = S;
    ctx->shadow_map[0].native_ptr = native_list;
    ctx->num_shadows = 1;
    cpu.a[0] = S;
    thunk_exec_NewMinList_m(ctx, &cpu);
    check(native_newminlist == 1 && m68k_read32(ctx, S) == 0, "NewMinList of a system list: the native one");
    cpu.a[0] = 0;
    thunk_exec_NewMinList_m(ctx, &cpu);
    check(native_newminlist == 1, "NewMinList(NULL): nothing");
}

int main(void)
{
    struct M68KEmuContext *ctx = &ctx_storage;

    ports(ctx);
    requests(ctx);
    tasks(ctx);
    copies(ctx);
    lists(ctx);
    printf("exec ports, requests, tasks, copies and lists: %u checks, %u failed\n", checks, failures);
    return failures != 0;
}
