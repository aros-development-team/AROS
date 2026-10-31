/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Idle task.
*/

#define DEBUG 0

#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/rawfmt.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <asm/cpu.h>

/* kernel and exec share one core image */
#include "kernel_riscv.h"

#if defined(__AROSEXEC_SMP__)
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include "exec_intern.h"
#endif

/*
 * One task that is always runnable, so the dispatcher never has to idle
 * inside itself when every other task is waiting. A trap taken in the
 * dispatcher cannot reschedule from within the very function about to do
 * so, and has to return early; that also skips core_ExitInterrupt(), and
 * with it the soft interrupt carrying an I/O completion. Halting as a task
 * keeps the CPU out of the dispatcher, so such a trap returns the ordinary
 * way and the completion is delivered at once.
 */
void riscv64IdleTask(struct ExecBase *SysBase)
{
    D(bug("[Exec] riscv64: '%s' running\n", FindTask(NULL)->tc_Node.ln_Name));

    for (;;)
    {
        /*
         * Wait for the next trap, counting the wait as this CPU's idle
         * time (the load figure is made from it). The trap is taken on
         * the way out. A spurious wakeup is harmless: the reschedule
         * below just finds nothing new and comes back here.
         */
        krnRVIdleWait();

        Reschedule();
    }
}

/*
 * Create the idle task of the CPU this runs on, pinned there: the boot
 * CPU's from the exec init below, each secondary hart's from its
 * bootstrap task (kernel_cpusmp.c), which then removes itself.
 */
struct Task *Exec_RVCreateIdleTask(struct ExecBase *SysBase)
{
    struct Task *idle;
#if defined(__AROSEXEC_SMP__)
    uint32_t cpunum = KrnGetCPUNumber();
    IPTR nameArg[] = { cpunum };
    struct MemList *ml;
    char *name;
    void *aff = KrnAllocCPUMask();

    if (!aff)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate the idle task's CPU mask\n", cpunum);
        return NULL;
    }
    KrnGetCPUMask(cpunum, aff);

    /* The name goes with the task, in its MemEntry list */
    if ((ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate the idle task's name\n", cpunum);
        KrnFreeCPUMask(aff);
        return NULL;
    }
    ml->ml_NumEntries      = 1;
    ml->ml_ME[0].me_Length = 16;
    if ((ml->ml_ME[0].me_Addr = AllocMem(16, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:%02u] FATAL : Failed to allocate the idle task's name\n", cpunum);
        FreeMem(ml, sizeof(struct MemList));
        KrnFreeCPUMask(aff);
        return NULL;
    }
    name = ml->ml_ME[0].me_Addr;
    RawDoFmt("CPU #%02u Idle", (RAWARG)nameArg, RAWFMTFUNC_STRING, name);
#else
    const char *name = "CPU Idle";
#endif

    idle = NewCreateTask(TASKTAG_NAME, name,
#if defined(__AROSEXEC_SMP__)
                         TASKTAG_AFFINITY, aff,
#endif
                         TASKTAG_PRI,  -128,
                         TASKTAG_PC,   riscv64IdleTask,
                         TASKTAG_ARG1, SysBase,
                         TAG_DONE);

    D(bug("[Exec] riscv64: '%s' @ 0x%p\n", name, idle));

#if defined(__AROSEXEC_SMP__)
    if (idle)
        AddTail(&idle->tc_MemEntry, &ml->ml_Node);
    else
    {
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, sizeof(struct MemList));
    }
#endif

    return idle;
}

static int riscv64Idle_Init(struct ExecBase *SysBase)
{
    return (Exec_RVCreateIdleTask(SysBase) != NULL);
}

ADD2INITLIB(riscv64Idle_Init, -127);
