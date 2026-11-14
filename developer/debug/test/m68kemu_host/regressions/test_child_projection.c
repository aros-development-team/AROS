/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A 68k child process of a program: the actual create_proc and child_entry
    of rom/m68kemu, with the Process lookup, the shadow registry, the tag
    walker and the memory accessors they use, extracted by
    run_child_projection.py. The creation of the native process, the moment
    it runs, the program's lock, the two heaps and the run of guest code are
    controlled collaborators that record what is asked of them. The child's
    native entry is called by the test, after create_proc has returned, as
    the scheduler would run it once the parent lets the lock go.
*/
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uintptr_t IPTR;
typedef uint64_t BPTR;
typedef uint64_t Tag;
typedef int BOOL;
typedef void *APTR;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#include "constants.inc"
struct Node { UBYTE ln_Type; };
struct Task { struct Node tc_Node; APTR tc_UserData; };
struct MsgPort { int unused; };
struct Process { struct Task pr_Task; BPTR pr_CLI; struct MsgPort pr_MsgPort; };
struct TagItem { Tag ti_Tag; IPTR ti_Data; };
struct M68KStructLayout { UWORD shadow_type; };
struct M68KEmuContext {
    UBYTE *mem;
    ULONG mem_size, m68k_sysbase, m68k_argptr, m68k_proc, seg_start, entry_point;
    BOOL low_guard, shadow_full_logged, trace_calls, trace_end, faulted;
    APTR emu_task;
    APTR native_procs[8];
    LONG num_children;
    UWORD num_shadows;
    char program_name[32];
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; UBYTE streams_valid; BPTR stream_native[3]; ULONG stream_handle[3]; } shadow_map[M68KEMU_MAX_SHADOWS];
};

/* where the test puts things in the program's memory */
#define ROOT      0x800u    /* the first Process */
#define TAGS      0x1000u   /* the tag list given to CreateNewProc */
#define NAME      0x1800u   /* the child's name */
#define ENTRY     0x3000u   /* the child's 68k entry */
#define EXITCODE  0x3100u   /* its 68k exit code */
#define HEAP      0x4000u   /* the heap begins here */
#define MEMORY    0x40000u
#define BLOCKS    32
#define CHILDREN  6
#define RUNS      8

static unsigned checks, failures;
/* Counted over the whole run: reset() does not put these back, so that a
   check after several cases sees all of them. */
static unsigned logs, low_traps, bugs, unexpected, forbids, permits;
static struct M68KStructLayout task_layout = { 1 }, port_layout = { 4 };
static struct Process root, pool[CHILDREN];
static struct Task *current;        /* the native task that runs now */
static struct Task *lock_owner;     /* the task that holds the program's lock */

/* the program's heap: every block, and whether it is still allocated */
static struct { ULONG addr, size; BOOL live; unsigned frees; } blocks[BLOCKS];
static unsigned heap_allocs, heap_frees, heap_bad_frees, heap_blocks;
static ULONG heap_next, heap_refuse_size;

/* native memory: every AllocVec */
static struct { void *p; BOOL live; } vecs[BLOCKS];
static unsigned vec_allocs, vec_bad_frees, vec_blocks;

/* what CreateNewProc was asked for, one row for each native process made */
static struct {
    IPTR entry, userdata, stack;
    char name[40];
    BOOL terminated;
    struct Task *lock;
} made[CHILDREN];
static unsigned create_calls, creates;
static BOOL native_refuse;

/* every run of guest code */
static struct { ULONG entry, stacktop, d0, d1, a6, found; struct Task *task, *lock; } runs[RUNS];
static unsigned guest_runs;
static ULONG run_result;

static unsigned gil_obtains, gil_releases, lock_violations, ts_drops, holds, releases;

static void m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *why) { (void)ctx; (void)addr; (void)why; ++low_traps; }
static void m68k_log(struct M68KEmuContext *ctx, const char *fmt, ...) { (void)ctx; (void)fmt; ++logs; }
static void bug(const char *fmt, ...) { (void)fmt; ++bugs; }
#include "accessors.inc"
#include "registry.inc"
static const struct M68KStructLayout *shadow_find_layout(const char *name)
{
    if (!strcmp(name, "Task"))
        return &task_layout;
    assert(!strcmp(name, "MsgPort"));
    return &port_layout;
}

static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG n, ULONG flags)
{
    ULONG a = heap_next;

    (void)ctx;
    (void)flags;
    ++heap_allocs;
    if (heap_refuse_size && n == heap_refuse_size)
        return 0;
    n = (n + 15) & ~15u;
    assert(heap_blocks < BLOCKS && a + n <= ctx->mem_size);
    blocks[heap_blocks].addr = a;
    blocks[heap_blocks].size = n;
    blocks[heap_blocks].live = TRUE;
    blocks[heap_blocks].frees = 0;
    ++heap_blocks;
    heap_next += n;
    return a;
}

static void M68KEmu_HeapFree(struct M68KEmuContext *ctx, ULONG a, ULONG n)
{
    unsigned i;

    (void)ctx;
    for (i = 0; i < heap_blocks; i++)
    {
        if (blocks[i].addr == a && blocks[i].live && blocks[i].size == ((n + 15) & ~15u))
        {
            blocks[i].live = FALSE;
            ++blocks[i].frees;
            ++heap_frees;
            return;
        }
    }
    ++heap_bad_frees;
}

static unsigned heap_live(void)
{
    unsigned i, n = 0;

    for (i = 0; i < heap_blocks; i++)
        n += blocks[i].live ? 1 : 0;
    return n;
}

/* the one allocated block of a size, or 0 */
static ULONG live_block(ULONG size)
{
    unsigned i;
    ULONG found = 0;

    for (i = 0; i < heap_blocks; i++)
    {
        if (blocks[i].live && blocks[i].size == size)
        {
            if (found)
                return 0;
            found = blocks[i].addr;
        }
    }
    return found;
}

/* a block that was allocated at this address was freed exactly once and is not allocated now */
static BOOL freed_once(ULONG addr)
{
    unsigned i;

    for (i = 0; i < heap_blocks; i++)
    {
        if (blocks[i].addr == addr)
            return !blocks[i].live && blocks[i].frees == 1;
    }
    return FALSE;
}

static APTR AllocVec(ULONG size, ULONG flags)
{
    void *p = calloc(1, size ? size : 1);

    (void)flags;
    assert(p && vec_blocks < BLOCKS);
    vecs[vec_blocks].p = p;
    vecs[vec_blocks].live = TRUE;
    ++vec_blocks;
    ++vec_allocs;
    return p;
}

static void FreeVec(APTR p)
{
    unsigned i;

    if (!p)
        return;
    for (i = 0; i < vec_blocks; i++)
    {
        if (vecs[i].p == p && vecs[i].live)
        {
            vecs[i].live = FALSE;
            free(p);
            return;
        }
    }
    ++vec_bad_frees;
}

static unsigned vec_live(void)
{
    unsigned i, n = 0;

    for (i = 0; i < vec_blocks; i++)
        n += vecs[i].live ? 1 : 0;
    return n;
}

/* The allocator may give a freed address again: only a row that is still
   allocated answers for an address. */
static BOOL vec_is_live(IPTR p)
{
    unsigned i;

    for (i = 0; i < vec_blocks; i++)
    {
        if ((IPTR)vecs[i].p == p && vecs[i].live)
            return TRUE;
    }
    return FALSE;
}

/* The Task field sync, the stack bounds and the stream fields are not under test here. */
static void shadow_sync(struct M68KEmuContext *ctx, const struct M68KStructLayout *layout, ULONG a, void *native) { (void)ctx; (void)layout; (void)a; (void)native; }
static void m68k_task_stack(struct M68KEmuContext *ctx, ULONG a, struct Task *task) { (void)ctx; (void)a; (void)task; }
static void m68k_process_streams(struct M68KEmuContext *ctx, ULONG a, struct Task *task) { (void)ctx; (void)a; (void)task; }
static struct Task *FindTask(const void *name) { (void)name; return current; }
#include "process.inc"
#include "lifetime.inc"
#include "tags.inc"

/* what the registry holds for a native task, without making anything */
static ULONG guest_of(struct M68KEmuContext *ctx, struct Task *task)
{
    unsigned i;

    for (i = 0; i < ctx->num_shadows; i++)
    {
        if (ctx->shadow_map[i].native_ptr == task && ctx->shadow_map[i].type == task_layout.shadow_type)
            return ctx->shadow_map[i].m68k_addr;
    }
    return 0;
}

/* the program's lock: who has it, and whether it was taken while held */
static void m68k_gil_obtain(struct M68KEmuContext *ctx)
{
    (void)ctx;
    ++gil_obtains;
    if (lock_owner)
        ++lock_violations;
    lock_owner = current;
}

static void m68k_gil_release(struct M68KEmuContext *ctx)
{
    (void)ctx;
    ++gil_releases;
    if (lock_owner != current)
        ++lock_violations;
    lock_owner = NULL;
}

static void m68k_ts_drop(struct M68KEmuContext *ctx) { (void)ctx; ++ts_drops; }
static void m68k_context_hold(struct M68KEmuContext *ctx) { (void)ctx; ++holds; }
static void m68k_context_release(struct M68KEmuContext *ctx) { (void)ctx; ++releases; }
static void Forbid(void) { ++forbids; }
static void Permit(void) { ++permits; }

/* Guest code would run here. It is recorded and not run: who runs it, with
   the lock or not, where, and what the registry holds for that task. */
static ULONG M68KEmu_RunTask(struct M68KEmuContext *ctx, ULONG entry, ULONG stacktop, const ULONG regs[15])
{
    assert(guest_runs < RUNS);
    runs[guest_runs].entry = entry;
    runs[guest_runs].stacktop = stacktop;
    runs[guest_runs].d0 = regs[0];
    runs[guest_runs].d1 = regs[1];
    runs[guest_runs].a6 = regs[14];
    runs[guest_runs].task = current;
    runs[guest_runs].lock = lock_owner;
    runs[guest_runs].found = guest_of(ctx, current);
    ++guest_runs;
    return run_result;
}

/* The native process is made and does not run yet. */
static struct Process *CreateNewProc(const struct TagItem *t)
{
    struct Process *p;
    unsigned i, k;

    ++create_calls;
    if (native_refuse)
        return NULL;
    assert(creates < CHILDREN);
    p = &pool[creates];
    memset(p, 0, sizeof(*p));
    p->pr_Task.tc_Node.ln_Type = NT_PROCESS;
    memset(&made[creates], 0, sizeof(made[0]));
    for (i = 0; i < 20 && t[i].ti_Tag != TAG_DONE; i++)
    {
        if (t[i].ti_Tag == NP_Entry)
            made[creates].entry = t[i].ti_Data;
        if (t[i].ti_Tag == NP_UserData)
            made[creates].userdata = t[i].ti_Data;
        if (t[i].ti_Tag == NP_StackSize)
            made[creates].stack = t[i].ti_Data;
        if (t[i].ti_Tag == NP_Name)
        {
            const char *s = (const char *)t[i].ti_Data;

            for (k = 0; k < sizeof(made[0].name) - 1 && s[k]; k++)
                made[creates].name[k] = s[k];
            made[creates].name[k] = 0;
        }
    }
    made[creates].terminated = i < 20;
    made[creates].lock = lock_owner;
    p->pr_Task.tc_UserData = (APTR)made[creates].userdata;
    ++creates;
    return p;
}

/* Not reached by these cases: streams, a seglist, a native program. */
static BPTR m68k_bptr_in(struct M68KEmuContext *ctx, ULONG v) { (void)ctx; (void)v; ++unexpected; return 0; }
static LONG GetSegListInfo(BPTR seg, struct TagItem *tags) { (void)seg; (void)tags; ++unexpected; return 0; }
static ULONG M68KEmu_LoadHunks(struct M68KEmuContext *ctx, BPTR seg) { (void)ctx; (void)seg; ++unexpected; return 0; }
static void m68k_native_proc_exit(IPTR rc, IPTR data) { (void)rc; (void)data; ++unexpected; }

/* create_proc does not use its CPU argument. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "children.inc"
#pragma GCC diagnostic pop

/* Every failed check is printed: the limit is above the number of checks. */
static void check(int ok, const char *label)
{
    ++checks;
    if (!ok)
    {
        if (failures < 200)
            printf("FAIL: %s\n", label);
        ++failures;
    }
}

static void raw32(struct M68KEmuContext *ctx, ULONG a, ULONG v)
{
    assert(a <= ctx->mem_size - 4);
    ctx->mem[a] = (UBYTE)(v >> 24);
    ctx->mem[a + 1] = (UBYTE)(v >> 16);
    ctx->mem[a + 2] = (UBYTE)(v >> 8);
    ctx->mem[a + 3] = (UBYTE)v;
}

static ULONG tag_at;
static void tags_begin(void) { tag_at = TAGS; }
static void tag(struct M68KEmuContext *ctx, ULONG t, ULONG d)
{
    raw32(ctx, tag_at, t);
    raw32(ctx, tag_at + 4, d);
    tag_at += 8;
}

/* The first Process is registered with its port, as after its first lookup;
   the parent runs and holds the lock. */
static void reset(struct M68KEmuContext *ctx)
{
    unsigned i;

    for (i = 0; i < vec_blocks; i++)
    {
        if (vecs[i].live)
            free(vecs[i].p);
    }
    memset(vecs, 0, sizeof(vecs));
    memset(blocks, 0, sizeof(blocks));
    memset(made, 0, sizeof(made));
    memset(runs, 0, sizeof(runs));
    memset(pool, 0, sizeof(pool));
    memset(&root, 0, sizeof(root));
    memset(ctx->mem, 0, ctx->mem_size);
    memset(ctx->shadow_map, 0, sizeof(ctx->shadow_map));
    memset(ctx->native_procs, 0, sizeof(ctx->native_procs));
    ctx->num_shadows = 0;
    ctx->shadow_full_logged = FALSE;
    ctx->m68k_sysbase = 0x400;
    ctx->m68k_argptr = 0;
    ctx->seg_start = 0;
    ctx->entry_point = 0;
    ctx->low_guard = TRUE;
    ctx->trace_calls = ctx->trace_end = ctx->faulted = FALSE;
    ctx->num_children = 0;
    ctx->program_name[0] = 0;
    root.pr_Task.tc_Node.ln_Type = NT_PROCESS;
    current = &root.pr_Task;
    lock_owner = current;
    ctx->emu_task = current;
    ctx->m68k_proc = ROOT;
    heap_allocs = heap_frees = heap_bad_frees = heap_blocks = 0;
    heap_next = HEAP;
    heap_refuse_size = 0;
    vec_allocs = vec_bad_frees = vec_blocks = 0;
    create_calls = creates = 0;
    native_refuse = FALSE;
    guest_runs = 0;
    run_result = 7;
    gil_obtains = gil_releases = lock_violations = ts_drops = holds = releases = 0;
    if (shadow_register(ctx, ROOT, &root.pr_Task, task_layout.shadow_type) != ROOT)
        abort();
    if (m68k_process_of(ctx, &root.pr_Task) != ROOT)
        abort();
    memcpy(ctx->mem + NAME, "child one", 10);
}

/* The tag list of a child with this stack request; an exit code when given. */
static void child_tags(struct M68KEmuContext *ctx, ULONG entry, ULONG stack, ULONG exitcode, ULONG exitdata)
{
    tags_begin();
    if (entry)
        tag(ctx, (ULONG)NP_Entry, entry);
    tag(ctx, (ULONG)NP_StackSize, stack);
    tag(ctx, (ULONG)NP_Name, NAME);
    if (exitcode)
    {
        tag(ctx, (ULONG)NP_ExitCode, exitcode);
        tag(ctx, (ULONG)NP_ExitData, exitdata);
    }
    tag(ctx, (ULONG)TAG_DONE, 0);
}

static ULONG create(struct M68KEmuContext *ctx)
{
    return (ULONG)create_proc(ctx, NULL, TAGS, 0, 0, 0, 0);
}

/* The scheduler runs a native process that CreateNewProc made: the parent
   has let the lock go. Afterwards the parent runs again, with the lock. */
static LONG run_child(unsigned i)
{
    LONG rc;

    lock_owner = NULL;
    current = &pool[i].pr_Task;
    rc = ((LONG (*)(void))made[i].entry)();
    current = &root.pr_Task;
    return rc;
}

static void parent_runs(void)
{
    current = &root.pr_Task;
    lock_owner = current;
}

/* the registry entries and the memory of the processes that must stay as they are */
struct snapshot {
    UWORD entries;
    UBYTE map[8 * sizeof(((struct M68KEmuContext *)0)->shadow_map[0])];
    UBYTE first[M68K_SIZEOF_PROCESS], other[M68K_SIZEOF_PROCESS];
    ULONG other_at;
};

static void take(struct M68KEmuContext *ctx, struct snapshot *s, ULONG other_at)
{
    assert(ctx->num_shadows <= 8);
    s->entries = ctx->num_shadows;
    memcpy(s->map, ctx->shadow_map, s->entries * sizeof(ctx->shadow_map[0]));
    memcpy(s->first, ctx->mem + ROOT, M68K_SIZEOF_PROCESS);
    s->other_at = other_at;
    if (other_at)
        memcpy(s->other, ctx->mem + other_at, M68K_SIZEOF_PROCESS);
}

static BOOL same(struct M68KEmuContext *ctx, const struct snapshot *s)
{
    return ctx->num_shadows >= s->entries &&
        !memcmp(s->map, ctx->shadow_map, s->entries * sizeof(ctx->shadow_map[0])) &&
        !memcmp(s->first, ctx->mem + ROOT, M68K_SIZEOF_PROCESS) &&
        (!s->other_at || !memcmp(s->other, ctx->mem + s->other_at, M68K_SIZEOF_PROCESS));
}

static BOOL ran(ULONG entry)
{
    unsigned i;

    for (i = 0; i < guest_runs; i++)
    {
        if (runs[i].entry == entry)
            return TRUE;
    }
    return FALSE;
}

static void test_creation(struct M68KEmuContext *ctx)
{
    unsigned before = failures;
    ULONG r, stack;
    LONG rc;

    /* a child with an exit code, from its creation to its end */
    reset(ctx);
    child_tags(ctx, ENTRY, 65536, EXITCODE, 0x1234);
    r = create(ctx);
    stack = live_block(65536);
    check(r && r != ROOT && shadow_lookup(ctx, r) == &pool[0].pr_Task && shadow_lookup(ctx, r + M68K_PR_MSGPORT) == &pool[0].pr_MsgPort,
          "CreateNewProc returns the child's own guest Process, registered with its port");
    check(create_calls == 1 && creates == 1 && made[0].entry == (IPTR)child_entry && made[0].userdata && made[0].stack == 65536 && made[0].terminated,
          "one native process is started, with the module's child entry, the child's record and a 65536-byte native stack");
    check(!strcmp(made[0].name, "child one") && vec_allocs == 2 && vec_live() == 1 && vec_is_live(made[0].userdata),
          "its name is a copy of the program's string, freed after the call; its record stays");
    check(ctx->num_children == 1 && holds == 1 && releases == 0 && stack && heap_live() == 2,
          "the child is counted and holds the context before it runs; its 65536-byte guest stack is allocated");
    check(guest_runs == 0 && gil_obtains == 0 && gil_releases == 0 && lock_owner == &root.pr_Task && made[0].lock == &root.pr_Task,
          "nothing of the child runs inside the call: the parent keeps the lock throughout");
    rc = run_child(0);
    check(guest_runs == 2 && runs[0].entry == ENTRY && runs[0].task == &pool[0].pr_Task && runs[0].lock == &pool[0].pr_Task &&
          stack && runs[0].stacktop == stack + 65536 && runs[0].a6 == ctx->m68k_sysbase,
          "the child runs its entry once, as itself, holding the lock, on its own stack, with ExecBase in A6");
    check(r && runs[0].found == r, "the child finds the guest Process that its parent was given");
    check(runs[1].entry == EXITCODE && runs[1].d0 == 7 && runs[1].d1 == 0x1234 && stack && runs[1].stacktop == stack + 65536,
          "its exit code runs afterwards with the entry's result and the exit data");
    check(rc == 7, "the native process ends with the entry's result");
    check(stack && freed_once(stack) && heap_frees == 1 && heap_bad_frees == 0, "its guest stack is freed once");
    check(ctx->num_children == 0 && holds == 1 && releases == 1, "its count and its hold are given back once");
    check(vec_live() == 0 && vec_bad_frees == 0, "its record is freed once");
    check(ts_drops == 1 && gil_obtains == 1 && gil_releases == 1 && !lock_owner && !lock_violations,
          "it took the lock once, dropped its task state and let the lock go");
    check(r && shadow_lookup(ctx, r) == &pool[0].pr_Task && heap_live() == 1, "its guest Process stays where the program can find it");
    /* a program that faulted */
    reset(ctx);
    child_tags(ctx, ENTRY, 16384, EXITCODE, 0x1234);
    r = create(ctx);
    ctx->faulted = TRUE;
    (void)run_child(0);
    check(r && guest_runs == 1 && runs[0].entry == ENTRY && !ran(EXITCODE), "after a fault of the program no exit code is run");
    /* the guest stack */
    reset(ctx);
    child_tags(ctx, ENTRY, 100, 0, 0);
    r = create(ctx);
    check(r && live_block(16384), "a stack request below the minimum gets 16384 bytes");
    reset(ctx);
    child_tags(ctx, ENTRY, 65537, 0, 0);
    r = create(ctx);
    check(r && live_block(65552), "a larger stack request is rounded up to 16 bytes");
    check(!unexpected && !bugs && !low_traps && !forbids && !permits && !logs,
          "no stream, seglist, native program, trace, low-memory or refusal path is reached by a creation");
    printf("GROUP creation: %u failed\n", failures - before);
}

static void test_refusal(struct M68KEmuContext *ctx)
{
    unsigned before = failures;
    struct snapshot s;
    ULONG r;

    reset(ctx);
    child_tags(ctx, 0, 16384, 0, 0);
    r = create(ctx);
    check(r == 0 && create_calls == 0 && vec_live() == 0 && heap_live() == 0 && ctx->num_children == 0 && holds == 0,
          "no entry: refused before any native process, with nothing left");
    reset(ctx);
    child_tags(ctx, MEMORY - 1, 16384, 0, 0);
    r = create(ctx);
    check(r == 0 && create_calls == 0 && vec_live() == 0 && heap_live() == 0 && ctx->num_children == 0 && holds == 0,
          "an entry that does not lie in the program's memory: refused before any native process");
    reset(ctx);
    child_tags(ctx, ENTRY, 16384, 0, 0);
    heap_refuse_size = 16384;
    r = create(ctx);
    check(r == 0 && create_calls == 0 && vec_live() == 0 && heap_live() == 0 && ctx->num_children == 0 && holds == 0,
          "no memory for the guest stack: refused before any native process");
    /* the native process cannot be made */
    reset(ctx);
    take(ctx, &s, 0);
    child_tags(ctx, ENTRY, 16384, EXITCODE, 0x1234);
    native_refuse = TRUE;
    r = create(ctx);
    check(r == 0 && create_calls == 1 && creates == 0, "the native creation is refused: CreateNewProc returns 0");
    check(ctx->num_children == 0 && holds == 1 && releases == 1, "the count and the hold are taken back");
    check(heap_live() == 0 && heap_frees == 1 && heap_bad_frees == 0 && vec_live() == 0 && vec_bad_frees == 0,
          "the guest stack and the record are freed once");
    check(ctx->num_shadows == s.entries && guest_runs == 0 && gil_obtains == 0, "nothing is registered and nothing runs");
    check(same(ctx, &s), "the parent's entries and memory are as before");
    check(!unexpected && !bugs && !low_traps && !forbids && !permits && !logs,
          "no stream, seglist, native program, trace, low-memory or registry path is reached by a refusal");
    printf("GROUP refusal: %u failed\n", failures - before);
}

/* No guest Process can be made for a child whose native process exists. */
#define CASE_LABELS(c) { \
    c ": a first child is made and has ended", \
    c ": CreateNewProc returns 0", \
    c ": the native process was started and waits for the lock", \
    c ": the parent frees nothing that the child owns: its record, its stack, its count and its hold stay", \
    c ": no guest Process and no registry entry is left for it", \
    c ": nothing ran inside the call and the parent kept the lock", \
    c ": the cancelled child runs no entry of the program", \
    c ": and no exit code", \
    c ": it looks for no guest Process and makes none, although there is room again", \
    c ": its native process ends with 0", \
    c ": it frees its guest stack once", \
    c ": it gives back its count and its hold once", \
    c ": it frees its record once", \
    c ": it took the lock once, dropped its task state and let the lock go", \
    c ": the parent's and the earlier child's entries and memory are as before", \
    c ": a later CreateNewProc gives a child again", \
    c ": and that child runs its entry as itself" }
static const char *const projection_labels[3][17] = {
    CASE_LABELS("no memory for the guest Process"),
    CASE_LABELS("full registry"),
    CASE_LABELS("one registry slot left"),
};

