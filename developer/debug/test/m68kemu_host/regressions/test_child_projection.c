/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A 68k child process of a program: the actual create_proc and child_entry
    of rom/m68kemu, with the Process lookup, the shadow registry, the tag
    walker, the copy of a seglist into the program's memory with its release
    and the memory accessors they use, extracted by run_child_projection.py.
    The creation of the native process, the moment it runs, the program's
    lock, the two heaps, a native seglist with the relocation list dos keeps
    for it and the run of guest code are controlled collaborators that
    record what is asked of them. The child's native entry is called by the
    test, after create_proc has returned, as the scheduler would run it once
    the parent lets the lock go. That order is modelled cooperatively, one
    task at a time: this is not a test of real SMP or of the native runtime.
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
/* a BPTR is a plain pointer on every target but m68k (AROS_FAST_BPTR) */
#define BADDR(x) ((APTR)(IPTR)(x))
#include "constants.inc"
/* struct HunkRelocGroup, the HUNKRELOC_* kinds, struct HunkRelocTable */
#include "relocs.inc"
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
static struct { ULONG addr, size; BOOL live; unsigned frees, order; struct Task *holder; } blocks[BLOCKS];
static unsigned heap_allocs, heap_frees, heap_bad_frees, heap_blocks, free_order;
static int watch_row = -1;          /* the first of two rows whose liveness each run of guest code records */
static ULONG heap_next, heap_refuse_size;

/* native memory: every AllocVec */
static struct { void *p; BOOL live; } vecs[BLOCKS];
static unsigned vec_allocs, vec_bad_frees, vec_blocks;

/* what CreateNewProc was asked for, one row for each native process made */
static struct {
    IPTR entry, userdata, stack, seglist, exitcode, exitdata, freeseglist, freeseglist_values[4];
    BOOL freeseglist_given;
    unsigned freeseglist_count;
    char name[40];
    BOOL terminated;
    struct Task *lock;
} made[CHILDREN];
static unsigned create_calls, creates;
static BOOL native_refuse;

/* every run of guest code */
static struct { ULONG entry, stacktop, d0, d1, a6, found; struct Task *task, *lock; BOOL watched_live; } runs[RUNS];
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
    ++heap_allocs;
    if (heap_refuse_size && n == heap_refuse_size)
        return 0;
    n = (n + 15) & ~15u;
    assert(heap_blocks < BLOCKS && a + n <= ctx->mem_size);
    /* as the real heap does */
    if (flags & MEMF_CLEAR)
        memset(ctx->mem + a, 0, n);
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
            blocks[i].order = ++free_order;
            blocks[i].holder = lock_owner;
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
    runs[guest_runs].watched_live = watch_row >= 0 && blocks[watch_row].live && blocks[watch_row + 1].live;
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
        if (t[i].ti_Tag == NP_Seglist)
            made[creates].seglist = t[i].ti_Data;
        if (t[i].ti_Tag == NP_ExitCode)
            made[creates].exitcode = t[i].ti_Data;
        if (t[i].ti_Tag == NP_ExitData)
            made[creates].exitdata = t[i].ti_Data;
        if (t[i].ti_Tag == NP_FreeSeglist)
        {
            /* the first is the one dos goes by (FindTagItem); every one is kept, in its order */
            if (!made[creates].freeseglist_given)
                made[creates].freeseglist = t[i].ti_Data;
            made[creates].freeseglist_given = TRUE;
            if (made[creates].freeseglist_count < 4)
                made[creates].freeseglist_values[made[creates].freeseglist_count] = t[i].ti_Data;
            ++made[creates].freeseglist_count;
        }
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

/* A seglist that dos loaded, as LoadSeg() leaves it: one allocation for
   each segment that begins with a ULONG, the size of the allocation, then
   the BPTR of the next segment, then the data; the BPTR of a segment is the
   address of that link. dos puts the link four bytes after an eight-byte
   boundary; here it lies on one, so that reading it through a BPTR pointer,
   as the copy does, is an aligned read for the sanitizer. Nothing else of
   the layout differs. Each segment is an allocation of its own, written
   with memcpy() and read with the types the copy reads it with. Segment 0
   holds a longword that dos relocated as an absolute reference to segment 1. */
#define SEGS 2
#define SEG_MAX 0x30u
static const ULONG seg_bytes[SEGS] = { 0x20u, 0x30u };
static UBYTE *seg_alloc[SEGS];
static UBYTE *seg_link[SEGS];
static UBYTE seg_before[SEGS][SEG_MAX];     /* the native data before the call */
#define RELOC_AT       4u                   /* the relocated longword of segment 0 */
#define RELOC_ORIGINAL 8u                   /* what the file had there: offset 8 into segment 1 */
static ULONG reloc_offsets[1] = { RELOC_AT };
static struct HunkRelocGroup reloc_group;
static struct HunkRelocTable reloc_table;
/* what dos knows about the seglist: whether it is 68k code, and whether it
   answers the relocation list */
