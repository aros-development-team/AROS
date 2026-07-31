/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU-level task switching for the opensbi-riscv64 target.

    cpu_Switch() saves the interrupted context (the trap frame) into the
    current task's register frame; cpu_Dispatch() selects the next task
    and loads its register frame into the trap frame - the trap exit
    path then resumes the new task via sret.
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

/*
 * Copy the volatile part of a context: the x registers, pc and Flags.
 * The fpuContext/vecContext pointers are deliberately preserved - each
 * ExceptionContext owns its own save areas.
 */
static void copyContext(struct ExceptionContext *dst,
                        struct ExceptionContext *src)
{
    int i;

    for (i = 0; i < RISCV_REGSAVE_CNT; i++)
        dst->x[i] = src->x[i];
    dst->pc    = src->pc;
    dst->Flags = src->Flags;
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    copyContext(ctx, regs);
    task->tc_SPReg = (APTR)regs->sp;

    /* TODO: save the FPU (and, when present, vector) state here once
       lazy FS/VS dirty tracking is wired up (see kernel_cpu.h) */

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    while (!(task = core_Dispatch()))
    {
        /*
         * Nothing to run. Interrupts are masked inside the trap
         * handler, so wait for one and service a pending timer tick by
         * hand; the wakeup it causes (via Signal from a future
         * interrupt handler) makes a task ready.
         */
        asm volatile("wfi");
        if (csr_read(sip) & SIE_STIE)
            krnTimerTick();
    }

    copyContext(regs, task->tc_UnionETask.tc_ETask->et_RegFrame);

    /* Handle task's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
    /* Leave the trap and resume the new task */
}
