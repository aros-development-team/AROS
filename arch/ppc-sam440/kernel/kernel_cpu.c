/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/amcc440.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "exec_intern.h"
#include "etask.h"

#include "kernel_intern.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void cpu_Dispatch(context_t *regs)
{
    struct ExecBase *SysBase = getSysBase();
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
        wrmsr(rdmsr() | MSR_POW | MSR_EE);
        __asm__ __volatile__("sync; isync;");
        __asm__ __volatile__("wrteei 0");
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
        idle_time += mftbu() - idle;
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
    
    /* Restore the task's state */
    regs = task->tc_UnionETask.tc_ETask->et_RegFrame;

    if (SysBase->IDNestCnt < 0)
            regs->cpu.srr1 |= MSR_EE;

    /* Copy the fpu, mmx, xmm state */
// FIXME: Change to the lazy saving of the FPU state!!!!
    
    regs->cpu.srr1 &= ~MSR_POW;
}

void cpu_Switch(context_t *regs)
{
    struct ExecBase *SysBase = getSysBase();
    struct Task *task;
    
    /* Disable interrupts for a while */
    __asm__ __volatile__("wrteei 0");

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    bcopy(regs, task->tc_UnionETask.tc_ETask->et_RegFrame, sizeof(context_t));
        
    /* Copy the fpu, mmx, xmm state */
// FIXME: Change to the lazy saving of the FPU state!!!!
        
    task->tc_SPReg = (APTR)regs->cpu.gpr[1];
        
    /* And enable interrupts */
    __asm__ __volatile__("wrteei 1");

    core_Switch();
}