static struct { BPTR seg; BOOL hunk, relocs; } seg_record;
static ULONG seg_handle;                    /* how the program names the seglist */
static unsigned info_calls, bptr_ins, copies_outside;

static UBYTE *seg_data(unsigned i) { return seg_link[i] + sizeof(BPTR); }

static void host32(UBYTE *p, ULONG v)
{
    p[0] = (UBYTE)(v >> 24);
    p[1] = (UBYTE)(v >> 16);
    p[2] = (UBYTE)(v >> 8);
    p[3] = (UBYTE)v;
}

/* The program's handle of the seglist is the one native BPTR it stands for;
   no other value is translated here, and no stream either. */
static BPTR m68k_bptr_in(struct M68KEmuContext *ctx, ULONG v)
{
    (void)ctx;
    if (seg_handle && v == seg_handle)
    {
        ++bptr_ins;
        return seg_record.seg;
    }
    ++unexpected;
    return 0;
}

/* The double of dos.library's GetSegListInfo(): the number of tags it
   answered, 0 for a seglist it has no node for. */
static LONG GetSegListInfo(BPTR seg, struct TagItem *tags)
{
    LONG count = 0;
    unsigned i;

    ++info_calls;
    if (!seg || seg != seg_record.seg || !tags)
    {
        ++unexpected;
        return 0;
    }
    for (i = 0; i < 8 && tags[i].ti_Tag != TAG_DONE; i++)
    {
        if (tags[i].ti_Tag == GSLI_68KHUNK && tags[i].ti_Data && seg_record.hunk)
        {
            *(IPTR *)(IPTR)tags[i].ti_Data = 1;
            ++count;
        }
        if (tags[i].ti_Tag == GSLI_68KRELOCS && tags[i].ti_Data && seg_record.relocs)
        {
            *(struct HunkRelocTable **)(IPTR)tags[i].ti_Data = &reloc_table;
            ++count;
        }
    }
    return count;
}

/* exec's CopyMem(source, dest, size). A copy that does not lie in the
   program's memory is counted and not carried out. */
static struct M68KEmuContext *active;
static void CopyMem(const void *src, void *dst, ULONG n)
{
    const UBYTE *p = (const UBYTE *)dst;

    if (!p || p < active->mem || n > active->mem_size || (ULONG)(p - active->mem) > active->mem_size - n)
    {
        ++copies_outside;
        return;
    }
    memcpy(dst, src, n);
}

#include "seglist.inc"

/* Not reached by these cases: a native program that ends. */
static void m68k_native_proc_exit(IPTR rc, IPTR data) { (void)rc; (void)data; ++unexpected; }

/* the data registers of the CPU that calls a dos entry: the test's own array */
static ULONG M68KEmu_GetD(void *cpu, int n) { return ((const ULONG *)cpu)[n]; }

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
    return (ULONG)create_proc(ctx, NULL, TAGS, 0, 0, 0, 0, TRUE);
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

/* ---- a child whose code is a seglist that dos loaded ---- */

/* The seglist of a case, what dos knows about it, the program's handle for
   it, and its native data as it is before the call. */
static void seglist_make(struct M68KEmuContext *ctx, BOOL hunk, BOOL relocs)
{
    unsigned i, k;
    BPTR next;

    for (i = 0; i < SEGS; i++)
    {
        UBYTE *base;
        ULONG total = seg_bytes[i] + (ULONG)(sizeof(ULONG) + sizeof(BPTR));

        if (!seg_alloc[i])
            seg_alloc[i] = calloc(1, SEG_MAX + 16);
        assert(seg_alloc[i]);
        base = seg_alloc[i];
        memset(base, 0, SEG_MAX + 16);

        seg_link[i] = base + 8;
        memcpy(base + 4, &total, sizeof(total));
        for (k = 0; k < seg_bytes[i]; k++)
            seg_data(i)[k] = (UBYTE)(0x40 + 0x40 * i + k);
    }
    next = (BPTR)(IPTR)seg_link[1];
    memcpy(seg_link[0], &next, sizeof(next));
    /* dos added the low 32 bits of segment 1's host address */
    host32(seg_data(0) + RELOC_AT, (ULONG)(IPTR)seg_data(1) + RELOC_ORIGINAL);
    reloc_group.hrg_Hunk = 0;
    reloc_group.hrg_Target = 1;
    reloc_group.hrg_Kind = HUNKRELOC_ABS;
    reloc_group.hrg_Count = 1;
    reloc_group.hrg_Offsets = reloc_offsets;
    reloc_table.hrt_Groups = 1;
    reloc_table.hrt_Group = &reloc_group;
    seg_record.seg = (BPTR)(IPTR)seg_link[0];
    seg_record.hunk = hunk;
    seg_record.relocs = relocs;
    seg_handle = (M68KEMU_HANDLE_BASE >> 2) + 1;
    for (i = 0; i < SEGS; i++)
        memcpy(seg_before[i], seg_data(i), seg_bytes[i]);
    info_calls = bptr_ins = 0;
    /* the parent's own segment fields, which a copy must leave as they are */
    ctx->entry_point = 0x00011110;
    ctx->seg_start = 0x00022220;
}

