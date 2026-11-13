/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for KrnMapGlobal()/KrnUnmapGlobal().

          Phase 1 maps one 4 KiB page inside a live 2 MiB mapping, which
          splits it, while another CPU reads the pages either side. The
          mapped page and both neighbours must keep their contents.

          Phase 2 has two CPUs unmap and remap pages in a loop. Every page
          remapped must read back what was written through it.

          Only memory the test allocates is touched. AllocMem() returns RAM
          inside the boot identity mapping, so virtual and physical address
          agree and the pages are mapped back that way before being freed.
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <aros/kernel.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <string.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs;

#define STACKSIZE       16384
#define WAIT_TICKS      500         /* 10 s bound on every wait */
#define PAGE            4096
#define BIGPAGE         (2 * 1024 * 1024)
#define BLOCKSIZE       (BIGPAGE + BIGPAGE)

#define SPLIT_OFFSET    0x80000     /* the page mapped in phase 1 */
#define READER_ITERS    20000

#define MAP_LOOPS       200
#define MAP_SLOTS       16          /* pages each phase-2 worker cycles over */
#define MAP_FLAGS       (MAP_Readable | MAP_Writable)

struct MapWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_Stop;
    volatile ULONG  w_Done;
    volatile ULONG  w_Count;
    volatile ULONG  w_Errors;
    volatile ULONG  w_Progress;
    int             w_Index;
    UBYTE          *w_Base;
};

/* The tasks hold pointers to these, so they outlive the functions that start
   the tasks. */
static struct MapWorker g_Reader;
static struct MapWorker g_Churn[2];

static struct MapWorker *myworker(void)
{
    return (struct MapWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, struct MapWorker *w)
{
    cpumask_t *mask = KrnAllocCPUMask();
    struct Task *t;

    if (!mask)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    t = NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                      TASKTAG_PC,        (IPTR)pc,
                      TASKTAG_STACKSIZE, STACKSIZE,
                      TASKTAG_USERDATA,  (IPTR)w,
                      TASKTAG_AFFINITY,  (IPTR)mask,
                      TAG_DONE);

