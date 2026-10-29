/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#ifdef PRIVATEPOOL_HOST_TEST
#include "host_support.h"
#else
#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#endif

static unsigned checks, failures;

static void check(int okay, const char *what)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("PRIVATE POOL FAIL: %s\n", what);
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
    f->header.mh_First = (struct MemChunk *)f->base;
    f->header.mh_First->mc_Next = NULL;
    f->header.mh_First->mc_Bytes = capacity;
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
            bug("PRIVATE POOL RANDOM: first bad state at operation %u, slot %u\n", i, slot);
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
    IPTR n = ((IPTR)UINT32_MAX + 1) + 37;
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
          "managed allocation preserves a request larger than 4 GiB");
    stdDealloc(&mh.header, NULL, p, n, NULL, NULL);
    check(managed_block == p && managed_size == n && managed_calls == 2,
          "managed deallocation preserves 64-bit pointers and sizes");
#if defined(__AROSEXEC_SMP__)
    check(mh.header.mh_SpinLock == 0, "managed header lock is released");
    mh.header.mh_First = (struct MemChunk *)MEMF_SEM_PROTECTED;
    p = stdAlloc(&mh.header, NULL, n, 0, NULL, NULL);
    stdDealloc(&mh.header, NULL, p, n, NULL, NULL);
    check(managed_size == n && managed_calls == 4,
          "semaphore-protected managed allocator keeps its delegation path");
#endif
}

#if defined(__AROSEXEC_SMP__)
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

static void host_checks(void)
{
    struct Fixture f;
    unsigned before;
    APTR p;

    if (!setup(&f, MEMCHUNK_TOTAL * 4))
        return;
    check(sizeof(IPTR) == sizeof(void *) && sizeof(IPTR) == 8,
          "host exercises 64-bit pointers and sizes");
    check((IPTR)f.base > UINT32_MAX, "host pool uses an address above 4 GiB");
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
        Deallocate(&f.header, p, (IPTR)-1);
        check(allocator_alerts > before && f.header.mh_Free == free_before,
              "overflowing free is rejected without changing accounting");
        Deallocate(&f.header, p, MEMCHUNK_TOTAL);
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
#if defined(__AROSEXEC_SMP__)
    concurrent_pool();
#endif
}
#endif

int main(void)
{
    ufo_pool();
    tails();
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
    bug("PRIVATE POOL TEST: %u/%u pass, %u failures\n",
        checks - failures, checks, failures);
    return failures ? 20 : 0;
}