/* The tag list of a child whose code is that seglist, with this stack
   request; an exit code when given. */
static void seglist_tags(struct M68KEmuContext *ctx, ULONG stack, ULONG exitcode, ULONG exitdata)
{
    tags_begin();
    tag(ctx, (ULONG)NP_Seglist, seg_handle);
    tag(ctx, (ULONG)NP_StackSize, stack);
    tag(ctx, (ULONG)NP_Name, NAME);
    if (exitcode)
    {
        tag(ctx, (ULONG)NP_ExitCode, exitcode);
        tag(ctx, (ULONG)NP_ExitData, exitdata);
    }
    tag(ctx, (ULONG)TAG_DONE, 0);
}

/* the native seglist and the parent's segment fields are as they were */
static BOOL parent_segments_kept(struct M68KEmuContext *ctx)
{
    unsigned i;

    for (i = 0; i < SEGS; i++)
    {
        if (memcmp(seg_before[i], seg_data(i), seg_bytes[i]))
            return FALSE;
    }
    return ctx->entry_point == 0x00011110 && ctx->seg_start == 0x00022220;
}

/* The two blocks the copy took, the first and the second allocation from
   row `first` on: chained as the segments were, each with its size word,
   the data the segment's, and the relocated longword pointing into the
   copy of segment 1, as dos/doshunks.h defines HUNKRELOC_ABS for the
   addresses in the program's memory. What a block held stays readable
   after it was given back: the fixture's heap gives no address twice. */
static BOOL copy_made(struct M68KEmuContext *ctx, unsigned first)
{
    ULONG d0, d1, k;

    if (heap_blocks < first + 2 || blocks[first].size != ((seg_bytes[0] + 8 + 15) & ~15u) ||
        blocks[first + 1].size != ((seg_bytes[1] + 8 + 15) & ~15u))
        return FALSE;
    d0 = blocks[first].addr + 8;
    d1 = blocks[first + 1].addr + 8;
    if (m68k_read32(ctx, d0 - 8) != seg_bytes[0] + 8 || m68k_read32(ctx, d0 - 4) != (d1 - 4) >> 2 ||
        m68k_read32(ctx, d1 - 8) != seg_bytes[1] + 8 || m68k_read32(ctx, d1 - 4) != 0)
        return FALSE;
    for (k = 0; k < seg_bytes[0]; k++)
    {
        if ((k < RELOC_AT || k >= RELOC_AT + 4) && ctx->mem[d0 + k] != seg_before[0][k])
            return FALSE;
    }
    for (k = 0; k < seg_bytes[1]; k++)
    {
        if (ctx->mem[d1 + k] != seg_before[1][k])
            return FALSE;
    }
    return m68k_read32(ctx, d0 + RELOC_AT) == d1 + RELOC_ORIGINAL;
}

/* every block of the fixture's heap was freed at most once */
static BOOL no_double_free(void)
{
    unsigned i;

    for (i = 0; i < heap_blocks; i++)
    {
        if (blocks[i].frees > 1)
            return FALSE;
    }
    return heap_bad_frees == 0;
}