static void projection_case(struct M68KEmuContext *ctx, unsigned cause)
{
    const char *const *label = projection_labels[cause];
    struct snapshot s;
    ULONG first, r, later, stack, allocs;
    UWORD real;
    unsigned runs0, obtains0, drops0, holds0, releases0, live0, creates0, n;
    LONG rc;

    /* an earlier child that has ended: its guest Process is something else to leave alone */
    reset(ctx);
    child_tags(ctx, ENTRY, 16384, 0, 0);
    first = create(ctx);
    (void)run_child(0);
    parent_runs();
    check(first && guest_runs == 1 && ctx->num_children == 0 && vec_live() == 0 && heap_live() == 1, label[0]);
    take(ctx, &s, first);
    real = ctx->num_shadows;
    runs0 = guest_runs;
    obtains0 = gil_obtains;
    drops0 = ts_drops;
    holds0 = holds;
    releases0 = releases;
    live0 = heap_live();
    creates0 = creates;
    /* what stands in the way */
    if (cause == 0)
        heap_refuse_size = M68K_SIZEOF_PROCESS;
    if (cause == 1)
        ctx->num_shadows = M68KEMU_MAX_SHADOWS;
    if (cause == 2)
        ctx->num_shadows = M68KEMU_MAX_SHADOWS - 1;
    child_tags(ctx, ENTRY, 32768, EXITCODE, 0x55);
    r = create(ctx);
    stack = live_block(32768);
    check(r == 0, label[1]);
    check(creates == creates0 + 1 && made[creates0].entry == (IPTR)child_entry && made[creates0].userdata, label[2]);
    check(ctx->num_children == 1 && holds == holds0 + 1 && releases == releases0 && vec_live() == 1 &&
          vec_is_live(made[creates0].userdata) && stack && heap_live() == live0 + 1, label[3]);
    check(ctx->num_shadows == (cause == 0 ? real : cause == 1 ? M68KEMU_MAX_SHADOWS : M68KEMU_MAX_SHADOWS - 1) &&
          !guest_of(ctx, &pool[creates0].pr_Task), label[4]);
    check(guest_runs == runs0 && gil_obtains == obtains0 && lock_owner == &root.pr_Task, label[5]);
    /* there is room again before the child gets the lock */
    heap_refuse_size = 0;
    ctx->num_shadows = real;
    allocs = heap_allocs;
    rc = run_child(creates0);
    parent_runs();
    check(guest_runs == runs0, label[6]);
    check(!ran(EXITCODE), label[7]);
    check(heap_allocs == allocs && ctx->num_shadows == real && !guest_of(ctx, &pool[creates0].pr_Task), label[8]);
    check(rc == 0, label[9]);
    check(stack && freed_once(stack) && heap_bad_frees == 0, label[10]);
    check(ctx->num_children == 0 && holds == holds0 + 1 && releases == releases0 + 1, label[11]);
    check(vec_live() == 0 && vec_bad_frees == 0, label[12]);
    check(gil_obtains == obtains0 + 1 && ts_drops == drops0 + 1 && !lock_violations, label[13]);
    check(same(ctx, &s), label[14]);
    /* the program can still make a child */
    child_tags(ctx, ENTRY, 16384, 0, 0);
    n = creates;
    later = create(ctx);
    check(later && creates == n + 1 && shadow_lookup(ctx, later) == &pool[n].pr_Task, label[15]);
    runs0 = guest_runs;
    if (creates == n + 1)
        (void)run_child(n);
    check(later && guest_runs == runs0 + 1 && runs[runs0].entry == ENTRY && runs[runs0].task == &pool[n].pr_Task &&
          runs[runs0].found == later, label[16]);
}

static void test_projection(struct M68KEmuContext *ctx)
{
    unsigned before = failures, logs0, logs1, logs2;

    /* the registry's report of a full table, counted for each case by itself */
    logs0 = logs;
    projection_case(ctx, 0);
    logs1 = logs;
    projection_case(ctx, 1);
    logs2 = logs;
    projection_case(ctx, 2);
    check(logs1 - logs0 == 0 && logs2 - logs1 == 1 && logs - logs2 == 1,
          "the registry reports that it is full once in each of its two cases, and not at all when memory is what is missing");
    /* these counters are never put back: all three cases, and the groups before them, are in them */
    check(!unexpected && !bugs && !low_traps && !forbids && !permits,
          "no stream, seglist, native program, trace or low-memory path is reached by a cancelled child");
    printf("GROUP projection: %u failed\n", failures - before);
}

int main(void)
{
    static struct M68KEmuContext ctx;
    unsigned i;

    ctx.mem_size = MEMORY;
    ctx.mem = malloc(ctx.mem_size);
    assert(ctx.mem);
    test_creation(&ctx);
    test_refusal(&ctx);
    test_projection(&ctx);
    for (i = 0; i < vec_blocks; i++)
    {
        if (vecs[i].live)
            free(vecs[i].p);
    }
    free(ctx.mem);
    printf("child projection: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
