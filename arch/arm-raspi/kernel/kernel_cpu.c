/*
 * This file is intended to make generic kernel.resource compiling.
 * This code should NEVER be executed. This file MUST be overriden in
 * architecture-specific code for the scheduler to work!
 */

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include <proto/kernel.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#define D(x)

void cpu_Switch(regs_t *regs)
{
    struct Task *task;

    D(bug("[Kernel] cpu_Switch()\n"));

    /* Disable interrupts until the task switch */
    asm volatile("cpsid i\n");

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    /* Restore the task's state */
    STORE_TASKSTATE(task, regs)

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    D(bug("[Kernel] cpu_Dispatch()\n"));

    asm volatile("cpsid i\n");

    /* Break Disable() if needed */
    if (SysBase->IDNestCnt >= 0) {
        SysBase->IDNestCnt = -1;
        asm volatile("cpsie i\n");
    }

    while (!(task = core_Dispatch())) {
        asm volatile ("mcr p15, #0, %[r], c7, c14, #0" : : [r] "r" (0) );
        asm volatile ("mcr p15, #0, %[r], c7, c10, #5" : : [r] "r" (0) );

        asm volatile("cpsid i\n");

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

void cpu_DumpRegs(regs_t *regs)
{
    int i;
    
    bug("[KRN] Register Dump:\n");
    for (i = 0; i < 12; i++)
    {
        bug("[KRN]      r%02d: 0x%08x\n", i, ((uint32_t *)regs)[i]);
    }
    bug("[KRN] (ip) r12: 0x%08x\n", ((uint32_t *)regs)[12]);
    bug("[KRN] (sp) r13: 0x%08x\n", ((uint32_t *)regs)[13]);
    bug("[KRN] (lr) r14: 0x%08x\n", ((uint32_t *)regs)[14]);
    bug("[KRN] (pc) r15: 0x%08x\n", ((uint32_t *)regs)[15]);
    bug("[KRN]     cpsr: 0x%08x\n", ((uint32_t *)regs)[16]);
}