static void test_seglist(struct M68KEmuContext *ctx)
{
    unsigned before = failures, first, bugs0, forbids0, permits0;
    ULONG r, stack;

    /* a child of the copy, from its creation to its end */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    seglist_tags(ctx, 16384, 0, 0);
    first = heap_blocks;
    r = create(ctx);
    stack = live_block(16384);
    check(copy_made(ctx, first) && blocks[first].live && blocks[first + 1].live,
          "seglist: a 68k seglist is copied into the program's memory, one block for each segment, chained and relocated for it");
    check(info_calls == 2 && bptr_ins == 1, "seglist: dos is asked once whether it is 68k code and once for its relocations; the handle is translated once");
    check(r && shadow_lookup(ctx, r) == &pool[0].pr_Task && made[0].entry == (IPTR)child_entry && made[0].userdata && stack &&
          ctx->num_children == 1 && holds == 1 && guest_runs == 0,
          "seglist: CreateNewProc returns the child's guest Process; the child is started, counted, holds the context and has its stack");
    check(parent_segments_kept(ctx), "seglist: the native seglist and the parent's segment fields are as before");
    (void)run_child(0);
    parent_runs();
    check(guest_runs == 1 && runs[0].entry == blocks[first].addr + 8 && runs[0].task == &pool[0].pr_Task &&
          runs[0].lock == &pool[0].pr_Task && runs[0].found == r && stack && runs[0].stacktop == stack + 16384,
          "seglist: the child runs the copy's first segment as itself, holding the lock, on its own stack");
    check(stack && freed_once(stack) && ctx->num_children == 0 && releases == 1 && vec_live() == 0 && no_double_free() &&
          gil_obtains == 1 && gil_releases == 1 && !lock_violations,
          "seglist: the child gives back its stack, its count, its hold and its record once, and took the lock once without a clash");
    /* the copy cannot be made: dos answers no relocation list */
    reset(ctx);
    seglist_make(ctx, TRUE, FALSE);
    seglist_tags(ctx, 16384, 0, 0);
    bugs0 = bugs;
    r = create(ctx);
    check(r == 0 && create_calls == 0 && heap_allocs == 0 && heap_live() == 0 && vec_live() == 0 && ctx->num_children == 0 &&
          holds == 0 && bugs == bugs0 + 1 && parent_segments_kept(ctx),
          "seglist: a seglist that cannot be copied: refused before any native process or allocation, with nothing left");
    /* no memory for the guest stack once the copy is made */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    seglist_tags(ctx, 16384, 0, 0);
    heap_refuse_size = 16384;
    first = heap_blocks;
    r = create(ctx);
    check(r == 0 && create_calls == 0 && copy_made(ctx, first) && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr) &&
          heap_live() == 0 && vec_live() == 0 && ctx->num_children == 0 && holds == 0 && no_double_free(),
          "seglist: no memory for the guest stack: refused before any native process; the copy is given back once");
    /* the native process cannot be made once the copy and the stack are */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    seglist_tags(ctx, 16384, EXITCODE, 0x1234);
    native_refuse = TRUE;
    first = heap_blocks;
    r = create(ctx);
    check(r == 0 && create_calls == 1 && creates == 0 && ctx->num_children == 0 && holds == 1 && releases == 1,
          "seglist: the native creation is refused: CreateNewProc returns 0, the count and the hold are taken back");
    check(copy_made(ctx, first) && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr) &&
          freed_once(blocks[first + 2].addr) && blocks[first + 2].size == 16384 && heap_live() == 0 && vec_live() == 0 &&
          no_double_free() && guest_runs == 0 && parent_segments_kept(ctx),
          "seglist: the copy, the guest stack and the record are given back once each, and nothing runs");
    /* a native seglist: started natively, no copy, no guest stack, no 68k child */
    reset(ctx);
    seglist_make(ctx, FALSE, FALSE);
    seglist_tags(ctx, 16384, 0, 0);
    forbids0 = forbids;
    permits0 = permits;
    r = create(ctx);
    check(r && shadow_lookup(ctx, r) == &pool[0].pr_Task && made[0].entry == 0 && made[0].seglist == (IPTR)seg_record.seg &&
          made[0].stack == 65536 && made[0].exitcode == (IPTR)m68k_native_proc_exit && made[0].exitdata == (IPTR)ctx &&
          ctx->native_procs[0] == &pool[0],
          "seglist: a native seglist is started as a native program with its own exit hook, and the program gets its guest Process");
    check(info_calls == 1 && heap_allocs == 1 && heap_live() == 1 && vec_live() == 0 && ctx->num_children == 0 && holds == 1 &&
          releases == 0 && forbids == forbids0 + 1 && permits == permits0 + 1 && guest_runs == 0,
          "seglist: nothing is copied for it and no guest stack or 68k child is made; it holds the context");
    check(!unexpected && !low_traps && !copies_outside,
          "seglist: no stream, trace, low-memory or foreign seglist path is reached, and no copy leaves the program's memory");
    printf("GROUP seglist: %u failed\n", failures - before);
}

/* A cancelled child whose code was a copy of a seglist. The copy belongs to
   the cancelled child, which is going to run none of it: it has to be
   given back, once, and everything else the child owns has to come back
   when the child runs. */
#define COPY_LABELS(c) { \
    c ": a first child is made and has ended", \
    c ": CreateNewProc returns 0", \
    c ": the seglist was copied: one block for each segment, chained and relocated", \
    c ": the native process was started and waits for the lock", \
    c ": the copy is given back inside the call, each block exactly once; only the child's stack is left of the call", \
    c ": the parent frees nothing else that the child owns: its record, its stack, its count and its hold stay", \
    c ": no guest Process and no registry entry is left for it", \
    c ": nothing ran inside the call and the parent kept the lock", \
    c ": the native seglist and the parent's segment fields are as before", \
    c ": the cancelled child runs neither the copy nor an exit code", \
    c ": its native process ends with 0", \
    c ": it frees its guest stack once, and no block is freed twice", \
    c ": it gives back its count and its hold once and frees its record once", \
    c ": it took the lock once, dropped its task state and let the lock go", \
    c ": the program's heap is as it was before the call: nothing of the copy or of the child is left", \
    c ": the parent's and the earlier child's entries and memory are as before" }
static const char *const copy_labels[2][16] = {
    COPY_LABELS("copy, no memory for the guest Process"),
    COPY_LABELS("copy, full registry"),
};

