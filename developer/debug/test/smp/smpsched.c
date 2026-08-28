/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP scheduler coverage the other tests skip: whether unpinned
          tasks migrate between cores, whether waking a high-priority task
          preempts a busy core, and whether SetTaskPri survives being
          aimed at a task running on another core.
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;

#define STACKSIZE       16384
#define WAIT_TICKS      500
#define MIGRATE_TICKS   100         /* observation window, ~2s */
#define SETPRI_LOOPS    400

struct SchedWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_Stop;
    volatile ULONG  w_Done;
    volatile ULONG  w_Count;
    volatile ULONG  w_CpuMask;      /* cores this worker has run on */
    volatile ULONG  w_RanOn;
};

static struct SchedWorker *myworker(void)
{
    return (struct SchedWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn_pinned(const char *name, APTR pc, int cpu, LONG pri,
                                 struct SchedWorker *w)
{
    cpumask_t *mask = KrnAllocCPUMask();

    if (!mask)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       pri,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TASKTAG_AFFINITY,  (IPTR)mask,
                         TAG_DONE);
}

static struct Task *spawn_free(const char *name, APTR pc, LONG pri,
                               struct SchedWorker *w)
{
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       pri,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TAG_DONE);
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

/******************************************************************************/
/*  Phase 1: migration of unpinned busy tasks                                 */
/*                                                                            */
/*  More runnable unpinned workers than cores. All must make progress, and   */
/*  the per-worker record of observed cores shows whether any migrated.      */
/******************************************************************************/

static void BusyTracker(void)
{
    struct SchedWorker *w = myworker();

    w->w_Started = 1;

    while (!w->w_Stop)
    {
        w->w_CpuMask |= 1UL << KrnGetCPUNumber();
        w->w_Count++;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestMigration(void)
{
    struct SchedWorker w[8];
    struct Task *t[8];
    int n = 2 * g_NumCPUs, k, migrated = 0, ok = 1;

    if (n > 8)
        n = 8;

    bug("[smpsched] phase 1: %d unpinned busy workers on %d cores...\n", n, g_NumCPUs);

    memset(w, 0, sizeof(w));
    for (k = 0; k < n; k++)
    {
        t[k] = spawn_free("smpsched.busy", BusyTracker, 0, &w[k]);
        if (!t[k])
        {
            bug("[smpsched] phase 1: INVALID (task create failed)\n");
            return -1;
        }
    }

    Delay(MIGRATE_TICKS);

    for (k = 0; k < n; k++)
        w[k].w_Stop = 1;

    for (k = 0; k < n; k++)
    {
        if (!wait_flag(&w[k].w_Done, WAIT_TICKS))
        {
            bug("[smpsched] phase 1: *** FAIL *** worker %d never stopped\n", k);
            ok = 0;
            break;
        }
    }

    Forbid();
    for (k = 0; k < n; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < n; k++)
    {
        bug("[smpsched] phase 1: worker %d: %lu iters, cpumask %02lx\n",
            k, (unsigned long)w[k].w_Count, (unsigned long)w[k].w_CpuMask);
        if (w[k].w_Count == 0)
        {
            bug("[smpsched] phase 1: *** FAIL *** worker %d starved\n", k);
            return 0;
        }
        if (w[k].w_CpuMask & (w[k].w_CpuMask - 1))
            migrated++;
    }
    bug("[smpsched] phase 1: OK (%d of %d workers ran on more than one core)\n",
        migrated, n);
    return 1;
}

/******************************************************************************/
/*  Phase 2: waking a high-priority task preempts a busy core                 */
/*                                                                            */
/*  Every core spins on priority 0. A priority 10 task is woken by Signal()  */
/*  and must run anyway - if no core gets preempted, it never reports back.  */
/******************************************************************************/

static void PinnedSpinner(void)
{
    struct SchedWorker *w = myworker();

    w->w_Started = 1;
    while (!w->w_Stop)
        w->w_Count++;
    w->w_Done = 1;
    Wait(0);
}

static void HighPrioWaiter(void)
{
    struct SchedWorker *w = myworker();

    w->w_Started = 1;
    Wait(SIGBREAKF_CTRL_C);
    w->w_RanOn = KrnGetCPUNumber();
    w->w_Done = 1;
    Wait(0);
}

static int TestPriorityWake(void)
{
    struct SchedWorker sw[4], hw;
    struct Task *st[4], *ht;
    int k, ok = 1;

    if (g_NumCPUs < 2)
    {
        bug("[smpsched] phase 2: SKIP (needs SMP)\n");
        return 1;
    }

    bug("[smpsched] phase 2: high-priority wake with all %d cores busy...\n",
        g_NumCPUs);

    memset(sw, 0, sizeof(sw));
    memset(&hw, 0, sizeof(hw));

    ht = spawn_free("smpsched.hiprio", HighPrioWaiter, 10, &hw);
    if (!ht || !wait_flag(&hw.w_Started, WAIT_TICKS))
    {
        bug("[smpsched] phase 2: INVALID (waiter did not start)\n");
        return -1;
    }
    Delay(1);   /* let it reach Wait() */

    for (k = 0; k < g_NumCPUs && k < 4; k++)
    {
        st[k] = spawn_pinned("smpsched.spin", PinnedSpinner, k, 0, &sw[k]);
        if (!st[k] || !wait_flag(&sw[k].w_Started, WAIT_TICKS))
        {
            bug("[smpsched] phase 2: INVALID (spinner %d did not start)\n", k);
            return -1;
        }
    }

    /* All cores now run priority-0 spinners. Wake the priority-10 task. */
    Signal(ht, SIGBREAKF_CTRL_C);

    if (!wait_flag(&hw.w_Done, WAIT_TICKS))
    {
        bug("[smpsched] phase 2: *** FAIL *** high-priority task never ran - "
            "no core was preempted for it\n");
        ok = 0;
    }
    else
        bug("[smpsched] phase 2: woke on cpu %lu\n", (unsigned long)hw.w_RanOn);

    for (k = 0; k < g_NumCPUs && k < 4; k++)
        sw[k].w_Stop = 1;
    for (k = 0; k < g_NumCPUs && k < 4; k++)
        wait_flag(&sw[k].w_Done, WAIT_TICKS);

    Forbid();
    for (k = 0; k < g_NumCPUs && k < 4; k++)
        RemTask(st[k]);
    RemTask(ht);
    Permit();

    if (ok)
        bug("[smpsched] phase 2: OK\n");
    return ok;
}

/******************************************************************************/
/*  Phase 3: SetTaskPri aimed at a task running on another core               */
/******************************************************************************/

static int TestCrossSetPri(void)
{
    struct SchedWorker w;
    struct Task *t;
    ULONG before, after;
    int i;

    bug("[smpsched] phase 3: cross-core SetTaskPri while running...\n");

    memset(&w, 0, sizeof(w));
    t = spawn_free("smpsched.pri", BusyTracker, 0, &w);
    if (!t || !wait_flag(&w.w_Started, WAIT_TICKS))
    {
        bug("[smpsched] phase 3: INVALID (worker did not start)\n");
        return -1;
    }

    for (i = 0; i < SETPRI_LOOPS; i++)
    {
        SetTaskPri(t, 5);
        SetTaskPri(t, -5);
        SetTaskPri(t, 0);
    }

    before = w.w_Count;
    Delay(10);
    after = w.w_Count;

    w.w_Stop = 1;
    if (!wait_flag(&w.w_Done, WAIT_TICKS))
    {
        bug("[smpsched] phase 3: *** FAIL *** worker lost after SetTaskPri storm\n");
        return 0;
    }

    Forbid();
    RemTask(t);
    Permit();

    if (after == before)
    {
        bug("[smpsched] phase 3: *** FAIL *** worker stopped progressing\n");
        return 0;
    }
    bug("[smpsched] phase 3: OK\n");
    return 1;
}

int main(void)
{
    int pass = 0, fail = 0, inval = 0;
    int r;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();

    bug("[smpsched] start: cpus=%ld\n", (LONG)g_NumCPUs);

    r = TestMigration();     if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestPriorityWake();  if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestCrossSetPri();   if (r > 0) pass++; else if (!r) fail++; else inval++;

    bug("[smpsched] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, inval);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
