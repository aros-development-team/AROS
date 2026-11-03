/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#ifdef PRIVATEPOOL_HOST_TEST
#include "host_support.h"
#define report printf
#else
#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#define report Printf
#endif

static unsigned checks, failures;

static void check(int okay, const char *what)
{
    checks++;
    if (!okay)
    {
        failures++;
        report("PRIVATE POOL FAIL: %s\n", what);
#ifndef PRIVATEPOOL_HOST_TEST
        bug("PRIVATE POOL FAIL: %s\n", what);
#endif
    }
}

struct Fixture
{
    struct MemHeader header;
    UBYTE *storage, *base;
    IPTR capacity;
};

static int setup(struct Fixture *f, IPTR capacity)
{
    IPTR a = MEMCHUNK_TOTAL;

    memset(f, 0, sizeof(*f));
#if defined(PRIVATEPOOL_HOST_TEST) && defined(__AROSEXEC_SMP__)
    atomic_init(&f->header.mh_SpinLock, 0);
#endif
    f->storage = AllocMem(capacity + a * 3, MEMF_ANY);
    check(f->storage != NULL, "allocate guarded pool storage");
    if (!f->storage)
        return 0;
    f->base = (UBYTE *)(((IPTR)f->storage + a * 2 - 1) & ~(a - 1));
    f->capacity = capacity;
    memset(f->base - a, 0xb7, capacity + a * 2);
    f->header.mh_Node.ln_Type = NT_MEMORY;
    f->header.mh_Lower = f->base;
    f->header.mh_Upper = f->base + capacity;
    f->header.mh_Free = capacity;
    if (capacity >= MEMCHUNK_TOTAL)
    {
        f->header.mh_First = (struct MemChunk *)f->base;
        f->header.mh_First->mc_Next = NULL;
        f->header.mh_First->mc_Bytes = capacity;
    }
    return 1;
}

static void teardown(struct Fixture *f)
{
    IPTR i, a = MEMCHUNK_TOTAL;
    int intact = 1;

    for (i = 0; i < a; i++)
        if ((f->base - a)[i] != 0xb7 ||
            f->base[f->capacity + i] != 0xb7)
            intact = 0;
    check(intact, "pool boundary guards remain intact");
    FreeMem(f->storage, f->capacity + a * 3);
}

static void restored(struct Fixture *f)
{
    check(f->header.mh_Free == f->capacity, "restore the original free-byte count");
    check(f->header.mh_First == (struct MemChunk *)f->base,
          "coalesce back to the original first chunk");
    if (f->header.mh_First)
        check(f->header.mh_First->mc_Next == NULL &&
              f->header.mh_First->mc_Bytes == f->capacity,
              "restore the complete original extent");
}

static void ufo_pool(void)
{
    struct Fixture f;
    APTR p;

    if (!setup(&f, 128140))
        return;
    p = Allocate(&f.header, 64020);
    check(p == f.base, "UFO-size allocation uses the first aligned block");
    if (p)
    {
        check(((IPTR)p & (MEMCHUNK_TOTAL - 1)) == 0, "returned block is aligned");
        memset(p, 0x39, 64020);
        Deallocate(&f.header, p, 64020);
        restored(&f);
    }
    teardown(&f);
}

static void tails(void)
{
    static const unsigned orders[6][3] = {
        {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
    };
    IPTR tail, a = MEMCHUNK_TOTAL;
    unsigned order, i;

    for (tail = 0; tail < a; tail++)
        for (order = 0; order < 6; order++)
        {
            struct Fixture f;
            APTR p[3] = {NULL, NULL, NULL};
            int complete = 1;

            if (!setup(&f, a * 3 + tail))
                return;
            for (i = 0; i < 3; i++)
            {
                p[i] = Allocate(&f.header, a);
                check(p[i] == f.base + i * a, "first-fit allocation preserves block addresses");
                if (!p[i])
                {
                    complete = 0;
                    break;
                }
                memset(p[i], i + 1, a);
            }
            if (complete)
            {
                check(f.header.mh_Free == tail, "retain unusable tail bytes in accounting");
                check(Allocate(&f.header, 1) == NULL, "unusable tail cannot satisfy an allocation");
                for (i = 0; i < 3; i++)
                    Deallocate(&f.header, p[orders[order][i]], a);
                restored(&f);
            }
            teardown(&f);
        }
}

/* Populate an empty header through the public API, including an uneven
 * upper bound and a lower bound inside the first aligned allocation unit.
 * The caller owns the complete aligned storage, including that prefix. */
static void initialise_pool(void)
{
    IPTR tail, offset, a = MEMCHUNK_TOTAL;

    for (tail = 0; tail < a; tail++)
        for (offset = 0; offset < a; offset += 4)
        {
            struct Fixture f;
            APTR p;

            if (!setup(&f, a * 3 + tail))
                return;
            f.header.mh_First = NULL;
            f.header.mh_Free = 0;
            f.header.mh_Lower = f.base + offset;
            Deallocate(&f.header, f.base + offset, f.capacity - offset);
            restored(&f);
            p = Allocate(&f.header, a * 3);
            check(p == f.base, "allocate a pool populated by Deallocate");
            if (p)
            {
                memset(p, 0x63, a * 3);
                Deallocate(&f.header, p, a * 3);
                restored(&f);
            }
            teardown(&f);
        }
}

static void tiny_regions(void)
{
    IPTR tail;

    for (tail = 1; tail < MEMCHUNK_TOTAL; tail++)
    {
        struct Fixture f;

        if (!setup(&f, tail))
            return;
        f.header.mh_Free = 0;
        Deallocate(&f.header, f.base, tail);
        check(f.header.mh_First == NULL && f.header.mh_Free == tail,
              "initialise a sub-chunk region without writing a header");
        check(Allocate(&f.header, 1) == NULL,
              "a sub-chunk region cannot satisfy an allocation");
        teardown(&f);
    }
}

static void explicit_tail_free(void)
{
    IPTR tail, a = MEMCHUNK_TOTAL;

    for (tail = 1; tail < a; tail++)
    {
        struct Fixture f;
        APTR p, q;

        if (!setup(&f, a * 3 + tail))
            return;
        p = Allocate(&f.header, a * 3);
        check(p != NULL, "allocate before explicitly freeing an orphan tail");
        if (p)
        {
            Deallocate(&f.header, p, f.capacity);
            restored(&f);
        }
        p = Allocate(&f.header, a);
        q = Allocate(&f.header, a * 2);
        check(p && q, "allocate blocks before merging an explicit terminal tail");
        if (p && q)
        {
            Deallocate(&f.header, p, a);
            Deallocate(&f.header, q, a * 2 + tail);
            restored(&f);
        }
        teardown(&f);
    }
}

static void informational_lower(void)
{
    struct Fixture f;
    APTR p;

    if (!setup(&f, MEMCHUNK_TOTAL * 4))
        return;
    /* A private allocator traverses mh_First; mh_Lower is informational. */
    f.header.mh_Lower = f.base + MEMCHUNK_TOTAL;
    p = Allocate(&f.header, MEMCHUNK_TOTAL);
    check(p == f.base, "a private first chunk may precede mh_Lower");
    if (p)
    {
        Deallocate(&f.header, p, MEMCHUNK_TOTAL);
        restored(&f);
    }
    p = Allocate(&f.header, f.capacity);
    check(p == f.base && f.header.mh_First == NULL,
          "exhaust a private region whose informational lower bound is higher");
    if (p)
    {
        Deallocate(&f.header, p, f.capacity);
        restored(&f);
    }
    teardown(&f);
}

static int consistent(struct Fixture *f)
{
    struct MemChunk *mc = f->header.mh_First;
    IPTR sum = 0, end = (IPTR)f->base;
    IPTR upper = (IPTR)f->header.mh_Upper;
    unsigned count = 0;

    while (mc)
    {
        IPTR p = (IPTR)mc;

        if (++count > 65 || p < end || p > upper ||
            sizeof(*mc) > upper - p || (p & (MEMCHUNK_TOTAL - 1)))
            return 0;
        if (mc->mc_Bytes < sizeof(*mc) || mc->mc_Bytes > upper - p)
            return 0;
        sum += mc->mc_Bytes;
        end = p + mc->mc_Bytes;
        mc = mc->mc_Next;
    }
    if (end != upper)
        sum += upper & (MEMCHUNK_TOTAL - 1);
    return sum == f->header.mh_Free;
}

/* Populate an empty region in separately returned allocation units. The
 * last piece has no room for a header until its preceding unit is free. */
static void piecemeal_regions(void)
{
    static const unsigned orders[24][4] = {
        {0,1,2,3}, {0,1,3,2}, {0,2,1,3}, {0,2,3,1}, {0,3,1,2}, {0,3,2,1},
        {1,0,2,3}, {1,0,3,2}, {1,2,0,3}, {1,2,3,0}, {1,3,0,2}, {1,3,2,0},
        {2,0,1,3}, {2,0,3,1}, {2,1,0,3}, {2,1,3,0}, {2,3,0,1}, {2,3,1,0},
        {3,0,1,2}, {3,0,2,1}, {3,1,0,2}, {3,1,2,0}, {3,2,0,1}, {3,2,1,0}
    };
    IPTR tail, a = MEMCHUNK_TOTAL;
    unsigned order, step;

    for (tail = 1; tail < a; tail++)
        for (order = 0; order < 24; order++)
        {
            struct Fixture f;
            unsigned freed[4] = {0};
            IPTR expected = 0;

            if (!setup(&f, a * 3 + tail))
                return;
            f.header.mh_First = NULL;
            f.header.mh_Free = 0;
            memset(f.base, 0x6d, f.capacity);
            for (step = 0; step < 4; step++)
            {
                unsigned piece = orders[order][step], i;
                IPTR j, size = piece == 3 ? tail : a;
                int intact = 1;

                Deallocate(&f.header, f.base + piece * a, size);
                freed[piece] = 1;
                expected += size;
                check(f.header.mh_Free == expected,
                      "piecemeal free counts each returned byte exactly once");
                for (j = 0; j < a; j++)
                    if ((f.base - a)[j] != 0xb7 ||
                        f.base[f.capacity + j] != 0xb7)
                        intact = 0;
                check(intact, "separate terminal frees preserve pool boundary guards");
                intact = 1;
                for (i = 0; i < 4; i++)
                    if (!freed[i] || (i == 3 && !freed[2]))
                        for (j = 0; j < (i == 3 ? tail : a); j++)
                            if (f.base[i * a + j] != 0x6d)
                                intact = 0;
                check(intact, "live pieces and an isolated tail are not overwritten");
                if (freed[3])
                {
                    int valid = consistent(&f);

                    check(valid, "piecemeal terminal free leaves a valid free list");
                    /* Avoid following a damaged list in the negative control. */
                    if (valid)
                    {
                        APTR block = Allocate(&f.header, a);
                        int owned = 0;

                        for (i = 0; i < 3; i++)
                            if (freed[i] && block == f.base + i * a)
                                owned = 1;
                        check(expected >= a ? owned : block == NULL,
                              "allocation after a separate tail uses only returned units");
                        if (block)
                        {
                            memset(block, 0x39, a);
                            Deallocate(&f.header, block, a);
                        }
                        check(f.header.mh_Free == expected && consistent(&f),
                              "allocation round trip retains the isolated tail count");
                    }
                }
            }
            restored(&f);
            if (consistent(&f))
            {
                APTR block = Allocate(&f.header, a * 3);

                check(block == f.base, "piecemeal population restores the full usable region");
                if (block)
                    Deallocate(&f.header, block, a * 3);
                restored(&f);
            }
            teardown(&f);
        }
}

static void fragmented(void)
{
    struct Fixture f;
    APTR blocks[32] = {NULL};
    IPTR sizes[32] = {0};
    unsigned random = 0x31415926, i, slot;
    int okay = 1;

    if (!setup(&f, MEMCHUNK_TOTAL * 8192 + 4))
        return;
    for (i = 0; i < 4000; i++)
    {
        IPTR j;

        random = random * 1664525u + 1013904223u;
        slot = (random >> 16) & 31;
        if (blocks[slot])
        {
            for (j = 0; j < sizes[slot]; j++)
                if (((UBYTE *)blocks[slot])[j] != slot + 1)
                    okay = 0;
            Deallocate(&f.header, blocks[slot], sizes[slot]);
            blocks[slot] = NULL;
        }
        else
        {
            sizes[slot] = 1 + ((random >> 5) & 1023);
            blocks[slot] = Allocate(&f.header, sizes[slot]);
            if (blocks[slot])
                memset(blocks[slot], slot + 1, sizes[slot]);
        }
        if (!okay || !consistent(&f))
        {
            report("PRIVATE POOL RANDOM: first bad state at operation %u, slot %u\n", i, slot);
            check(0, "mixed-size operations preserve data and the free list");
            teardown(&f);
            return;
        }
    }
    check(okay, "mixed-size blocks retain their contents");
    for (slot = 0; slot < 32; slot++)
        if (blocks[slot])
            Deallocate(&f.header, blocks[slot], sizes[slot]);
    restored(&f);
    teardown(&f);
}

#ifndef PRIVATEPOOL_HOST_TEST
struct TaskWorker { struct Fixture *fixture; unsigned id; int bad; };

static LONG task_worker(void)
{
    struct TaskWorker *w = FindTask(NULL)->tc_UserData;
    APTR blocks[8] = {NULL};
    IPTR sizes[8] = {0};
    unsigned random = 0x27182818 + w->id, i, slot;

    for (i = 0; i < 2000; i++)
    {
        IPTR j;

        random = random * 1664525u + 1013904223u;
        slot = (random >> 16) & 7;
        if (blocks[slot])
        {
            for (j = 0; j < sizes[slot]; j++)
                if (((UBYTE *)blocks[slot])[j] != w->id + 1)
                    w->bad = 1;
            Deallocate(&w->fixture->header, blocks[slot], sizes[slot]);
            blocks[slot] = NULL;
        }
        else
        {
            sizes[slot] = 1 + ((random >> 5) & 127);
            blocks[slot] = Allocate(&w->fixture->header, sizes[slot]);
            if (!blocks[slot])
                w->bad = 1;
            else
                memset(blocks[slot], w->id + 1, sizes[slot]);
        }
        if (!(i & 63))
            Delay(1);
    }
    for (slot = 0; slot < 8; slot++)
        if (blocks[slot])
            Deallocate(&w->fixture->header, blocks[slot], sizes[slot]);
    return 0;
}

static void task_sharing(void)
{
    struct Fixture f;
    struct TaskWorker workers[4];
    ULONG ids[4];
    unsigned started = 0, i;

    if (!setup(&f, MEMCHUNK_TOTAL * 2048 + 4))
        return;
    for (i = 0; i < 4; i++)
    {
        struct Process *child;

        workers[i] = (struct TaskWorker){&f, i, 0};
        child = CreateNewProcTags(NP_Entry, (IPTR)task_worker,
            NP_Name, (IPTR)"private pool worker", NP_StackSize, 8192,
            NP_UserData, (IPTR)&workers[i], NP_NotifyOnDeath, TRUE, TAG_DONE);
        if (!child)
            break;
        ids[i] = GetETask((struct Task *)child)->et_UniqueID;
        started++;
    }
    check(started == 4, "start tasks sharing one private pool");
    for (i = 0; i < started; i++)
    {
        ChildWait(ids[i]);
        ChildFree(ids[i]);
        check(!workers[i].bad, "shared-pool tasks retain distinct live blocks");
    }
    restored(&f);
    teardown(&f);
}

#if !defined(__AROSEXEC_SMP__)
static void nesting(void)
{
    struct Fixture f;
    APTR p;
    BYTE before = SysBase->TDNestCnt, nested;

    if (!setup(&f, MEMCHUNK_TOTAL * 8))
        return;
    Forbid();
    nested = SysBase->TDNestCnt;
    p = Allocate(&f.header, 1);
    check(SysBase->TDNestCnt == nested, "Allocate preserves the caller's Forbid nesting");
    if (p)
        Deallocate(&f.header, p, 1);
    check(SysBase->TDNestCnt == nested, "Deallocate preserves the caller's Forbid nesting");
    Permit();
    check(SysBase->TDNestCnt == before, "restore the original scheduling state");
    teardown(&f);
}
#endif
#endif

#ifdef PRIVATEPOOL_HOST_TEST
static IPTR managed_size;
static APTR managed_block;
static unsigned managed_calls;

static APTR managed_allocate(struct MemHeaderExt *mh, IPTR n, ULONG *flags)
{
    (void)mh;
    (void)flags;
    managed_size = n;
    managed_calls++;
    return &managed_size;
}

static void managed_deallocate(struct MemHeaderExt *mh, APTR p, IPTR n)
{
    (void)mh;
    managed_size = n;
    managed_block = p;
    managed_calls++;
}

static void managed_pool(void)
{
    struct MemHeaderExt mh;
#if UINTPTR_MAX > UINT32_MAX
    IPTR n = ((IPTR)UINT32_MAX + 1) + 37;
#else
    IPTR n = 65537;
#endif
    APTR p;

    memset(&mh, 0, sizeof(mh));
    mh.header.mh_Node.ln_Type = NT_MEMORY;
    mh.header.mh_Attributes = 0x8000;
#if defined(__AROSEXEC_SMP__)
    atomic_init(&mh.header.mh_SpinLock, 0);
#endif
    mh.mhe_Alloc = managed_allocate;
    mh.mhe_Free = managed_deallocate;
    managed_calls = 0;
    p = stdAlloc(&mh.header, NULL, n, 0, NULL, NULL);
    check(p == &managed_size && managed_size == n && managed_calls == 1,
          "managed allocation preserves the pointer-sized request");
    stdDealloc(&mh.header, NULL, p, n, NULL, NULL);
    check(managed_block == p && managed_size == n && managed_calls == 2,
          "managed deallocation preserves pointers and sizes");
#if defined(__AROSEXEC_SMP__)
    check(mh.header.mh_SpinLock == 0, "managed header lock is released");
    mh.header.mh_First = (struct MemChunk *)MEMF_SEM_PROTECTED;
    p = stdAlloc(&mh.header, NULL, n, 0, NULL, NULL);
    stdDealloc(&mh.header, NULL, p, n, NULL, NULL);
    check(managed_size == n && managed_calls == 4,
          "semaphore-protected managed allocator keeps its delegation path");
#endif
}

#if defined(__AROSEXEC_SMP__) && !defined(__wasi__)
struct Worker { struct Fixture *fixture; unsigned id; int bad; };

static void *worker(void *argument)
{
    struct Worker *w = argument;
    unsigned i;

    for (i = 0; i < 1000; i++)
    {
        IPTR n = 1 + ((i * 7 + w->id) % (MEMCHUNK_TOTAL * 3));
        UBYTE *p = Allocate(&w->fixture->header, n);
        IPTR j;

        if (!p)
        {
            w->bad = 1;
            break;
        }
        memset(p, w->id + 1, n);
        sched_yield();
        for (j = 0; j < n; j++)
            if (p[j] != w->id + 1)
                w->bad = 1;
        Deallocate(&w->fixture->header, p, n);
    }
    return NULL;
}

static void concurrent_pool(void)
{
    struct Fixture f;
    struct Worker workers[4];
    pthread_t threads[4];
    unsigned started = 0, i;

    if (!setup(&f, MEMCHUNK_TOTAL * 256 + 4))
        return;
    for (i = 0; i < 4; i++)
    {
        workers[i] = (struct Worker){&f, i, 0};
        if (pthread_create(&threads[i], NULL, worker, &workers[i]))
            break;
        started++;
    }
    check(started == 4, "start concurrent pool workers");
    for (i = 0; i < started; i++)
    {
        pthread_join(threads[i], NULL);
        check(!workers[i].bad, "concurrent blocks remain distinct and usable");
    }
    restored(&f);
    teardown(&f);
}
#endif

/* A minimal system memory list lets the production NOMMU paths exercise
 * the same uneven chunks as Allocate/Deallocate. */
static void system_pool(void)
{
    IPTR a = MEMCHUNK_TOTAL, tail;

    for (tail = 1; tail < a; tail++)
    {
        struct Fixture f;
        struct ExecBase sys = {0};
        APTR p;
        unsigned before = allocator_alerts;

        if (!setup(&f, a * 3 + tail))
            return;
        host_memlist(&sys, &f.header);
        check(nommu_AvailMem(MEMF_LARGEST, &sys) == a * 3,
              "largest system chunk excludes the unusable tail");
        p = nommu_AllocMem(nommu_AvailMem(MEMF_LARGEST, &sys), 0, NULL, &sys);
        check(p == f.base, "allocate the reported largest system chunk");
        if (p)
        {
            struct TraceLocation t = {"FreeMem", NULL, NULL};
            nommu_FreeMem(p, a * 3, &t, &sys);
            restored(&f);
        }
        /* AllocAbs must not write a header into the remaining tail. */
        p = nommu_AllocAbs(f.base + a, a * 2, &sys);
        check(p == f.base + a, "absolute allocation accepts an uneven terminal chunk");
        if (p)
        {
            memset(p, 0x59, a * 2);
            check(nommu_AvailMem(MEMF_LARGEST, &sys) == a,
                  "absolute allocation leaves a usable leading chunk");
            Deallocate(&f.header, p, a * 2);
            restored(&f);
        }
        p = nommu_AllocAbs(f.base, a * 3, &sys);
        check(p == f.base && f.header.mh_First == NULL && f.header.mh_Free == tail,
              "absolute allocation can consume all usable bytes without a tail header");
        if (p)
            Deallocate(&f.header, p, a * 3);
        restored(&f);
        check(allocator_alerts == before, "valid NOMMU operations raise no alerts");
        teardown(&f);
    }
}

static void invalid_system_tail(void)
{
    struct Fixture f;
    struct ExecBase sys = {0};
    unsigned before;

    if (!setup(&f, MEMCHUNK_TOTAL * 3 + 3))
        return;
    host_memlist(&sys, &f.header);
    f.header.mh_First->mc_Bytes = f.capacity - 2;
    before = allocator_alerts;
    nommu_AvailMem(MEMF_LARGEST, &sys);
    check(allocator_alerts > before,
          "largest scan rejects an uneven chunk that does not reach mh_Upper");
    before = allocator_alerts;
    check(nommu_AllocAbs(f.base, MEMCHUNK_TOTAL, &sys) == NULL &&
          allocator_alerts > before && f.header.mh_Free == f.capacity,
          "absolute allocation rejects an invalid terminal remainder without mutation");
    f.header.mh_First->mc_Bytes = f.capacity;
    teardown(&f);
}

static void overlapping_tail(void)
{
    struct Fixture f;
    APTR block;

    if (!setup(&f, MEMCHUNK_TOTAL * 3 + 1))
        return;
    block = Allocate(&f.header, MEMCHUNK_TOTAL * 2);
    check(block == f.base, "allocate before freeing an already-free terminal byte");
    if (block)
    {
        IPTR before_free = f.header.mh_Free;
        struct MemChunk *before_first = f.header.mh_First;
        unsigned before_alerts = allocator_alerts;

        Deallocate(&f.header, f.base + MEMCHUNK_TOTAL * 3, 1);
        check(allocator_alerts == before_alerts + 1 && last_alert == AN_FreeTwice,
              "a small terminal free still reports overlap with a real chunk");
        check(f.header.mh_First == before_first && f.header.mh_Free == before_free &&
              consistent(&f), "rejected overlapping tail does not change the free list");
        Deallocate(&f.header, block, MEMCHUNK_TOTAL * 2);
        restored(&f);
    }
    teardown(&f);
}

static void host_nesting(void)
{
    struct Fixture f;
    APTR p;
    signed char level;

    if (!setup(&f, MEMCHUNK_TOTAL * 4))
        return;
    for (level = -1; level < 3; level++)
    {
        host_sys.TDNestCnt = level;
        host_sys.pending_switch = 1;
        host_sys.switches = 0;
        p = Allocate(&f.header, 1);
        check(p && host_sys.TDNestCnt == level && !host_sys.switches &&
              host_sys.pending_switch,
              "Allocate preserves nesting and leaves a pending reschedule deferred");
        if (p)
            Deallocate(&f.header, p, 1);
        check(host_sys.TDNestCnt == level && !host_sys.switches &&
              host_sys.pending_switch,
              "Deallocate preserves nesting and leaves a pending reschedule deferred");
    }
    host_sys.TDNestCnt = -1;
    host_sys.pending_switch = 0;
    p = stdAlloc(&f.header, NULL, 1, 0, NULL, NULL);
    check(p != NULL, "early-boot allocation works without a task or ExecBase");
    if (p)
        stdDealloc(&f.header, NULL, p, 1, NULL, NULL);
    restored(&f);
    teardown(&f);
}

static void host_checks(void)
{
    struct Fixture f;
    unsigned before;
    APTR p;

    if (!setup(&f, MEMCHUNK_TOTAL * 4))
        return;
    check(sizeof(IPTR) == sizeof(void *), "host uses pointer-sized arithmetic");
#ifdef __wasi__
    check(sizeof(IPTR) == 4 && sizeof(struct MemChunk) == 8 && MEMCHUNK_TOTAL == 8,
          "32-bit host exercises the eight-byte chunk layout used on m68k");
#else
    check(sizeof(IPTR) == 8 && (IPTR)f.base > UINT32_MAX,
          "64-bit host pool uses an address above 4 GiB");
#endif
    check(Allocate(&f.header, 0) == NULL, "zero request is rejected");
    check(Allocate(&f.header, (IPTR)-1) == NULL, "rounding overflow is rejected");
    restored(&f);
    before = allocator_alerts;
    f.header.mh_Free--;
    check(Allocate(&f.header, MEMCHUNK_TOTAL) == NULL,
          "misaligned count in an aligned extent is rejected");
    check(allocator_alerts > before, "corrupt aligned pool still raises an alert");
    f.header.mh_Free++;
    before = allocator_alerts;
    f.header.mh_First->mc_Bytes = f.capacity + MEMCHUNK_TOTAL;
    check(Allocate(&f.header, MEMCHUNK_TOTAL) == NULL,
          "chunk extending past the pool is rejected");
    check(allocator_alerts > before, "out-of-bounds chunk still raises an alert");
    f.header.mh_First->mc_Bytes = f.capacity;
    p = Allocate(&f.header, MEMCHUNK_TOTAL);
    check(p != NULL, "allocate before invalid free test");
    if (p)
    {
        IPTR free_before = f.header.mh_Free;
        before = allocator_alerts;
        struct TraceLocation t = {"Deallocate", NULL, NULL};
        stdDealloc(&f.header, NULL, p, (IPTR)-1, &t, &host_sys);
        check(allocator_alerts > before && f.header.mh_Free == free_before,
              "overflowing free is rejected without changing accounting");
        Deallocate(&f.header, p, MEMCHUNK_TOTAL);
        restored(&f);
    }
    p = Allocate(&f.header, f.capacity);
    check(p != NULL, "allocate before public free bounds checks");
    if (p)
    {
        unsigned extra;
        for (extra = 1; extra <= 2; extra++)
        {
            before = allocator_alerts;
            Deallocate(&f.header, p, f.capacity + extra);
            check(allocator_alerts == before + 1 && f.header.mh_Free == 0 &&
                  f.header.mh_First == NULL &&
                  last_alert == (AN_MemCorrupt | AT_DeadEnd),
                  "public out-of-range frees raise one consistent alert without mutation");
        }
        Deallocate(&f.header, p, f.capacity);
        restored(&f);
    }
    teardown(&f);

    if (!setup(&f, MEMCHUNK_TOTAL * 3 + 4))
        return;
    p = stdAlloc(&f.header, NULL, MEMCHUNK_TOTAL, MEMF_REVERSE, NULL, NULL);
    check(p == f.base + MEMCHUNK_TOTAL * 2, "reverse allocation remains aligned in an uneven extent");
    if (p)
    {
        stdDealloc(&f.header, NULL, p, MEMCHUNK_TOTAL, NULL, NULL);
        restored(&f);
    }
    teardown(&f);
    managed_pool();
    system_pool();
    invalid_system_tail();
    overlapping_tail();
    host_nesting();
#if defined(__AROSEXEC_SMP__) && !defined(__wasi__)
    concurrent_pool();
#endif
}
#endif

int main(void)
{
    ufo_pool();
    tails();
    initialise_pool();
    informational_lower();
    tiny_regions();
    explicit_tail_free();
    piecemeal_regions();
    fragmented();
#ifndef PRIVATEPOOL_HOST_TEST
    task_sharing();
#if !defined(__AROSEXEC_SMP__)
    nesting();
#endif
#endif
#ifdef PRIVATEPOOL_HOST_TEST
    host_checks();
#endif
    report("PRIVATE POOL TEST: %u/%u pass, %u failures\n",
        checks - failures, checks, failures);
#ifndef PRIVATEPOOL_HOST_TEST
    bug("PRIVATE POOL TEST: %u/%u pass, %u failures\n",
        checks - failures, checks, failures);
#endif
    return failures ? 20 : 0;
}
