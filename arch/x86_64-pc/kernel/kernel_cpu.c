/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

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

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#define D(x)
#define DSCHED(x)
#define DREGS(x)

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

static inline unsigned long long RDTSC() {
   unsigned long long _tsc;
   asm volatile (".byte 0x0f, 0x31" : "=A" (_tsc));
   return _tsc;
} 

void cpu_Dispatch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ExceptionContext *ctx;
    apicid_t cpunum = KrnGetCPUNumber();
    struct APICData *apicData;

    DSCHED(
        bug("[Kernel:%02d] cpu_Dispatch()\n", cpunum);
    )

    apicData  = KernelBase->kb_PlatformData->kb_APIC;

    /* 
     * Is the list of ready tasks empty? Well, increment the idle switch count and halt CPU.
     * It should be extended by some plugin mechanism which would put CPU and whole machine
     * into some more sophisticated sleep states (ACPI?)
     */
    while (!(task = core_Dispatch()))
    {
        /* Sleep almost forever ;) */
        __asm__ __volatile__("sti; hlt; cli");

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
    }

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
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_secs = 0;
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_micro = 0;
    IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = RDTSC();
    if ((apicData) &&
        (apicData->cores[cpunum].cpu_TimerFreq) &&
        !(IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_secs) &&
        !(IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_micro))
    {
        IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_secs =
            IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 / apicData->cores[cpunum].cpu_TimerFreq;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_micro =
            IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 % apicData->cores[cpunum].cpu_TimerFreq;
    }
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
    struct timeval timeVal;
    apicid_t cpunum = KrnGetCPUNumber();
    struct APICData *apicData;

    DSCHED(bug("[Kernel:%02d] cpu_Switch()\n", cpunum);)

    timeCur = RDTSC();

    task = GET_THIS_TASK;
    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    timeCur -= IntETask(task->tc_UnionETask.tc_ETask)->iet_private1;

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

    if ((apicData) && (apicData->cores[cpunum].cpu_TimerFreq))
    {
        /* Update the task's CPU time */
        timeVal.tv_secs = timeCur / apicData->cores[cpunum].cpu_TimerFreq;
        timeVal.tv_micro = timeCur % apicData->cores[cpunum].cpu_TimerFreq;

        ADDTIME(&IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime, &timeVal);
    }
    core_Switch();
}