static void cancelled_copy_case(struct M68KEmuContext *ctx, unsigned cause)
{
    const char *const *label = copy_labels[cause];
    struct snapshot s;
    ULONG first_child, r, stack, copy0, copy1;
    UWORD real;
    unsigned runs0, obtains0, drops0, holds0, releases0, live0, creates0, first;
    LONG rc;

    /* an earlier child that has ended: its guest Process is something else to leave alone */
    reset(ctx);
    child_tags(ctx, ENTRY, 16384, 0, 0);
    first_child = create(ctx);
    (void)run_child(0);
    parent_runs();
    check(first_child && guest_runs == 1 && ctx->num_children == 0 && vec_live() == 0 && heap_live() == 1, label[0]);
    take(ctx, &s, first_child);
    real = ctx->num_shadows;
    runs0 = guest_runs;
    obtains0 = gil_obtains;
    drops0 = ts_drops;
    holds0 = holds;
    releases0 = releases;
    live0 = heap_live();
    creates0 = creates;
    seglist_make(ctx, TRUE, TRUE);
    /* the blocks of the copy must not look like the guest Process to the refusal */
    assert(seg_bytes[0] + 8 != M68K_SIZEOF_PROCESS && seg_bytes[1] + 8 != M68K_SIZEOF_PROCESS);
    /* what stands in the way */
    if (cause == 0)
        heap_refuse_size = M68K_SIZEOF_PROCESS;
    if (cause == 1)
        ctx->num_shadows = M68KEMU_MAX_SHADOWS;
    seglist_tags(ctx, 32768, EXITCODE, 0x55);
    first = heap_blocks;
    r = create(ctx);
    stack = live_block(32768);
    check(r == 0, label[1]);
    check(copy_made(ctx, first) && info_calls == 2 && bptr_ins == 1, label[2]);
    /* the rows of the copy's two blocks, in the heap's own record */
    copy0 = blocks[first].addr;
    copy1 = blocks[first + 1].addr;
    check(creates == creates0 + 1 && made[creates0].entry == (IPTR)child_entry && made[creates0].userdata &&
          made[creates0].lock == &root.pr_Task, label[3]);
    /* the ownership this case is about: the copy, in the heap's own state */
    check(freed_once(copy0) && freed_once(copy1) && stack && heap_live() == live0 + 1, label[4]);
    check(ctx->num_children == 1 && holds == holds0 + 1 && releases == releases0 && vec_live() == 1 &&
          vec_is_live(made[creates0].userdata) && stack, label[5]);
    check(ctx->num_shadows == (cause == 0 ? real : M68KEMU_MAX_SHADOWS) && !guest_of(ctx, &pool[creates0].pr_Task), label[6]);
    check(guest_runs == runs0 && gil_obtains == obtains0 && lock_owner == &root.pr_Task, label[7]);
    check(parent_segments_kept(ctx), label[8]);
    /* there is room again before the child gets the lock */
    heap_refuse_size = 0;
    ctx->num_shadows = real;
    rc = run_child(creates0);
    parent_runs();
    check(guest_runs == runs0 && !ran(copy0 + 8) && !ran(EXITCODE), label[9]);
    check(rc == 0, label[10]);
    check(stack && freed_once(stack) && no_double_free(), label[11]);
    check(ctx->num_children == 0 && holds == holds0 + 1 && releases == releases0 + 1 && vec_live() == 0 && vec_bad_frees == 0, label[12]);
    check(gil_obtains == obtains0 + 1 && ts_drops == drops0 + 1 && !lock_violations, label[13]);
    check(heap_live() == live0 && !blocks[first].live && !blocks[first + 1].live, label[14]);
    check(same(ctx, &s), label[15]);
}

static void test_cancelled_copy(struct M68KEmuContext *ctx)
{
    unsigned before = failures, logs0, logs1, bugs0 = bugs;

    logs0 = logs;
    cancelled_copy_case(ctx, 0);
    logs1 = logs;
    cancelled_copy_case(ctx, 1);
    check(logs1 - logs0 == 0 && logs - logs1 == 1 && bugs == bugs0 && !unexpected && !low_traps && !copies_outside,
          "copy: the registry reports a full table once in its case only; no stream, trace, low-memory or foreign seglist path is reached");
    printf("GROUP cancelled-copy: %u failed\n", failures - before);
}

/* ---- who gives a copy back: the child, when dos would unload its seglist ---- */

#define BORROW 0x2000u                      /* a segment in the program's own memory: its link word */
static ULONG regs[16];                      /* the data registers of the CPU that calls a dos entry */

static struct M68KChild *record_of(unsigned i) { return (struct M68KChild *)made[i].userdata; }

/* the row of the fixture's heap that was allocated at this address */
static unsigned row_of(ULONG addr)
{
    unsigned i;

    for (i = 0; i < heap_blocks; i++)
    {
        if (blocks[i].addr == addr)
            return i;
    }
    return BLOCKS;
}

/* The tags of a child: its code from NP_Seglist (seglist), or NP_Entry
   (entry); NP_FreeSeglist when given; an exit code when given. */
