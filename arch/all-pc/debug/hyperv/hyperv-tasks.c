/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__
#define __KERNEL_NOEXTERNBASE__
#define __AROS_KERNEL__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include "etask.h"
#include "kernel_base.h"
#include "kernel_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "exec_intern.h"
#endif
#include "apic.h"

#include "hyperv-cpu.h"

#include LC_LIBDEFS_FILE

void HVDEBUGDumpTaskStats(struct KernelBase *KernelBase, struct Task *task)
{
    struct APICData *apicData = KernelBase->kb_PlatformData->kb_APIC;
    struct ETask *etask = GetETask(task);

    kprintf(" Task @ 0x%p, State = %08x, '%s'\n", task, task->tc_State, task->tc_Node.ln_Name);
    if (etask)
    {
        struct ExceptionContext *regs = (struct ExceptionContext *)etask->et_RegFrame;
        struct timespec timeLast;
        UQUAD timeVal;
        int cpunum =
#if !defined(__AROSEXEC_SMP__)
            0;
#else
            IntETask(etask)->iet_CpuNumber;
#endif

        kprintf("     IP 0x%p, SP 0x%p (0x%p;0x%p", regs->rip, regs->rsp,  task->tc_SPLower, task->tc_SPUpper);
#if (0)
        kprintf(" - %u%%", (((IPTR)task->tc_SPUpper - (IPTR)regs->rsp) / (100 /((IPTR)task->tc_SPUpper - (IPTR)task->tc_SPLower))));
#endif
        kprintf(")\n");

        // Convert TSC cycles into nanoseconds
        timeVal = (IntETask(etask)->iet_private1 * 1000000000) / apicData->cores[cpunum].cpu_TSCFreq;
        timeLast.tv_sec = timeVal / 1000000000;
        timeLast.tv_nsec = timeVal % 1000000000;

        kprintf("     Runtime : last +%u.%u, total %u.%u\n", timeLast.tv_sec, timeLast.tv_nsec, IntETask(etask)->iet_CpuTime.tv_sec, IntETask(etask)->iet_CpuTime.tv_nsec);
    }
}

static int HYDEBUGCompareTaskCtx(APTR ctx, struct Task *curTask)
{
    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(curTask)->et_RegFrame;
    int retval;

    if ((retval = HVDEBUGCompareCtx(ctx, GetETask(curTask)->et_RegFrame)))
    {
        if ((regs->rsp <= (IPTR)curTask->tc_SPUpper) && (regs->rsp >= (IPTR)curTask->tc_SPLower))
        {
            retval = 1;
        }
    }
    return retval;
}

void HVDEBUGDumpTasks(APTR ctx, struct KernelBase *_KernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_KernelBase;
    struct Task *curTask;
    int taskMatch;

    kprintf("\n[HyperV:DEBUG] %s()\n");

#if !defined(__AROSEXEC_SMP__)
    kprintf("\nRUN Task:\n");
    curTask = FindTask(NULL);
    HVDEBUGDumpTaskStats(KernelBase, curTask);
    if (!(taskMatch = HYDEBUGCompareTaskCtx(ctx, curTask)))
        kprintf("  *  Task Context matches current CPU Context\n");
    else if (taskMatch == 1)
        kprintf("  *  Partial Task Context match with current CPU Context\n");
    kprintf("\n");
#else
    kprintf("\nRUN'ning Task(s):\n");

    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        if (!(taskMatch = HYDEBUGCompareTaskCtx(ctx, curTask)))
            kprintf("  *  Task Context matches current CPU Context\n");
        else if (taskMatch == 1)
            kprintf("  *  Partial Task Context match with current CPU Context\n");
        kprintf("\n");
    }

    kprintf("\nSPIN'ning Task(s):\n");

    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        if (!(taskMatch = HYDEBUGCompareTaskCtx(ctx, curTask)))
            kprintf("  *  Task Context matches current CPU Context\n");
        else if (taskMatch == 1)
            kprintf("  *  Partial Task Context match with current CPU Context\n");
        kprintf("\n");
    }
#endif
    kprintf("\nREADY Task(s):\n");

    ForeachNode(&SysBase->TaskReady, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        if (!(taskMatch = HYDEBUGCompareTaskCtx(ctx, curTask)))
            kprintf("  *  Task Context matches current CPU Context\n");
        else if (taskMatch == 1)
            kprintf("  *  Partial Task Context match with current CPU Context\n");
        if (curTask->tc_SigWait & curTask->tc_SigRecvd)
            kprintf("  *  Recv Signal %08x\n", curTask->tc_SigWait & curTask->tc_SigRecvd);
        else if (curTask->tc_SigExcept & curTask->tc_SigRecvd)
            kprintf("  *  Exception %08x pending\n", curTask->tc_SigExcept & curTask->tc_SigRecvd);
        kprintf("\n");
    }
    
    kprintf("\nWAIT'ing Task(s):\n");

    ForeachNode(&SysBase->TaskWait, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        if (!(taskMatch = HYDEBUGCompareTaskCtx(ctx, curTask)))
            kprintf("  *  Task Context matches current CPU Context\n");
        else if (taskMatch == 1)
            kprintf("  *  Partial Task Context match with current CPU Context\n");
        kprintf("     signals - wait %08x, recvd %08x, except %08x\n", curTask->tc_SigWait, curTask->tc_SigRecvd, curTask->tc_SigExcept);
        if (curTask->tc_SigWait & curTask->tc_SigRecvd)
            kprintf("  *  TASK SHOULD HAVE BEEN AWOKEN!?! (%08x)\n", curTask->tc_SigWait & curTask->tc_SigRecvd);
        else if (curTask->tc_SigExcept & curTask->tc_SigRecvd)
            kprintf("  *  TASK has a pending exception (%08x)\n", curTask->tc_SigExcept & curTask->tc_SigRecvd);
        kprintf("\n");
    }
}
