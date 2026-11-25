/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: A task that crashes must not take its CPU with it.

          A victim task, bound to another CPU, executes an illegal
          instruction: as it is, inside Disable(), and inside Forbid().
          Afterwards that CPU must still schedule: a second task bound to
          it has to run. With Disable() that also shows the CPU takes
          interrupts again, and with Forbid() that it switches tasks again.

          The victims are left where the system puts them: in the crash
          requester if there is one, suspended if there is not. Nobody
          answers the requesters, so run this test last.
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

#define STACKSIZE   32768
#define WAIT_TICKS  250

#define KIND_PLAIN      0
#define KIND_DISABLE    1
#define KIND_FORBID     2
#define KINDS           3

static const char * const kindname[KINDS] = { "plain", "Disable()", "Forbid()" };

struct CrashWorker
{
    volatile ULONG w_Kind;
    volatile ULONG w_Started;
    volatile ULONG w_Survived;
    volatile ULONG w_Done;
};

static struct CrashWorker victim, canary;

/* An instruction no CPU of the kind executes */
static inline void crash(void)
{
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("ud2");
#elif defined(__aarch64__)
    asm volatile(".word 0x00000000");
#elif defined(__arm__)
    asm volatile(".word 0xe7f000f0");
#elif defined(__riscv)
    asm volatile(".word 0x00000000");
#else
    *(volatile ULONG *)1 = 0;
#endif
}

static void CrashVictim(void)
{
    struct CrashWorker *w = (struct CrashWorker *)FindTask(NULL)->tc_UserData;

    if (w->w_Kind == KIND_DISABLE)
        Disable();
    else if (w->w_Kind == KIND_FORBID)
        Forbid();

    w->w_Started = 1;
    crash();

    /* Only if the user told the requester to carry on */
    w->w_Survived = 1;
    Wait(0);
}

static void CanaryWorker(void)
{
    struct CrashWorker *w = (struct CrashWorker *)FindTask(NULL)->tc_UserData;

    w->w_Done = 1;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, struct CrashWorker *w)
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
    memset(&victim, 0, sizeof(victim));
    victim.w_Kind = kind;

    bug("[smpcrash] CPU #%02d: %s crash\n", cpu, kindname[kind]);

    if (spawn("smpcrash.victim", CrashVictim, cpu, &victim) == NULL ||
        !wait_flag(&victim.w_Started, WAIT_TICKS))
    {
        bug("[smpcrash] CPU #%02d: INVALID - victim did not start\n", cpu);
        return -1;
    }

    /* Time for the crash to be taken and handled */
    Delay(50);

    memset(&canary, 0, sizeof(canary));
    if (spawn("smpcrash.canary", CanaryWorker, cpu, &canary) == NULL ||
        !wait_flag(&canary.w_Done, WAIT_TICKS))
    {
        bug("[smpcrash] CPU #%02d: *** FAIL *** CPU does not run a new task "
            "after a %s crash on it\n", cpu, kindname[kind]);
        return 0;
    }

    return 1;
}

int main(void)
{
    int ncpus, i, kind, pass = 0, fail = 0, inval = 0;
    int cpus[2];

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpcrash] start: cpus=%ld\n", (LONG)ncpus);
    if (ncpus < 2)
    {
        bug("[smpcrash] only %ld CPU online - needs SMP; SKIP\n", (LONG)ncpus);
        return RETURN_WARN;
    }

    /* Every crash leaves a task behind, and a requester where there is one: two CPUs are enough */
    cpus[0] = 1;
    cpus[1] = ncpus - 1;

    for (kind = 0; kind < KINDS && !fail; kind++)
    {
        for (i = 0; i < 2 && !fail; i++)
        {
            int r;

            if (i && (cpus[1] == cpus[0]))
                break;

            r = one(cpus[i], kind);
            if (r > 0)       pass++;
            else if (r == 0) fail++;
            else             inval++;
        }
    }

    bug("[smpcrash] DONE: %ld PASS, %ld FAIL, %ld INVALID\n",
        (LONG)pass, (LONG)fail, (LONG)inval);

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}
#else
int main(void)
{
    return RETURN_WARN;
}
#endif
