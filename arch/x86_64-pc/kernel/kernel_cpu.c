/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
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

#define SCHEDULERASCII_DEBUG

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

 #if (DEBUG > 0)
#define DSCHED(x) x
#else
#define DSCHED(x)
#endif

#if (DEBUG > 0) && defined(SCHEDULERASCII_DEBUG)
#define DEBUGCOLOR_SET       "\033[32m"
#define DEBUGFUNCCOLOR_SET   "\033[32;1m"
#define DEBUGCOLOR_RESET     "\033[0m"
#else
#define DEBUGCOLOR_SET
#define DEBUGFUNCCOLOR_SET
#define DEBUGCOLOR_RESET
#endif

#define DEBUG_XMM 0 /* Keep the same with all-pc/kernel/cpu_intr.c !! */
#if DEBUG_XMM
extern UBYTE *pseudorsp;
#define SAVE_XMM_INTO_AREA(area)                \
    asm volatile (                              \
        "       movaps %%xmm0, (%0)\n"          \
        "       movaps %%xmm1, 16(%0)\n"        \
        "       movaps %%xmm2, 32(%0)\n"        \
        "       movaps %%xmm3, 48(%0)\n"        \
        "       movaps %%xmm4, 64(%0)\n"        \
        "       movaps %%xmm5, 80(%0)\n"        \
        "       movaps %%xmm6, 96(%0)\n"        \
        "       movaps %%xmm7, 112(%0)\n"       \
        ::"r"(area));

#define SAVE_XMM_AND_CHECK                      \
UQUAD xmmpost[16];                              \
SAVE_XMM_INTO_AREA(xmmpost)                     \
UQUAD *xmmpre = (UQUAD *)localarea;             \
for (int i = 0; i < 15; i++)                    \
    if (xmmpre[i] != xmmpost[i]) bug("diff in core_Switch (%d) %lx vs %lx!!\n", i, xmmpre[i], xmmpost[i]);

#endif

void cpu_Dispatch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ETask * taskEtask = NULL;
    struct ExceptionContext *ctx;
    apicid_t cpunum = KrnGetCPUNumber();

    DSCHED(
        bug("[Kernel:%03u]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", cpunum, __func__);
    )
    
    /*
     * Is the list of ready tasks empty? Well, increment the idle switch count and halt CPU.
     */
    while (!(task = core_Dispatch()))
    {
        /* Sleep until we receive an interupt....*/
        DSCHED(
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Nothing to do .. sleeping..." DEBUGCOLOR_RESET "\n", cpunum, __func__);
        )

        __asm__ __volatile__("sti; hlt; cli");

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
    }

    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Task to Run @ 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, task);
    )

    if (task->tc_Flags & TF_ETASK)
    {
        taskEtask = GetETask(task);
        if (taskEtask)
        {
            /* Get task's context */
            ctx = taskEtask->et_RegFrame;

            DSCHED(
                bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: tc_ETask = 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, taskEtask);
                bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: et_RegFrame = 0x%p, FXSData = 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx, ctx->FXSData);
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
                    if (ctx->XSData)
                    {
                        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: restoring AVX registers" DEBUGCOLOR_RESET "\n", cpunum, __func__);)
                        asm volatile(
                            "       xor %%edx, %%edx\n"
                            "       mov $0b111, %%eax\n"        /* Load instruction mask */
                            "       xrstor (%0)"
                            ::"r"(ctx->XSData): "rax", "rdx");
                    }
                    else
                    {
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: AVX reg storage missing for task 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, task);
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         '%s'" DEBUGCOLOR_RESET "\n", cpunum, __func__, task->tc_Node.ln_Name);
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         ctx data @ %p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx);
                    }
                }
                else if (ctx->Flags & ECF_FPFXS)
                {
                    if (ctx->FXSData)
                    {
                        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: restoring SSE registers" DEBUGCOLOR_RESET "\n", cpunum, __func__);)
                        asm volatile("fxrstor (%0)"::"r"(ctx->FXSData));
                    }
                    else
                    {
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: SSE reg storage missing for task 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, task);
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         '%s'" DEBUGCOLOR_RESET "\n", cpunum, __func__, task->tc_Node.ln_Name);
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         ctx data @ %p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx);
                    }
                }
            }
