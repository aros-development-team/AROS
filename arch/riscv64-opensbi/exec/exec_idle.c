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

/*
 * One task that is always runnable, so the dispatcher never has to idle
 * inside itself when every other task is waiting. A trap taken in the
 * dispatcher cannot reschedule from within the very function about to do
 * so, and has to return early; that also skips core_ExitInterrupt(), and
 * with it the soft interrupt carrying an I/O completion. Halting as a task
 * keeps the CPU out of the dispatcher, so such a trap returns the ordinary
 * way and the completion is delivered at once.
 */
static void riscv64IdleTask(struct ExecBase *SysBase)
{
    D(bug("[Exec] riscv64: '%s' running\n", FindTask(NULL)->tc_Node.ln_Name));

    for (;;)
    {
        /*
         * Tasks run with traps enabled, so this simply waits for the next
         * one. It is a hint: a spurious return is harmless, the reschedule
         * below just finds nothing new and comes back here.
         */
        asm volatile("wfi");

        Reschedule();
    }
}

static int riscv64Idle_Init(struct ExecBase *SysBase)
{
    struct Task *idle;

    idle = NewCreateTask(TASKTAG_NAME, "CPU Idle",
                         TASKTAG_PRI,  -128,
                         TASKTAG_PC,   riscv64IdleTask,
                         TASKTAG_ARG1, SysBase,
                         TAG_DONE);

    D(bug("[Exec] riscv64: idle task @ 0x%p\n", idle));

    return TRUE;
}

ADD2INITLIB(riscv64Idle_Init, -127);
