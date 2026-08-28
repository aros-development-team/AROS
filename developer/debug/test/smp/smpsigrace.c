/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Cross-CPU Signal() vs RemTask() lifetime race check.

          The cross-CPU Signal() path queues an IPI entry carrying a raw
          Task pointer on the target CPU. If the task is torn down and its
          memory freed before that CPU drains its queue, signal_hook spins
          on a freed (mungwall-poisoned) tc_SpinLock inside the FIQ handler
          and the CPU wedges. This test makes that window deterministic:

          Phase 0: functional check - a cross-CPU Signal() must still wake
          a waiting task (guards against lost wakeups in the fixed path).

          Phase 1: a "blocker" task holds Disable() on the target CPU (FIQ
          masked, so the IPI queue cannot drain) while the main task sends
          a Signal() to a task parked there and immediately RemTask()s it,
          freeing its memory. On a kernel without IPI cancellation the
          stale entry is then guaranteed to be drained AFTER the free: the
          blocker's Enable() delivers the pending FIQ straight into the
          use-after-free and never returns (run with mungwall to make the
          freed lock word poisoned). On a fixed kernel RemTask() sweeps
          the entry and the blocker finishes - its completion is the
          pass/fail signal.

          Phase 3: pool-exhaustion deadlock check. The per-CPU IPI entry
          pools are finite; a sender whose target pool is empty spins for
          a free entry, and under Disable() its own CPU cannot drain its
          inbound queue (FIQ masked). Two Disable()-d CPUs storming more
          signals than the pool size at tasks on each other's CPU
          therefore deadlocked - each waited for a drain only the other
          could trigger. The kernel now drains its own inbound queue
          inline while spinning Disabled; without that both storm workers
          hang forever with interrupts masked.

          Phase 2: best-effort storm against the suicide teardown path -
          signals are fired at a worker that exits (by itself) right
          afterwards, then a canary confirms the target CPU still
          schedules. All signals are sent before the worker is released to
          exit, so the test never violates the "task pointer valid during
          the Signal() call" contract - it only exercises the queue
          extending the pointer's life beyond the call.
*/

#include <aros/config.h>

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

#define STACKSIZE       16384
#define DISABLE_SPIN    2000000     /* blocker's Disable() window */
#define BUSY_TIMEOUT    400000000   /* bound for busy flag waits */
#define WAIT_TICKS      250         /* 5s bound for Delay-based waits */
#define PHASE1_ITERS    8           /* per secondary CPU */
#define PHASE2_ITERS    128
#define PHASE2_SIGNALS  32
#define PHASE3_SIGNALS  200         /* > the per-CPU IPI pool (128) */

struct SigWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_InDisable;
    volatile ULONG  w_Go;
    volatile ULONG  w_Dying;
    volatile ULONG  w_Done;
    volatile ULONG  w_Spin;
    struct Task * volatile w_Target;
};

static volatile ULONG g3_Go;

static struct SigWorker *myworker(void)
{
    return (struct SigWorker *)FindTask(NULL)->tc_UserData;
}

/* Phase 0: prove a cross-CPU Signal() still wakes a waiter. */
static void WakeWorker(void)
{
    struct SigWorker *w = myworker();

    w->w_Started = 1;
    Wait(SIGBREAKF_CTRL_C);
    w->w_Done = 1;
    Wait(0);
}

/* Phase 1 victim: park forever; only ever woken by RemTask(). */
static void ParkWorker(void)
{
    struct SigWorker *w = myworker();

    w->w_Started = 1;
    Wait(0);
}

/* Phase 1 blocker: hold Disable() (FIQ masked -> IPI queue frozen) long
 * enough for the main task to Signal()+RemTask() the parked victim. The
 * Enable() then delivers the pending Signal-IPI: on a broken kernel
 * signal_hook spins on the freed task's lock and w_Done is never set. */
static void BlockerWorker(void)
{
    struct SigWorker *w = myworker();
    ULONG n;

    Disable();
    w->w_InDisable = 1;
    __sync_synchronize();
    for (n = 0; n < DISABLE_SPIN; n++)
        w->w_Spin++;
    Enable();

    w->w_Done = 1;
}

/* Phase 2 victim: spin until released, then exit (suicide RemTask path). */
static void ExitWorker(void)
{
    struct SigWorker *w = myworker();

    w->w_Started = 1;
    while (!w->w_Go)
        w->w_Spin++;
    w->w_Dying = 1;
}

/* Phase 3: storm more cross-CPU signals than the IPI pool holds, under
 * Disable() so this CPU's own inbound queue cannot drain via FIQ. Two of
 * these on different CPUs, targeting tasks on each other's CPU, exhaust
 * both pools simultaneously. */
static void StormWorker(void)
{
    struct SigWorker *w = myworker();
    int k;

    w->w_Started = 1;
    while (!g3_Go)
        w->w_Spin++;

    Disable();
    for (k = 0; k < PHASE3_SIGNALS; k++)
        Signal(w->w_Target, SIGBREAKF_CTRL_C);
    Enable();

    w->w_Done = 1;
}