static void owned_tags(struct M68KEmuContext *ctx, ULONG seglist, ULONG entry, ULONG stack, BOOL give, ULONG freeseg, ULONG exitcode)
{
    tags_begin();
    if (entry)
        tag(ctx, (ULONG)NP_Entry, entry);
    if (seglist)
        tag(ctx, (ULONG)NP_Seglist, seglist);
    tag(ctx, (ULONG)NP_StackSize, stack);
    tag(ctx, (ULONG)NP_Name, NAME);
    if (give)
        tag(ctx, (ULONG)NP_FreeSeglist, freeseg);
    if (exitcode)
    {
        tag(ctx, (ULONG)NP_ExitCode, exitcode);
        tag(ctx, (ULONG)NP_ExitData, 0x77);
    }
    tag(ctx, (ULONG)TAG_DONE, 0);
}

/* CreateNewProc as the program calls it: D1, the tag list */
static ULONG create_new(struct M68KEmuContext *ctx)
{
    memset(regs, 0, sizeof(regs));
    regs[1] = TAGS;
    return (ULONG)m68k_dos_CreateNewProc(ctx, regs);
}

static void test_owned_copy(struct M68KEmuContext *ctx)
{
    unsigned before = failures, first, a_first, b_first, k, ok, live0, srow;
    ULONG r, stack, published, port;
    UBYTE borrowed[16];

    /* CreateNewProc without NP_FreeSeglist, with an exit code: dos would unload the seglist at the end */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    owned_tags(ctx, seg_handle, 0, 16384, FALSE, 0, EXITCODE);
    live0 = heap_live();
    first = heap_blocks;
    watch_row = (int)first;
    r = create_new(ctx);
    stack = live_block(16384);
    check(r && copy_made(ctx, first) && record_of(0)->copy == blocks[first].addr + 8 && blocks[first].live && blocks[first + 1].live &&
          !made[0].freeseglist_given,
          "owned: CreateNewProc without NP_FreeSeglist gives the child its copy, which stays allocated until the child runs");
    (void)run_child(0);
    parent_runs();
    check(guest_runs == 2 && runs[0].entry == blocks[first].addr + 8 && runs[1].entry == EXITCODE && runs[0].watched_live && runs[1].watched_live,
          "owned: the entry and the exit code run while the copy is allocated");
    srow = row_of(stack);
    check(stack && srow < BLOCKS && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr) && freed_once(stack) &&
          blocks[first].order < blocks[srow].order && blocks[first + 1].order < blocks[srow].order &&
          blocks[first].holder == &pool[0].pr_Task && blocks[first + 1].holder == &pool[0].pr_Task && no_double_free(),
          "owned: after the exit code the child gives the copy back, each block once, holding the lock, before its stack");
    check(heap_live() == live0 + 1 && vec_live() == 0 && ctx->num_children == 0 && releases == holds && parent_segments_kept(ctx),
          "owned: the heap holds what it held before the call and the child's guest Process; record, count and hold are back");
    /* NP_FreeSeglist TRUE given */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    owned_tags(ctx, seg_handle, 0, 16384, TRUE, 1, 0);
    first = heap_blocks;
    watch_row = (int)first;
    r = create(ctx);
    ok = r && record_of(0)->copy == blocks[first].addr + 8 && made[0].freeseglist_given && made[0].freeseglist == 1;
    (void)run_child(0);
    parent_runs();
    check(ok && runs[0].watched_live && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr) && no_double_free(),
          "owned: NP_FreeSeglist TRUE is passed on to dos as given, and the child owns its copy and gives it back once");
    /* a fault of the program while the child runs */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    owned_tags(ctx, seg_handle, 0, 16384, FALSE, 0, EXITCODE);
    first = heap_blocks;
    r = create(ctx);
    ctx->faulted = TRUE;
    (void)run_child(0);
    parent_runs();
    check(r && guest_runs == 1 && !ran(EXITCODE) && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr) && no_double_free(),
          "owned: after a fault no exit code runs, and the copy is given back once all the same");
    /* three children of the copy, one after another */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    live0 = heap_live();
    ok = TRUE;
    for (k = 0; k < 3; k++)
    {
        owned_tags(ctx, seg_handle, 0, 16384, FALSE, 0, 0);
        first = heap_blocks;
        r = create(ctx);
        (void)run_child(k);
        parent_runs();
        ok = ok && r && copy_made(ctx, first) && freed_once(blocks[first].addr) && freed_once(blocks[first + 1].addr);
    }
    check(ok && heap_live() == live0 + 3 && no_double_free(),
          "owned: three children of the copy leave no block of any copy behind: the program's heap does not grow by them");
    /* NP_FreeSeglist FALSE: the program keeps the seglist, and what the child published of it */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    owned_tags(ctx, seg_handle, 0, 16384, TRUE, 0, 0);
    first = heap_blocks;
    r = create(ctx);
    stack = live_block(16384);
    check(r && copy_made(ctx, first) && record_of(0)->copy == 0 && made[0].freeseglist_given && made[0].freeseglist == 0,
          "kept: NP_FreeSeglist FALSE is passed on to dos as given, and the child does not own the copy");
    (void)run_child(0);
    parent_runs();
    /* what an interrupt, a timer vector or a semaphore the child installed would keep: an address in its copy */
    published = blocks[first].addr + 8 + RELOC_AT;
    check(blocks[first].live && blocks[first + 1].live && blocks[first].frees == 0 && blocks[first + 1].frees == 0 && stack && freed_once(stack) &&
          m68k_read32(ctx, published) == blocks[first + 1].addr + 8 + RELOC_ORIGINAL && no_double_free(),
          "kept: after the child the copy is still allocated and an address it published reads as before");
    /* the old CreateProc: dos never unloads its seglist */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    first = heap_blocks;
    memset(regs, 0, sizeof(regs));
    regs[1] = NAME;
    regs[3] = seg_handle;
    regs[4] = 16384;
    port = (ULONG)m68k_dos_CreateProc(ctx, regs);
    stack = live_block(16384);
    check(port && shadow_lookup(ctx, port - M68K_PR_MSGPORT) == &pool[0].pr_Task && port == guest_of(ctx, &pool[0].pr_Task) + M68K_PR_MSGPORT &&
          copy_made(ctx, first) && record_of(0)->copy == 0 && !made[0].freeseglist_given,
          "kept: CreateProc returns the child's port, makes the copy and gives it to no one; nothing of NP_FreeSeglist is added");
    (void)run_child(0);
    parent_runs();
    published = blocks[first].addr + 8 + RELOC_AT;
    check(blocks[first].live && blocks[first + 1].live && blocks[first].frees == 0 && stack && freed_once(stack) &&
          m68k_read32(ctx, published) == blocks[first + 1].addr + 8 + RELOC_ORIGINAL && no_double_free(),
          "kept: after a CreateProc child the copy is still allocated and an address it published reads as before");
    /* a segment in the program's own memory runs in place */
    reset(ctx);
    raw32(ctx, BORROW - 4, 16 + 8);
    raw32(ctx, BORROW, 0);
    for (k = 0; k < 16; k++)
        ctx->mem[BORROW + 4 + k] = (UBYTE)(0x90 + k);
    memcpy(borrowed, ctx->mem + BORROW + 4, sizeof(borrowed));
    owned_tags(ctx, BORROW >> 2, 0, 16384, FALSE, 0, 0);
    live0 = heap_live();
    r = create(ctx);
    stack = live_block(16384);
    check(r && record_of(0)->copy == 0 && heap_live() == live0 + 2 && stack,
          "borrowed: a segment of the program's own memory is not copied: the stack and the guest Process are all the call takes");
    (void)run_child(0);
    parent_runs();
    check(guest_runs == 1 && runs[0].entry == BORROW + 4 && freed_once(stack) && heap_live() == live0 + 1 &&
          !memcmp(borrowed, ctx->mem + BORROW + 4, sizeof(borrowed)) && no_double_free(),
          "borrowed: the child runs it in place and frees only its stack; the segment is as it was");
    /* NP_Entry with NP_FreeSeglist TRUE: nothing to own */
    reset(ctx);
    owned_tags(ctx, 0, ENTRY, 16384, TRUE, 1, 0);
    live0 = heap_live();
    r = create(ctx);
    stack = live_block(16384);
    ok = r && record_of(0)->copy == 0;
    (void)run_child(0);
    parent_runs();
    check(ok && runs[0].entry == ENTRY && freed_once(stack) && heap_live() == live0 + 1 && no_double_free(),
          "direct: a child of NP_Entry owns no copy, even with NP_FreeSeglist TRUE, and frees only its stack");
    /* an earlier child that owns its copy, and another child that is cancelled */
    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    owned_tags(ctx, seg_handle, 0, 16384, FALSE, 0, 0);
    a_first = heap_blocks;
    r = create(ctx);
    owned_tags(ctx, seg_handle, 0, 32768, FALSE, 0, 0);
    heap_refuse_size = M68K_SIZEOF_PROCESS;
    b_first = heap_blocks;
    (void)create(ctx);
    heap_refuse_size = 0;
    check(r && creates == 2 && freed_once(blocks[b_first].addr) && freed_once(blocks[b_first + 1].addr) && record_of(1)->copy == 0 &&
          blocks[a_first].live && blocks[a_first + 1].live && record_of(0)->copy == blocks[a_first].addr + 8,
          "owned: another child's cancellation gives back that child's copy only; the earlier child keeps its own and still owns it");
    (void)run_child(1);
    parent_runs();
    (void)run_child(0);
    parent_runs();
    check(guest_runs == 1 && runs[0].entry == blocks[a_first].addr + 8 && freed_once(blocks[a_first].addr) && freed_once(blocks[a_first + 1].addr) &&
          ctx->num_children == 0 && no_double_free(),
          "owned: then the cancelled child runs nothing, the earlier child runs its copy and gives it back once, and no block is freed twice");
    watch_row = -1;
    check(!unexpected && !low_traps && !copies_outside,
          "owned: no stream, trace, low-memory or foreign seglist path is reached, and no copy leaves the program's memory");
    printf("GROUP owned-copy: %u failed\n", failures - before);
}

