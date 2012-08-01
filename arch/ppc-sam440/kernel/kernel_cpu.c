/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <asm/amcc440.h>
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

#include <strings.h>

/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void cpu_Dispatch(context_t *regs)
{
    struct Task *task;
    uint64_t idle;
    extern uint64_t idle_time;

    __asm__ __volatile__("wrteei 0;");
    idle = mftbu();

    while (!(task = core_Dispatch())) {
        /* 
         * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
         * It should be extended by some plugin mechanism which would put CPU and whole machine
         * into some more sophisticated sleep states (ACPI?)
         */
        if (SysBase->IDNestCnt < 0) {
            wrdcr(UIC0_ER, uic_er[0]);
        }
        wrmsr(rdmsr() | MSR_POW | MSR_EE);
        __asm__ __volatile__("sync; isync;");
        __asm__ __volatile__("wrteei 0");
        idle_time += mftbu() - idle;
    }

    /* Restore the task's state */
    CopyMem(task->tc_UnionETask.tc_ETask->et_RegFrame, regs, sizeof(regs_t));
    regs->cpu.gpr[1] = (IPTR)task->tc_SPReg;

    /* Set the external interrupts */
    if (SysBase->IDNestCnt < 0) {
        wrdcr(UIC0_ER, uic_er[0]);
    } else {
        wrdcr(UIC0_ER, 0);
    }

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    /* Store the launch time */
    GetIntETask(task)->iet_private1 = mftbu();

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));       
    }
    
    /* Copy the fpu, mmx, xmm state */
// FIXME: Change to the lazy saving of the FPU state!!!!

    regs->cpu.srr1 |= MSR_EE;
}

void cpu_Switch(context_t *regs)
{
    struct Task *task;
    
    /* Disable interrupts until the task switch */
    __asm__ __volatile__("wrteei 0");

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    memmove(task->tc_UnionETask.tc_ETask->et_RegFrame, regs, sizeof(context_t));
        
    /* Copy the fpu, mmx, xmm state */
// FIXME: Change to the lazy saving of the FPU state!!!!
        
    task->tc_SPReg = (APTR)regs->cpu.gpr[1];
        
    core_Switch();
}
