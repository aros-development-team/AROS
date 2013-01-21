/*
 * This file is intended to make generic kernel.resource compiling.
 * This code should NEVER be executed. This file MUST be overriden in
 * architecture-specific code for the scheduler to work!
 */

#include <aros/debug.h>

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#include <kernel_debug.h>

#define RESTORE_TASKSTATE(task, regs)                                           \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    for (i = 0; i < 12; i++)                                                    \
    {                                                                           \
        ((uint32_t *)regs)[i] = ctx->r[i];                                      \
    }                                                                           \
    ((uint32_t *)regs)[12] = ctx->ip;                                           \
    ((uint32_t *)regs)[13] = ctx->sp;                                           \
    ((uint32_t *)regs)[14] = ctx->lr;                                           \
    ((uint32_t *)regs)[15] = ctx->pc;                                           \
    ((uint32_t *)regs)[16] = ctx->cpsr;

void cpu_Switch(regs_t *regs)
{
    struct Task *task;
    int i;

    /* Disable interrupts until the task switch */

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    int i;

    /* Break Disable() if needed */
    if (SysBase->IDNestCnt >= 0) {
        SysBase->IDNestCnt = -1;
    }

    while (!(task = core_Dispatch())) {
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
    }

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));       
    }
}
