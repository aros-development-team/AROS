/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: RemTask() of a task that is running on another CPU.

          The other SMP tests only ever remove a task that is waiting.
          Here the victim is busy on its own CPU when another CPU removes
          it. After RemTask() has returned the victim must not run any
          more, and its CPU must still schedule other tasks.

          Check 1: the victim counts in memory that belongs to the test.
          If the count still moves after RemTask() has returned, the
          task was not stopped: it is running on a stack, and with a task
          structure, that have been freed.

          Check 2: a second task is started on the victim's CPU. If it
          never runs, that CPU is stuck with the removed task.
*/

#include <aros/config.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;

#define STACKSIZE   16384
#define WAIT_TICKS  250

struct RunWorker
{
    volatile ULONG w_Started;
    volatile ULONG w_Count;
    volatile ULONG w_Done;
};

static struct RunWorker victim[2], canary;

static void VictimWorker(void)
{
    struct RunWorker *w = (struct RunWorker *)FindTask(NULL)->tc_UserData;

    w->w_Started = 1;
    for (;;)
        w->w_Count++;
}

/* The same, but it never lets go of its CPU by itself */
static void ForbidVictimWorker(void)
{
    struct RunWorker *w = (struct RunWorker *)FindTask(NULL)->tc_UserData;

    Forbid();
    w->w_Started = 1;
    for (;;)
        w->w_Count++;
}

static void CanaryWorker(void)
{
    struct RunWorker *w = (struct RunWorker *)FindTask(NULL)->tc_UserData;

    w->w_Done = 1;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, struct RunWorker *w)
{
    cpumask_t *mask = KrnAllocCPUMask();

    if (mask == NULL)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       0,
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

/* 1 pass, 0 fail, -1 invalid */
static int one(int cpu, int kind)
{
    struct RunWorker *w = &victim[kind];
    struct Task *t;
    ULONG before, after;
    int ok = 1;

    memset(w, 0, sizeof(*w));
    t = spawn(kind ? "remrun.forbid" : "remrun.busy",
              kind ? (APTR)ForbidVictimWorker : (APTR)VictimWorker, cpu, w);
    if (t == NULL || !wait_flag(&w->w_Started, WAIT_TICKS))
    {
        bug("[smpremrun] CPU #%02d: INVALID - victim did not start\n", cpu);
        return -1;
    }
    Delay(5);

    before = w->w_Count;
    Delay(5);
    if (w->w_Count == before)
    {
        bug("[smpremrun] CPU #%02d: INVALID - victim is not running\n", cpu);
        return -1;
    }

    RemTask(t);
    bug("[smpremrun] CPU #%02d: RemTask(%s victim) returned\n", cpu, kind ? "Forbid()" : "busy");

    before = w->w_Count;
    Delay(25);
    after = w->w_Count;
    if (after != before)
    {
        bug("[smpremrun] CPU #%02d: *** FAIL *** victim still runs after RemTask(): "
            "%lu more loops in half a second\n", cpu, (unsigned long)(after - before));
        ok = 0;
    }

    memset(&canary, 0, sizeof(canary));
    if (spawn("remrun.canary", CanaryWorker, cpu, &canary) == NULL ||
        !wait_flag(&canary.w_Done, WAIT_TICKS))
    {
        bug("[smpremrun] CPU #%02d: *** FAIL *** CPU does not run a new task "
            "after RemTask() of its running task\n", cpu);
        ok = 0;
    }

    return ok;
}

int main(void)
{
    int ncpus, cpu, kind, pass = 0, fail = 0, inval = 0;

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpremrun] start: cpus=%ld\n", (LONG)ncpus);
    if (ncpus < 2)
    {
        bug("[smpremrun] only %ld CPU online - needs SMP; SKIP\n", (LONG)ncpus);
        return RETURN_WARN;
    }

    for (kind = 0; kind < 2 && !fail; kind++)
    {
        for (cpu = 1; cpu < ncpus && !fail; cpu++)
        {
            int r = one(cpu, kind);

            if (r > 0)       pass++;
            else if (r == 0) fail++;
            else             inval++;
        }
    }

    bug("[smpremrun] DONE: %ld PASS, %ld FAIL, %ld INVALID\n",
        (LONG)pass, (LONG)fail, (LONG)inval);

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}
#else
int main(void)
{
    return RETURN_WARN;
}
#endif