/* ---- two NP_FreeSeglist tags: the first one decides, as dos has it ---- */

#define TAGS2 0x1100u                       /* where a tag list goes on after TAG_MORE */

/* A child of the copy with two NP_FreeSeglist tags, the second one in the
   same list or behind TAG_MORE. */
static void duplicate_tags(struct M68KEmuContext *ctx, ULONG first, ULONG second, BOOL more)
{
    tags_begin();
    tag(ctx, (ULONG)NP_Seglist, seg_handle);
    tag(ctx, (ULONG)NP_StackSize, 16384);
    tag(ctx, (ULONG)NP_Name, NAME);
    tag(ctx, (ULONG)NP_FreeSeglist, first);
    if (more)
    {
        tag(ctx, (ULONG)TAG_MORE, TAGS2);
        tag_at = TAGS2;
    }
    tag(ctx, (ULONG)NP_FreeSeglist, second);
    tag(ctx, (ULONG)TAG_DONE, 0);
}

static const char *const dup_labels[4][2] = {
    { "duplicate, FALSE then TRUE: both tags are passed on to dos in their order, the first is the one dos goes by, and the child does not own the copy",
      "duplicate, FALSE then TRUE: after the child the copy is still allocated and an address it published reads as before" },
    { "duplicate, TRUE then FALSE: both tags are passed on to dos in their order, the first is the one dos goes by, and the child owns the copy",
      "duplicate, TRUE then FALSE: the child gives the copy back once, holding the lock, after its entry and before its stack; the heap is as before plus its guest Process" },
    { "duplicate behind TAG_MORE, FALSE then TRUE: both tags are passed on to dos in their order, the first is the one dos goes by, and the child does not own the copy",
      "duplicate behind TAG_MORE, FALSE then TRUE: after the child the copy is still allocated and an address it published reads as before" },
    { "duplicate behind TAG_MORE, TRUE then FALSE: both tags are passed on to dos in their order, the first is the one dos goes by, and the child owns the copy",
      "duplicate behind TAG_MORE, TRUE then FALSE: the child gives the copy back once, holding the lock, after its entry and before its stack; the heap is as before plus its guest Process" }};

static void duplicate_case(struct M68KEmuContext *ctx, unsigned which)
{
    const char *const *label = dup_labels[which];
    ULONG first = (which & 1) ? 1 : 0, second = first ? 0 : 1, r, stack, published;
    BOOL more = which >= 2;
    unsigned row, srow, live0;

    reset(ctx);
    seglist_make(ctx, TRUE, TRUE);
    duplicate_tags(ctx, first, second, more);
    live0 = heap_live();
    row = heap_blocks;
    watch_row = (int)row;
    r = create_new(ctx);
    stack = live_block(16384);
    check(r && copy_made(ctx, row) && made[0].freeseglist_count == 2 && made[0].freeseglist_values[0] == first &&
          made[0].freeseglist_values[1] == second && made[0].freeseglist == first &&
          record_of(0)->copy == (first ? blocks[row].addr + 8 : 0), label[0]);
    (void)run_child(0);
    parent_runs();
    srow = row_of(stack);
    if (first)
        check(guest_runs == 1 && runs[0].watched_live && stack && srow < BLOCKS && freed_once(blocks[row].addr) && freed_once(blocks[row + 1].addr) &&
              freed_once(stack) && blocks[row].order < blocks[srow].order && blocks[row + 1].order < blocks[srow].order &&
              blocks[row].holder == &pool[0].pr_Task && heap_live() == live0 + 1 && no_double_free(), label[1]);
    else
    {
        published = blocks[row].addr + 8 + RELOC_AT;
        check(blocks[row].live && blocks[row + 1].live && blocks[row].frees == 0 && blocks[row + 1].frees == 0 && stack && freed_once(stack) &&
              m68k_read32(ctx, published) == blocks[row + 1].addr + 8 + RELOC_ORIGINAL && no_double_free(), label[1]);
    }
    watch_row = -1;
}

static void test_duplicate_freeseglist(struct M68KEmuContext *ctx)
{
    unsigned before = failures, which;

    for (which = 0; which < 4; which++)
        duplicate_case(ctx, which);
    printf("GROUP duplicate-freeseglist: %u failed\n", failures - before);
}

int main(void)
{
    static struct M68KEmuContext ctx;
    unsigned i;

    ctx.mem_size = MEMORY;
    ctx.mem = malloc(ctx.mem_size);
    assert(ctx.mem);
    active = &ctx;
    test_creation(&ctx);
    test_refusal(&ctx);
    test_projection(&ctx);
    test_seglist(&ctx);
    test_cancelled_copy(&ctx);
    test_owned_copy(&ctx);
    test_duplicate_freeseglist(&ctx);
    for (i = 0; i < vec_blocks; i++)
    {
        if (vecs[i].live)
            free(vecs[i].p);
    }
    for (i = 0; i < SEGS; i++)
        free(seg_alloc[i]);
    free(ctx.mem);
    printf("child projection: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