/* Phase 2 canary: proves its CPU still schedules tasks. */
static void CanaryWorker(void)
{
    struct SigWorker *w = myworker();

    w->w_Done = 1;
}

static struct Task *spawn(const char *name, APTR pc, LONG pri, int cpu,
                          struct SigWorker *w)
{
    cpumask_t *mask = KrnAllocCPUMask();

    if (mask == NULL)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    /* The mask becomes iet_CpuAffinity and is freed by exec at teardown. */
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       pri,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TASKTAG_AFFINITY,  (IPTR)mask,
                         TAG_DONE);
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

/* No Delay() here: the gap between seeing the flag and acting must stay
 * well inside the blocker's Disable() window. */
static int busy_wait_flag(volatile ULONG *flag)
{
    ULONG i;

    for (i = 0; i < BUSY_TIMEOUT && !*flag; i++)
        ;
    return *flag != 0;
}

static int phase0_one(int cpu)
{
    struct SigWorker wake;
    struct Task *t;
    static char name[32];

    memset(&wake, 0, sizeof(wake));
    snprintf(name, sizeof(name), "sigrace.wake.%d", cpu);

    t = spawn(name, WakeWorker, 0, cpu, &wake);
    if (t == NULL || !wait_flag(&wake.w_Started, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d: INVALID - wake worker did not start\n", cpu);
        return -1;
    }
    Delay(1);   /* let it reach Wait() */

    Signal(t, SIGBREAKF_CTRL_C);

    if (!wait_flag(&wake.w_Done, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d: *** FAIL *** cross-CPU Signal() lost - "
            "waiter never woke\n", cpu);
        return 0;
    }
    RemTask(t);
    return 1;
}

static int phase1_one(int cpu, int iter)
{
    struct SigWorker park, blk;
    struct Task *tpark, *tblk;
    static char pname[32], bname[32];

    memset(&park, 0, sizeof(park));
    memset(&blk, 0, sizeof(blk));
    snprintf(pname, sizeof(pname), "sigrace.park.%d.%d", cpu, iter);
    snprintf(bname, sizeof(bname), "sigrace.blk.%d.%d", cpu, iter);

    tpark = spawn(pname, ParkWorker, 0, cpu, &park);
    if (tpark == NULL || !wait_flag(&park.w_Started, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d: INVALID - park worker did not start\n", cpu);
        return -1;
    }
    Delay(1);   /* let it reach Wait(0) */

    tblk = spawn(bname, BlockerWorker, 5, cpu, &blk);
    if (tblk == NULL)
    {
        RemTask(tpark);
        return -1;
    }

    if (!busy_wait_flag(&blk.w_InDisable))
    {
        bug("[smpsigrace] CPU #%02d: INVALID - blocker never entered Disable()\n", cpu);
        RemTask(tpark);
        return -1;
    }

    /*
     * The target CPU's FIQ is masked: this entry stays queued there...
     */
    Signal(tpark, SIGBREAKF_CTRL_C);
    /*
     * ...and this frees the task's memory. Without cancellation the queue
     * now holds a dangling Task pointer that WILL be drained on Enable().
     */
    RemTask(tpark);

    if (!wait_flag(&blk.w_Done, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d iter %d: *** FAIL *** blocker never returned - "
            "CPU wedged draining a stale Signal-IPI after the task was freed\n",
            cpu, iter);
        return 0;
    }
    return 1;
}

static int phase2_one(int cpu, int iter)
{
    struct SigWorker w, canary;
    struct Task *t, *tc;
    static char wname[32], cname[32];
    int k;

    memset(&w, 0, sizeof(w));
    memset(&canary, 0, sizeof(canary));
    snprintf(wname, sizeof(wname), "sigrace.exit.%d.%d", cpu, iter);
    snprintf(cname, sizeof(cname), "sigrace.cnry.%d.%d", cpu, iter);

    t = spawn(wname, ExitWorker, 0, cpu, &w);
    if (t == NULL || !wait_flag(&w.w_Started, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d: INVALID - exit worker did not start\n", cpu);
        return -1;
    }

    /* All signals go out while the worker provably lives (it spins until
     * w_Go). Only the queued entries race its exit + teardown. */
    for (k = 0; k < PHASE2_SIGNALS; k++)
        Signal(t, SIGBREAKF_CTRL_C);
    w.w_Go = 1;
    __sync_synchronize();
    /* From here on `t` is never touched again. */

    if (!wait_flag(&w.w_Dying, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d: INVALID - exit worker never exited\n", cpu);
        return -1;
    }
    Delay(1);   /* let the service task free it while the queue drains */

    tc = spawn(cname, CanaryWorker, 0, cpu, &canary);
    if (tc == NULL || !wait_flag(&canary.w_Done, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d iter %d: *** FAIL *** canary never ran - "
            "CPU wedged after signal storm on an exiting task\n", cpu, iter);
        return 0;
    }
    return 1;
}

/* Cross-Disable signal storm between cpuA and cpuB. */
static int phase3_one(int cpuA, int cpuB)
{
    struct SigWorker parkA, parkB, stormA, stormB;
    struct Task *tparkA, *tparkB, *tstormA, *tstormB;
    static char names[4][32];

    memset(&parkA, 0, sizeof(parkA));
    memset(&parkB, 0, sizeof(parkB));
    memset(&stormA, 0, sizeof(stormA));
    memset(&stormB, 0, sizeof(stormB));
    g3_Go = 0;

    snprintf(names[0], sizeof(names[0]), "sigrace.p3park.%d", cpuA);
    snprintf(names[1], sizeof(names[1]), "sigrace.p3park.%d", cpuB);
    snprintf(names[2], sizeof(names[2]), "sigrace.p3storm.%d", cpuA);
    snprintf(names[3], sizeof(names[3]), "sigrace.p3storm.%d", cpuB);

    tparkA = spawn(names[0], ParkWorker, 0, cpuA, &parkA);
    tparkB = spawn(names[1], ParkWorker, 0, cpuB, &parkB);
    if (!tparkA || !tparkB ||
        !wait_flag(&parkA.w_Started, WAIT_TICKS) ||
        !wait_flag(&parkB.w_Started, WAIT_TICKS))
    {
        bug("[smpsigrace] phase 3: INVALID - park workers did not start\n");
        return -1;
    }
    Delay(1);

    /* Each storm targets the task parked on the OTHER storm's CPU. */
    stormA.w_Target = tparkB;
    stormB.w_Target = tparkA;
    tstormA = spawn(names[2], StormWorker, 0, cpuA, &stormA);
    tstormB = spawn(names[3], StormWorker, 0, cpuB, &stormB);
    if (!tstormA || !tstormB ||
        !wait_flag(&stormA.w_Started, WAIT_TICKS) ||
        !wait_flag(&stormB.w_Started, WAIT_TICKS))
    {
        bug("[smpsigrace] phase 3: INVALID - storm workers did not start\n");
        return -1;
    }

    g3_Go = 1;
    __sync_synchronize();

    if (!wait_flag(&stormA.w_Done, WAIT_TICKS) ||
        !wait_flag(&stormB.w_Done, WAIT_TICKS))
    {
        bug("[smpsigrace] CPU #%02d/#%02d: *** FAIL *** cross-Disable signal "
            "storm never completed - IPI pool exhaustion deadlock\n",
            cpuA, cpuB);
        return 0;
    }

    /* Leftover queued entries for the victims are swept here. */
    RemTask(tparkA);
    RemTask(tparkB);
    return 1;
}

int main(void)
{
    int ncpus, cpu, iter;
    int pass = 0, fail = 0, inval = 0;

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpsigrace] start: cpus=%ld (mungwall recommended to expose the pre-fix UAF)\n",
        (LONG)ncpus);

    if (ncpus < 2)
    {
        bug("[smpsigrace] only %ld CPU online - needs SMP; SKIP\n", (LONG)ncpus);
        return RETURN_WARN;
    }

    for (cpu = 1; cpu < ncpus; cpu++)
    {
        int r = phase0_one(cpu);
        if (r > 0)       pass++;
        else if (r == 0) fail++;
        else             inval++;
    }
    bug("[smpsigrace] phase 0 (cross-CPU wake) done\n");

    for (cpu = 1; cpu < ncpus && !fail; cpu++)
    {
        for (iter = 0; iter < PHASE1_ITERS; iter++)
        {
            int r = phase1_one(cpu, iter);
            if (r > 0)       pass++;
            else if (r == 0) { fail++; break; }
            else             inval++;
        }
    }
    bug("[smpsigrace] phase 1 (Signal vs foreign RemTask, drain frozen) done\n");

    for (iter = 0; iter < PHASE2_ITERS && !fail; iter++)
    {
        int r = phase2_one(1 + (iter % (ncpus - 1)), iter);
        if (r > 0)       pass++;
        else if (r == 0) fail++;
        else             inval++;
    }
    bug("[smpsigrace] phase 2 (signal storm vs suicide exit) done\n");

    if (ncpus >= 3 && !fail)
    {
        for (cpu = 1; cpu + 1 < ncpus && !fail; cpu += 2)
        {
            int r = phase3_one(cpu, cpu + 1);
            if (r > 0)       pass++;
            else if (r == 0) fail++;
            else             inval++;
        }
        bug("[smpsigrace] phase 3 (cross-Disable pool-exhaustion storm) done\n");
    }

    bug("[smpsigrace] DONE: %ld PASS, %ld FAIL, %ld INVALID\n",
        (LONG)pass, (LONG)fail, (LONG)inval);

    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif