/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#define DREGS(x)

extern struct Task *sysIdleTask;

void cpu_Probe(struct ARM_Implementation *kernARM)
{
    uint32_t tmp;

    asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (tmp));
    if ((tmp & 0xfff0) == 0xc070) /* armv7 */
        kernARM->ARMI_Family = 7;
    else
        kernARM->ARMI_Family = 6;
}

void cpu_Init(struct ARM_Implementation *kernARM, struct TagItem *msg)
{
    register unsigned int fpuflags;
    unsigned int delay;

    core_SetupMMU(msg);

    if (kernARM->ARMI_LED_Toggle)
    {
        for (delay = 0; delay < 100000; delay++) asm volatile ("mov r0, r0\n");
        kernARM->ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_OFF);
    }

    /* Enable Vector Floating Point Calculations */
    asm volatile("mrc p15,0,%[fpuflags],c1,c0,2\n" : [fpuflags] "=r" (fpuflags));   // Read Access Control Register 
    fpuflags |= (VFPSingle | VFPDouble);                                            // Enable Single & Double Precision 
    asm volatile("mcr p15,0,%[fpuflags],c1,c0,2\n" : : [fpuflags] "r" (fpuflags)); // Set Access Control Register
    asm volatile(
        "       mov %[fpuflags],%[vfpenable]    \n"                                 // Enable VFP 
        "       fmxr fpexc,%[fpuflags]          \n"
         : [fpuflags] "=r" (fpuflags) : [vfpenable] "I" (VFPEnable));

    if (kernARM->ARMI_LED_Toggle)
    {
        for (delay = 0; delay < 100000; delay++) asm volatile ("mov r0, r0\n");
        kernARM->ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_ON);
    }
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task;

    D(bug("[Kernel] cpu_Switch()\n"));

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    /* Restore the task's state */
    STORE_TASKSTATE(task, regs)

    /* Update the taks CPU time .. */
    GetIntETask(task)->iet_CpuTime += *((volatile unsigned int *)(SYSTIMER_CLO)) - GetIntETask(task)->iet_private1;

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    D(bug("[Kernel] cpu_Dispatch()\n"));

    /* Break Disable() if needed */
    if (SysBase->IDNestCnt >= 0) {
        SysBase->IDNestCnt = -1;
        ((uint32_t *)regs)[13] &= ~0x80;
    }

    if (!(task = core_Dispatch()))
        task = sysIdleTask;

    D(bug("[Kernel] cpu_Dispatch: Letting '%s' run for a bit..\n", task->tc_Node.ln_Name));

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    DREGS(cpu_DumpRegs(regs));

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    /* Store the launch time */
    GetIntETask(task)->iet_private1 = *((volatile unsigned int *)(SYSTIMER_CLO));

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
