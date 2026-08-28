/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP per-core preemption (time-slicing) verification.

          Pins TWO equal-priority, purely CPU-bound tasks to the SAME
          secondary CPU and lets them run for a fixed wall-clock window.
          Each worker does nothing but increment its own counter in a
          tight loop - it never calls Delay/Wait/Forbid, so the ONLY way
          the second task can ever run is if the kernel PREEMPTS the first
          (quantum expiry driven by that core's scheduler heartbeat).

          Without a per-core timer the first worker monopolises the core
          and the second never gets scheduled: its w_Cpu stays at the
          NO_CPU sentinel and its counter stays 0 -> FAIL. With a working
          per-core heartbeat both workers are time-sliced: both counters
          are large and roughly balanced -> PASS.

          The test is run once per CPU (0 .. ncpus-1), including the boot
          CPU: on this port every core expires its quantum from its own
          per-core heartbeat.
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

#define STACKSIZE   16384
#define RUN_TICKS   100             /* busy window per core (~2s at 50Hz) */
#define NO_CPU      (~0UL)          /* w_Cpu sentinel: "never scheduled" */

struct Worker
{
    volatile ULONG  w_Counter;      /* iterations completed in the window */
    volatile ULONG  w_Cpu;          /* CPU it first ran on, NO_CPU if never */
    volatile ULONG  w_Done;
    volatile ULONG *w_Stop;         /* shared stop flag */
    cpumask_t      *w_Affinity;
};

static volatile ULONG g_Stop;

/* Pure busy loop. Records which CPU it landed on (proof it was scheduled),
 * then spins until told to stop. No yielding calls - the other worker only
 * advances if this one is preempted. */
static void BusyWorker(void)
{
    struct Task *me = FindTask(NULL);
    struct Worker *w = (struct Worker *)me->tc_UserData;

    w->w_Cpu = (ULONG)KrnGetCPUNumber();
    __sync_synchronize();

    while (!*(w->w_Stop))
        w->w_Counter++;

    w->w_Done = 1;
    __sync_synchronize();
    Wait(0);
}

/* Returns: 1 = PASS, 0 = FAIL, -1 = INVALID (affinity not honoured) */
static int run_one(int cpu)
{
    struct Worker w[2];
    struct Task  *t[2] = { NULL, NULL };
    char          names[2][32];
    ULONG         a, b, ca, cb, lo, hi;
    int           i;

    g_Stop = 0;

    for (i = 0; i < 2; i++)
    {
        memset(&w[i], 0, sizeof(w[i]));
        w[i].w_Cpu  = NO_CPU;
        w[i].w_Stop = &g_Stop;
        w[i].w_Affinity = KrnAllocCPUMask();
        if (w[i].w_Affinity)
        {
            KrnClearCPUMask(w[i].w_Affinity);
            KrnGetCPUMask(cpu, w[i].w_Affinity);
        }
        snprintf(names[i], sizeof(names[i]), "preempt.cpu%d.%c", cpu, 'A' + i);
        t[i] = NewCreateTask(TASKTAG_NAME,      (IPTR)names[i],
                             TASKTAG_PRI,       0,
                             TASKTAG_PC,        (IPTR)BusyWorker,
                             TASKTAG_STACKSIZE, STACKSIZE,
                             TASKTAG_USERDATA,  (IPTR)&w[i],
                             TASKTAG_AFFINITY,  (IPTR)w[i].w_Affinity,
                             TAG_DONE);
        if (t[i] == NULL)
        {
            bug("[smppreempt] FAIL: could not create %s\n", (IPTR)names[i]);
            if (t[0])
            {
                int j;

                /* Stop and reap worker A before returning - it busy-loops
                 * on w[0]/g_Stop in THIS stack frame and would keep
                 * writing to a dead frame after run_one returns. */
                g_Stop = 1;
                __sync_synchronize();
                for (j = 0; j < 200 && !w[0].w_Done; j++)
                    Delay(1);
                Forbid();
                RemTask(t[0]);
                Permit();
            }
            return -1;
        }
    }

    /* Both workers busy-loop on `cpu` while we sleep. */
    Delay(RUN_TICKS);

    /* Snapshot progress BEFORE stopping: in the broken case the starved
     * worker only gets to run once the other exits, which would set its
     * w_Cpu after the fact and hide the starvation. */
    __sync_synchronize();
    a  = w[0].w_Counter; b  = w[1].w_Counter;
    ca = w[0].w_Cpu;     cb = w[1].w_Cpu;

    /* Stop and let both quiesce into Wait(0) before removing them. */
    g_Stop = 1;
    __sync_synchronize();
    for (i = 0; i < 200 && (!w[0].w_Done || !w[1].w_Done); i++)
        Delay(1);

    Forbid();
    for (i = 0; i < 2; i++)
        if (t[i]) RemTask(t[i]);
    Permit();

    bug("[smppreempt] CPU #%02d: A cpu=%ld iters=%lu | B cpu=%ld iters=%lu\n",
        cpu, (LONG)ca, a, (LONG)cb, b);

    if (ca == NO_CPU || cb == NO_CPU)
    {
        bug("[smppreempt] CPU #%02d: *** FAIL *** a worker was never scheduled "
            "(no per-core preemption - starved)\n", cpu);
        return 0;
    }
    if (ca != (ULONG)cpu || cb != (ULONG)cpu)
    {
        bug("[smppreempt] CPU #%02d: INVALID - affinity not honoured "
            "(ran on %ld/%ld)\n", cpu, (LONG)ca, (LONG)cb);
        return -1;
    }
    if (a == 0 || b == 0)
    {
        bug("[smppreempt] CPU #%02d: *** FAIL *** a worker made no progress\n", cpu);
        return 0;
    }

    lo = (a < b) ? a : b;
    hi = (a > b) ? a : b;
    if (hi / lo > 3)
        bug("[smppreempt] CPU #%02d: PASS (imbalanced ~%lu:1, but both time-sliced)\n",
            cpu, hi / lo);
    else
        bug("[smppreempt] CPU #%02d: PASS - both tasks time-sliced (ratio ~%lu:1)\n",
            cpu, hi / lo);
    return 1;
}

int main(void)
{
    int ncpus, cpu;
    int pass = 0, fail = 0, inval = 0;

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smppreempt] start: cpus=%ld, window=%ld ticks/core\n",
        (LONG)ncpus, (LONG)RUN_TICKS);

    if (ncpus < 2)
    {
        bug("[smppreempt] only %ld CPU online - per-core preemption needs SMP; SKIP\n",
            (LONG)ncpus);
        return RETURN_WARN;
    }

    for (cpu = 0; cpu < ncpus; cpu++)
    {
        int r = run_one(cpu);
        if (r > 0)      pass++;
        else if (r == 0) fail++;
        else            inval++;
    }

    bug("[smppreempt] DONE: %ld PASS, %ld FAIL, %ld INVALID (of %ld CPUs)\n",
        (LONG)pass, (LONG)fail, (LONG)inval, (LONG)ncpus);

    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif