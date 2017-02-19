/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/types/timespec_s.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "etask.h"

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#include "apic.h"
#include "apic_ia32.h"

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#define DSCHED(x)

void cpu_Dispatch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ExceptionContext *ctx;
    apicid_t cpunum = KrnGetCPUNumber();
    IPTR __APICBase = core_APIC_GetBase();

    DSCHED(
        bug("[Kernel:%03u] cpu_Dispatch()\n", cpunum);
    )
    
    /* 
     * Is the list of ready tasks empty? Well, increment the idle switch count and halt CPU.
     */
    while (!(task = core_Dispatch()))
    {
        /* Sleep until we receive an interupt....*/
        DSCHED(
            bug("[Kernel:%03u] cpu_Dispatch: Nothing to do .. sleeping...\n", cpunum);
        )

        __asm__ __volatile__("sti; hlt; cli");

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
    }

    DSCHED(
        bug("[Kernel:%03u] cpu_Dispatch: Task to Run @ 0x%p\n", cpunum, task);
    )

    /* Get task's context */
    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    /* 
     * Restore the fpu, mmx, xmm state
     * TODO: Change to the lazy saving of the XMM state!!!!
     */
    if (ctx->Flags & ECF_FPX)
	asm volatile("fxrstor (%0)"::"r"(ctx->FXData));

#if defined(__AROSEXEC_SMP__)
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
#endif

    /* TODO: Handle exception
    if (task->tc_Flags & TF_EXCEPT)
        Exception(); */

    /* Store the launch time */
    IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = APIC_REG(__APICBase, APIC_TIMER_CCR);
/*
    if ((apicData) &&
        (apicData->cores[cpunum].cpu_TimerFreq) &&
        !(IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_secs) &&
        !(IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_micro))
    {
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_secs = 0;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_micro = 0;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_secs =
            IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 / apicData->cores[cpunum].cpu_TimerFreq;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_micro =
            IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 % apicData->cores[cpunum].cpu_TimerFreq;
    }
*/
    DSCHED(
        bug("[Kernel:%03u] cpu_Dispatch: Leaving...\n", cpunum);
    )
    /*
     * Leave interrupt and jump to the new task.
     * We will restore CPU state right from this buffer,
     * so no need to copy anything.
     */
    core_LeaveInterrupt(ctx);
}

void cpu_Switch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ExceptionContext *ctx;
    UQUAD timeCur;
    struct timespec timeSpec;
    apicid_t cpunum = KrnGetCPUNumber();
    struct APICData *apicData;
    IPTR __APICBase = core_APIC_GetBase();

    DSCHED(bug("[Kernel:%03u] cpu_Switch()\n", cpunum);)

    timeCur = APIC_REG(__APICBase, APIC_TIMER_CCR);

    task = GET_THIS_TASK;
    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    KernelBase = getKernelBase();
    apicData  = KernelBase->kb_PlatformData->kb_APIC;

    /*
     * Copy current task's context into the ETask structure. Note that context on stack
     * misses SSE data pointer.
     */
    CopyMemQuick(regs, ctx, sizeof(struct ExceptionContext) - sizeof(struct FPXContext *));

    /*
     * Copy the fpu, mmx, xmm state
     * TODO: Change to the lazy saving of the XMM state!!!!
     */
    asm volatile("fxsave (%0)"::"r"(ctx->FXData));

    /* We have the complete data now */
    ctx->Flags = ECF_SEGMENTS | ECF_FPX;

    /* Set task's tc_SPReg */
    task->tc_SPReg = (APTR)regs->rsp;

    if (apicData && apicData->cores[cpunum].cpu_TimerFreq)
    {
        if (timeCur < IntETask(task->tc_UnionETask.tc_ETask)->iet_private1)
            timeCur = IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 - timeCur;
        else
            timeCur = IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 + apicData->cores[cpunum].cpu_TimerFreq - timeCur;
        
        // Convert LAPIC bus cycles into microseconds
        timeCur = (timeCur * 1000000000) / apicData->cores[cpunum].cpu_TimerFreq;
        
        /* Update the task's CPU time */
        timeSpec.tv_sec = timeCur / 1000000000;
        timeSpec.tv_nsec = timeCur % 1000000000;

        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec += timeSpec.tv_nsec;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec  += timeSpec.tv_sec;
        while(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec >= 1000000000)
        {
            IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec -= 1000000000;
            IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec++;
        }
    }
    core_Switch();
}
