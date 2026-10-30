/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Idle task.
*/

#define DEBUG 0

#include <aros/debug.h>

#include <exec/execbase.h>
#include <exec/tasks.h>
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

static int riscv64Idle_Init(struct ExecBase *SysBase)
{
    struct Task *idle;
#if defined(__AROSEXEC_SMP__)
    /*
     * This is the boot CPU's idle task, so it is pinned there. Each
     * secondary hart idles in its own bootstrap task instead (see
     * kernel_cpusmp.c), which is created on that hart as it comes up.
     */
    void *aff = KrnAllocCPUMask();

    if (!aff)
        return FALSE;
    KrnGetCPUMask(0, aff);
#endif

    idle = NewCreateTask(TASKTAG_NAME, "CPU Idle",
#if defined(__AROSEXEC_SMP__)
                         TASKTAG_AFFINITY, aff,
#endif
                         TASKTAG_PRI,  -128,
                         TASKTAG_PC,   riscv64IdleTask,
                         TASKTAG_ARG1, SysBase,
                         TAG_DONE);

    D(bug("[Exec] riscv64: idle task @ 0x%p\n", idle));

    return TRUE;
}

ADD2INITLIB(riscv64Idle_Init, -127);