#if defined(__AROSEXEC_SMP__)
            IntETask(taskEtask)->iet_CpuNumber = cpunum;
#endif
        }
    }

    if (task->tc_Flags & TF_EXCEPT)
    {
#if (0)
        Exception();
#else
        /* TODO: Handle exception */
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: !! unhandled exception in task @ 0x%p '%s'" DEBUGCOLOR_RESET "\n", cpunum, __func__, task, task->tc_Node.ln_Name);
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: tc_SigExcept %08X, tc_SigRecvd %08X" DEBUGCOLOR_RESET "\n", cpunum, __func__, task->tc_SigExcept, task->tc_SigRecvd);
#endif
    }

    if (taskEtask)
    {
        DSCHED(
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Caching task launch time (previous == %u)" DEBUGCOLOR_RESET "\n", cpunum, __func__, IntETask(taskEtask)->iet_private1);
        )
        /* Store the launch time */
        IntETask(taskEtask)->iet_private1 = RDTSC();
    }

    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Leaving..." DEBUGCOLOR_RESET "\n", cpunum, __func__);
    )
#if DEBUG_XMM
pseudorsp -= 16 * 8;
#endif
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
    struct ETask * taskEtask = NULL;
    apicid_t cpunum = KrnGetCPUNumber();

    DSCHED(bug("[Kernel:%03u]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", cpunum, __func__);)

    task = GET_THIS_TASK;

    if ((task) && (task->tc_Flags & TF_ETASK) && (regs->Flags & ECF_SEGMENTS))
    {
        taskEtask = GetETask(task);
        if (taskEtask)
        {
            struct APICData *apicData;
            struct ExceptionContext *ctx;
            UQUAD timeCur = RDTSC();
            ULONG tcFlags = 0;

            if (!(ctx = taskEtask->et_RegFrame))
            {
                bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: !!!!! MISSING et_RegFrame !!!!!" DEBUGCOLOR_RESET "\n", cpunum, __func__);
            }

            /* Restore first four XMM registers. They could have been modified by any interrupt handler.
               Interrupt handler or soft interrupt code is required to preserve XMM registers 5-15. */
            /* Registers are restored here so that their correct values are written into cpu context and
               can be restored later in cpu_Dispatch */
            asm volatile (
                "       movaps (%0), %%xmm0\n"
                "       movaps 16(%0), %%xmm1\n"
                "       movaps 32(%0), %%xmm2\n"
                "       movaps 48(%0), %%xmm3\n"
                ::"r"(regs->FXSData));

#if DEBUG_XMM
APTR localarea = pseudorsp - (16 * 8);
SAVE_XMM_AND_CHECK
#endif
            /*
            * Cache x86 FPU / XMM / AVX512 / MXCSR state first
            * NB: See the note about lazy saving of the fpu above!!
            */
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: ctx->FPUCtxSize = %u" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx->FPUCtxSize);)
            tcFlags &= ~(ECF_FPXS|ECF_FPFXS);
            if (ctx->FPUCtxSize > sizeof(struct FPFXSContext))
            {
                if (ctx->XSData)
                {
                    DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: saving AVX registers to 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx->XSData);)
                    asm volatile(
                        "       xor %%edx, %%edx\n"
                        "       mov $0b111, %%eax\n"        /* Load instruction mask */
                        "       xsave (%0)"
                        ::"r"(ctx->XSData): "rax", "rdx");
                    tcFlags |= ECF_FPXS;
                }
                else
                {
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: AVX reg storage missing for task 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, task);
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         '%s'" DEBUGCOLOR_RESET "\n", cpunum, __func__, task->tc_Node.ln_Name);
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         ctx data @ %p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx);
                }
            }
            else if (ctx->FPUCtxSize == sizeof(struct FPFXSContext))
            {
                if (ctx->FXSData)
                {
                    DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: saving SSE registers to 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx->FXSData);)
                    asm volatile("fxsave (%0)"::"r"(ctx->FXSData));
                    tcFlags |= ECF_FPFXS;
                }
                else
                {
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: SSE reg storage missing for task 0x%p" DEBUGCOLOR_RESET "\n", cpunum, __func__, task);
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         '%s'" DEBUGCOLOR_RESET "\n", cpunum, __func__, task->tc_Node.ln_Name);
                    bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s:         ctx data @ %p" DEBUGCOLOR_RESET "\n", cpunum, __func__, ctx);
                }
            }

            /*
            * Copy current task's context into the ETask structure. Note that context on stack
            * does not have valid pointer to full SSE data.
            */
            CopyMemQuick(regs, ctx, ((IPTR)&regs->gs  - (IPTR)regs) + sizeof(regs->gs));
            ctx->rip    = regs->rip;
            ctx->cs     = regs->cs;
            ctx->rflags = regs->rflags;
            ctx->rsp    = regs->rsp;
            ctx->ss     = regs->ss;

            ctx->Flags |= tcFlags;

            /* Set task's tc_SPReg */
            task->tc_SPReg = (APTR)regs->rsp;

            if (KernelBase->kb_PlatformData)
            {
                apicData  = KernelBase->kb_PlatformData->kb_APIC;
                if (apicData &&
                    apicData->cores[cpunum].cpu_TSCFreq &&
                    apicData->cores[cpunum].cpu_TimerFreq &&
                    timeCur)
                {
                    struct timespec timeSpec;
                    DSCHED(
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Updating task run-time" DEBUGCOLOR_RESET "\n", cpunum, __func__);
                    )
                    /*
                    if (timeCur < IntETask(taskEtask)->iet_private1)
                        timeCur = IntETask(taskEtask)->iet_private1 - timeCur;
                    else
                        timeCur = IntETask(taskEtask)->iet_private1 + apicData->cores[cpunum].cpu_TimerFreq - timeCur;
                    */
                    timeCur -= IntETask(taskEtask)->iet_private1;
                    
                    /* Increase CPU Usage cycles */
                    IntETask(taskEtask)->iet_private2 += timeCur;

                    // Convert TSC cycles into nanoseconds
                    timeCur = (timeCur * 1000000000) / apicData->cores[cpunum].cpu_TSCFreq;

                    /* Update the task's CPU time */
                    timeSpec.tv_sec = timeCur / 1000000000;
                    timeSpec.tv_nsec = timeCur % 1000000000;

                    IntETask(taskEtask)->iet_CpuTime.tv_nsec += timeSpec.tv_nsec;
                    IntETask(taskEtask)->iet_CpuTime.tv_sec  += timeSpec.tv_sec;
                    while(IntETask(taskEtask)->iet_CpuTime.tv_nsec >= 1000000000)
                    {
                        IntETask(taskEtask)->iet_CpuTime.tv_nsec -= 1000000000;
                        IntETask(taskEtask)->iet_CpuTime.tv_sec++;
                    }
                    DSCHED(
                        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: = %u.%u" DEBUGCOLOR_RESET "\n", cpunum, __func__, IntETask(taskEtask)->iet_CpuTime.tv_sec, IntETask(taskEtask)->iet_CpuTime.tv_nsec);
                    )
                }
            }
            else
            {
                DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Kernel not ready" DEBUGCOLOR_RESET "\n", cpunum, __func__);)
            }
        }
        else
        {
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: ETask missing" DEBUGCOLOR_RESET "\n", cpunum, __func__);)
        }
    }
    else
    {
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: INVALID STATE!!" DEBUGCOLOR_RESET "\n", cpunum, __func__);)
    }
    core_Switch();
}
