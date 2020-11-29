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

/*
 * NB - Enabling DSCHED() with safedebug enabled CAN cause
 * lockups!
 */
#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 0

 #if (DEBUG > 0)
#define DSCHED(x) x
#else
#define DSCHED(x)
#endif

void cpu_Dispatch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ExceptionContext *ctx;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    apicid_t cpunum = KrnGetCPUNumber();
#endif

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

    DSCHED(
        bug("[Kernel:%03u] cpu_Dispatch: Restoring FPU/MMX registers\n", cpunum);
        bug("[Kernel:%03u] cpu_Dispatch: tc_ETask = 0x%p\n", cpunum, task->tc_UnionETask.tc_ETask);
        bug("[Kernel:%03u] cpu_Dispatch: et_RegFrame = 0x%p, FXSData = 0x%p\n", cpunum, ctx, ctx->FXSData);
    )
    if (ctx)
    {
        /* 
         * Restore the x86 FPU / XMM / AVX512/ MXCSR state
         * NB: lazy saving of the x86 FPU states or FPU / XMM / AVX512 state, has
         * been reported to leak across process, aswell as VM boundaries, giving
         * attackers possibilities to read private data from other processes,
         * when using speculative execution side channel attacks.
         * While this isnt (currently) an issue for AROS, we will not use them none the less ...
         */
        if (ctx->Flags & ECF_FPXS)
        {
            DSCHED(bug("[Kernel:%03u] cpu_Dispatch: restoring AVX registers\n", cpunum);)
            asm volatile("xrstor (%0)"::"r"(ctx->XSData));
        }
        else if (ctx->Flags & ECF_FPFXS)
        {
            DSCHED(bug("[Kernel:%03u] cpu_Dispatch: restoring SSE registers\n", cpunum);)
            asm volatile("fxrstor (%0)"::"r"(ctx->FXSData));
        }
    }
#if defined(__AROSEXEC_SMP__)
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
#endif

    if (task->tc_Flags & TF_EXCEPT)
    {
#if (0)
        Exception();
#else
        /* TODO: Handle exception */
        DSCHED(
            bug("[Kernel:%03u] cpu_Dispatch: !! unhandled exception in task @ 0x%p '%s'\n", cpunum, task, task->tc_Node.ln_Name);
            bug("[Kernel:%03u] cpu_Dispatch: tc_SigExcept %08x, tc_SigRecvd %08x\n", cpunum, task->tc_SigExcept, task->tc_SigRecvd);
        )
#endif
    }

    DSCHED(
        bug("[Kernel:%03u] cpu_Dispatch: Caching task launch time\n", cpunum);
    )

    /* Store the launch time */
    IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = RDTSC();

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
    UQUAD timeCur = 0;
    struct timespec timeSpec;
    apicid_t cpunum = KrnGetCPUNumber();
    struct APICData *apicData;
    apicData  = KernelBase->kb_PlatformData->kb_APIC;

    DSCHED(bug("[Kernel:%03u] cpu_Switch()\n", cpunum);)

    task = GET_THIS_TASK;

    if (task)
    {
        timeCur = RDTSC();

        ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

        /*
        * Copy current task's context into the ETask structure. Note that context on stack
        * misses SSE data pointer.
        */
        CopyMemQuick(regs, ctx, sizeof(struct ExceptionContext) - sizeof(struct FPXSContext *));
        ctx->Flags = ECF_SEGMENTS;

        /*
        * Cache x86 FPU / XMM / AVX512 / MXCSR state
        * NB: See the note about lazy saving of the fpu above!!
        */
        if (ctx->FPUCtxSize > sizeof(struct FPXSContext))
        {
            DSCHED(bug("[Kernel:%03u] cpu_Switch: saving AVX registers\n", cpunum);)
            asm volatile("xsave (%0)"::"r"(ctx->XSData));
            ctx->Flags |= ECF_FPXS;
        }
        else if (ctx->FPUCtxSize == sizeof(struct FPFXSContext))
        {
            DSCHED(bug("[Kernel:%03u] cpu_Switch: saving SSE registers\n", cpunum);)
            asm volatile("fxsave (%0)"::"r"(ctx->FXSData));
            ctx->Flags |= ECF_FPFXS;
        }

        /* Set task's tc_SPReg */
        task->tc_SPReg = (APTR)regs->rsp;

        if (apicData && apicData->cores[cpunum].cpu_TimerFreq && timeCur)
        {
            /*
            if (timeCur < IntETask(task->tc_UnionETask.tc_ETask)->iet_private1)
                timeCur = IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 - timeCur;
            else
                timeCur = IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 + apicData->cores[cpunum].cpu_TimerFreq - timeCur;
            */
            timeCur -= IntETask(task->tc_UnionETask.tc_ETask)->iet_private1;
            
            /* Increase CPU Usage cycles */
            IntETask(task->tc_UnionETask.tc_ETask)->iet_private2 += timeCur;

            // Convert TSC cycles into nanoseconds
            timeCur = (timeCur * 1000000000) / apicData->cores[cpunum].cpu_TSCFreq;

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
    }
    core_Switch();
}