    /* A created task owns the mask and frees it with itself. On failure the
       mask was never taken. */
    if (!t)
        KrnFreeCPUMask(mask);
    return t;
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

/* Stop and remove the first n tasks. FALSE if one did not stop in time; it is
   removed regardless, so nothing keeps running against the caller's memory. */
static BOOL stop_tasks(struct MapWorker *w, struct Task **t, int n)
{
    BOOL ok = TRUE;
    int i;

    for (i = 0; i < n; i++)
        w[i].w_Stop = 1;
    for (i = 0; i < n; i++)
        if (t[i] && !wait_flag(&w[i].w_Done, WAIT_TICKS)
            && !wait_flag(&w[i].w_Done, WAIT_TICKS))
        {
            /* A second window before forcing RemTask(): a churn worker removed
               inside KrnMapGlobal()/KrnUnmapGlobal() leaves the Forbid() nest
               unbalanced and its page-table pool leaked. If this window also
               expires, both are knowingly abandoned: the run has failed. */
            ok = FALSE;
        }

    Forbid();
    for (i = 0; i < n; i++)
        if (t[i])
            RemTask(t[i]);
    Permit();

    return ok;
}

static void fill_pattern(UBYTE *addr, ULONG len, UBYTE seed)
{
    volatile UBYTE *p = addr;
    ULONG i;

    for (i = 0; i < len; i++)
        p[i] = (UBYTE)(seed + i);
}

static int check_pattern(UBYTE *addr, ULONG len, UBYTE seed)
{
    volatile UBYTE *p = addr;
    ULONG i;

    for (i = 0; i < len; i++)
        if (p[i] != (UBYTE)(seed + i))
            return 0;
    return 1;
}

static UBYTE *align_2m(UBYTE *block)
{
    return (UBYTE *)(((IPTR)block + BIGPAGE - 1) & ~(IPTR)(BIGPAGE - 1));
}

/******************************************************************************/
/*  Phase 1: map one 4 KiB page inside a live 2 MiB mapping                   */
/******************************************************************************/

/* Reads the first and last page of the 2 MiB region until told to stop. */
static void NeighbourReader(void)
{
    struct MapWorker *w = myworker();
    int i;

    w->w_Started = 1;

    while (!w->w_Stop)
    {
        for (i = 0; i < READER_ITERS; i++)
        {
            if (!check_pattern(w->w_Base, PAGE, 0x11) ||
                !check_pattern(w->w_Base + BIGPAGE - PAGE, PAGE, 0x22))
            {
                w->w_Errors++;
                goto done;
            }
            w->w_Progress++;
        }
    }

done:
    w->w_Done = 1;
    Wait(0);
}

/* 1 pass, 0 fail, -1 invalid (could not run). */
static int TestSplit2M(void)
{
    struct Task *t = NULL;
    UBYTE *block, *big, *target;
    int cpu, ok = 1;
    BOOL stopped = TRUE;

    if (g_NumCPUs < 2)
    {
        bug("[smpmmap] phase 1: SKIP (one CPU)\n");
        return 1;
    }

    block = AllocMem(BLOCKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!block)
    {
        bug("[smpmmap] phase 1: INVALID (could not allocate %d bytes)\n", BLOCKSIZE);
        return -1;
    }
    big = align_2m(block);
    target = big + SPLIT_OFFSET;

    fill_pattern(big, PAGE, 0x11);
    fill_pattern(big + BIGPAGE - PAGE, PAGE, 0x22);
    fill_pattern(target, PAGE, 0x33);

    memset(&g_Reader, 0, sizeof(g_Reader));
    g_Reader.w_Base = big;

    /* The reader runs on a CPU other than ours. */
    cpu = (KrnGetCPUNumber() + 1) % g_NumCPUs;

    bug("[smpmmap] phase 1: mapping 0x%p inside the 2 MiB region at 0x%p, cpu %d reading\n",
        target, big, cpu);

    t = spawn("smpmmap.reader", (APTR)NeighbourReader, cpu, &g_Reader);
    if (!t || !wait_flag(&g_Reader.w_Started, WAIT_TICKS))
    {
        bug("[smpmmap] phase 1: INVALID (reader did not start)\n");
        ok = -1;
        goto out;
    }

    /* KrnMapGlobal(): non-zero on success, 0 on failure, on every port */
    if (!KrnMapGlobal(target, target, PAGE, MAP_FLAGS))
    {
        bug("[smpmmap] phase 1: *** FAIL *** KrnMapGlobal of a 4 KiB page "
            "inside a 2 MiB mapping returned failure\n");
        ok = 0;
    }
    if (!check_pattern(target, PAGE, 0x33))
    {
        bug("[smpmmap] phase 1: *** FAIL *** the mapped page lost its contents\n");
        ok = 0;
    }
    if (!check_pattern(big, PAGE, 0x11))
    {
        bug("[smpmmap] phase 1: *** FAIL *** the page before it lost its contents\n");
        ok = 0;
    }
    if (!check_pattern(big + BIGPAGE - PAGE, PAGE, 0x22))
    {
        bug("[smpmmap] phase 1: *** FAIL *** the page after it lost its contents\n");
        ok = 0;
    }

out:
    if (t)
        stopped = stop_tasks(&g_Reader, &t, 1);
    if (t && !stopped && ok >= 0)
    {
        bug("[smpmmap] phase 1: *** FAIL *** reader never finished\n");
        ok = 0;
    }
    else if (ok > 0 && g_Reader.w_Errors)
    {
        bug("[smpmmap] phase 1: *** FAIL *** reader saw %lu bad reads (%lu sweeps)\n",
            (unsigned long)g_Reader.w_Errors, (unsigned long)g_Reader.w_Progress);
        ok = 0;
    }

    /* The target is mapped back to itself, so the block returns as it came.
       Keep it while a task may still be reading it, as release_block() keeps
       a block whose slice could not be mapped back. */
    if (stopped)
        FreeMem(block, BLOCKSIZE);
    else
        bug("[smpmmap] phase 1: block 0x%p not freed: a task may still read it\n",
            block);

    if (ok > 0)
        bug("[smpmmap] phase 1: OK (%lu reader sweeps)\n",
            (unsigned long)g_Reader.w_Progress);

    return ok;
}

/******************************************************************************/
/*  Phase 2: two CPUs unmapping and remapping in a loop                       */
/******************************************************************************/

static void MapChurn(void)
{
    struct MapWorker *w = myworker();
    int i, k;

    w->w_Started = 1;

    for (i = 0; i < MAP_LOOPS && !w->w_Stop; i++)
    {
        for (k = 0; k < MAP_SLOTS; k++)
        {
            UBYTE *addr = w->w_Base + (IPTR)k * PAGE;
            UBYTE seed = (UBYTE)(w->w_Index * 32 + k);

            KrnUnmapGlobal(addr, PAGE);

            if (!KrnMapGlobal(addr, addr, PAGE, MAP_FLAGS))
            {
                w->w_Errors++;
                continue;
            }

            fill_pattern(addr, PAGE, seed);
            if (!check_pattern(addr, PAGE, seed))
            {
                w->w_Errors++;
                continue;
            }

            w->w_Count++;
            w->w_Progress++;
        }
    }

    w->w_Done = 1;
    Wait(0);
}

/* Map a worker's slice back to itself. FALSE if that failed, in which case
   the block must not go back to exec's pool. */
static BOOL restore_slice(UBYTE *base)
{
    return KrnMapGlobal(base, base, MAP_SLOTS * PAGE, MAP_FLAGS) != 0;
}

/* Restore both slices, then free the block; keep it if a slice cannot be
   restored. FALSE in that case. A NULL s1 is a block with one slice. */
static BOOL release_block(UBYTE *block, UBYTE *s0, UBYTE *s1)
{
    BOOL ok = restore_slice(s0);

    if (s1 && !restore_slice(s1))
        ok = FALSE;
    if (ok)
        FreeMem(block, BLOCKSIZE);
    else
        bug("[smpmmap] phase 2: block 0x%p not freed: a page could not be mapped back\n",
            block);
    return ok;
}

/*
 * Worker i churns [base_i, base_i + MAP_SLOTS * PAGE) on CPU i. The two slices
 * never share a page; whether they share a 2 MiB entry is up to the caller.
 * *stopped is FALSE if a worker was removed without confirming w_Done: the
 * caller must then not hand the block back.
 * 1 pass, 0 fail, -1 invalid.
 */
static int churn_variant(const char *label, UBYTE *base0, UBYTE *base1,
                         BOOL *stopped)
{
    struct Task *t[2] = { NULL, NULL };
    ULONG before, after;
    int i, ok = 1;

    bug("[smpmmap] phase 2 (%s): two CPUs unmapping/remapping %d pages x %d loops"
        " at 0x%p and 0x%p\n", label, MAP_SLOTS, MAP_LOOPS, base0, base1);

    memset(g_Churn, 0, sizeof(g_Churn));
    g_Churn[0].w_Base = base0;
    g_Churn[1].w_Base = base1;

    for (i = 0; i < 2; i++)
    {
        g_Churn[i].w_Index = i;
        t[i] = spawn("smpmmap.churn", (APTR)MapChurn, i, &g_Churn[i]);
        if (!t[i] || !wait_flag(&g_Churn[i].w_Started, WAIT_TICKS))
        {
            bug("[smpmmap] phase 2 (%s): INVALID (worker %d did not start)\n", label, i);
            if (!stop_tasks(g_Churn, t, i + 1))
                *stopped = FALSE;
            return -1;
        }
    }

    before = g_Churn[0].w_Progress + g_Churn[1].w_Progress;
    Delay(50);
    after = g_Churn[0].w_Progress + g_Churn[1].w_Progress;

    /* No progress only means a deadlock if neither worker has finished. */
    if (after == before && !g_Churn[0].w_Done && !g_Churn[1].w_Done)
    {
        bug("[smpmmap] phase 2 (%s): *** FAIL *** neither worker progressed\n", label);
        ok = 0;
    }

    if (!stop_tasks(g_Churn, t, 2))
    {
        *stopped = FALSE;
        bug("[smpmmap] phase 2 (%s): *** FAIL *** a worker never finished\n", label);
        ok = 0;
    }

    bug("[smpmmap] phase 2 (%s): worker 0: %lu maps, %lu errors; "
        "worker 1: %lu maps, %lu errors\n", label,
        (unsigned long)g_Churn[0].w_Count, (unsigned long)g_Churn[0].w_Errors,
        (unsigned long)g_Churn[1].w_Count, (unsigned long)g_Churn[1].w_Errors);

    if (g_Churn[0].w_Errors || g_Churn[1].w_Errors)
    {
        bug("[smpmmap] phase 2 (%s): *** FAIL *** a page failed to map or read back\n",
            label);
        ok = 0;
    }

    if (ok > 0)
        bug("[smpmmap] phase 2 (%s): OK\n", label);

    return ok;
}

/*
 * Two variants. In "shared" both slices sit in one 2 MiB region, so the
 * first map on either CPU splits the entry the other CPU is walking. In
 * "separate" each slice has its own region, as a control.
 */
static int TestConcurrentMap(void)
{
    UBYTE *block, *base, *block0, *block1;
    int r, result;
    BOOL stopped = TRUE;

    if (g_NumCPUs < 2)
    {
        bug("[smpmmap] phase 2: SKIP (one CPU)\n");
        return 1;
    }

    block = AllocMem(BLOCKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!block)
    {
        bug("[smpmmap] phase 2: INVALID (could not allocate %d bytes)\n", BLOCKSIZE);
        return -1;
    }
    base = align_2m(block);

    result = churn_variant("shared 2 MiB", base, base + MAP_SLOTS * PAGE,
                           &stopped);
    if (stopped)
    {
        if (!release_block(block, base, base + MAP_SLOTS * PAGE) && result > 0)
            result = 0;
    }
    else
    {
        bug("[smpmmap] phase 2: block 0x%p not freed: a worker may still read it\n",
            block);
        if (result > 0)
            result = 0;
    }
    if (result <= 0)
        return result;

    block0 = AllocMem(BLOCKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    block1 = AllocMem(BLOCKSIZE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!block0 || !block1)
    {
        bug("[smpmmap] phase 2: INVALID (could not allocate %d bytes)\n", BLOCKSIZE);
        if (block0)
            FreeMem(block0, BLOCKSIZE);
        if (block1)
            FreeMem(block1, BLOCKSIZE);
        return -1;
    }

    stopped = TRUE;
    r = churn_variant("separate 2 MiB", align_2m(block0), align_2m(block1),
                      &stopped);
    if (stopped)
    {
        if (!release_block(block0, align_2m(block0), NULL) && r > 0)
            r = 0;
        if (!release_block(block1, align_2m(block1), NULL) && r > 0)
            r = 0;
    }
    else
    {
        bug("[smpmmap] phase 2: block 0x%p not freed: a worker may still read it\n",
            block0);
        bug("[smpmmap] phase 2: block 0x%p not freed: a worker may still read it\n",
            block1);
        if (r > 0)
            r = 0;
    }

    return r;
}

int main(void)
{
    int pass = 0, fail = 0, inval = 0;
    int r;

    KernelBase = OpenResource((CONST_STRPTR)"kernel.resource");
    if (!KernelBase)
    {
        bug("[smpmmap] INVALID (kernel.resource not available)\n");
        bug("[smpmmap] DONE: 0 PASS, 0 FAIL, 2 INVALID\n");
        return RETURN_FAIL;
    }
    g_NumCPUs = KrnGetCPUCount();

    bug("[smpmmap] start: cpus=%d\n", g_NumCPUs);

    r = TestSplit2M();          if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestConcurrentMap();    if (r > 0) pass++; else if (!r) fail++; else inval++;

    bug("[smpmmap] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, inval);
    return (fail || inval) ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
