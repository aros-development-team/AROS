/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
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

    kprintf("[HyperV:DEBUG] %s: Task @ 0x%p, State = %08x, '%s'\n", __func__, task, task->tc_State, task->tc_Node.ln_Name);
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

        kprintf("[HyperV:DEBUG] %s:     IP 0x%p, SP 0x%p (0x%p;0x%p", __func__, regs->rip, regs->rsp,  task->tc_SPLower, task->tc_SPUpper);
#if (0)
        kprintf(" - %u%%", (((IPTR)task->tc_SPUpper - (IPTR)regs->rsp) / (100 /((IPTR)task->tc_SPUpper - (IPTR)task->tc_SPLower))));
#endif
        kprintf(")\n");

        // Convert TSC cycles into nanoseconds
        timeVal = (IntETask(etask)->iet_private1 * 1000000000) / apicData->cores[cpunum].cpu_TSCFreq;
        timeLast.tv_sec = timeVal / 1000000000;
        timeLast.tv_nsec = timeVal % 1000000000;

        kprintf("[HyperV:DEBUG] %s:     Runtime : last +%u.%u, total %u.%u\n", __func__, timeLast.tv_sec, timeLast.tv_nsec, IntETask(etask)->iet_CpuTime.tv_sec, IntETask(etask)->iet_CpuTime.tv_nsec);
    }
}

void HVDEBUGDumpTasks(struct KernelBase *_KernelBase)
{
    struct KernelBase *KernelBase = (struct KernelBase *)_KernelBase;
    struct Task *curTask;

    kprintf("\n[HyperV:DEBUG] %s()\n", __func__);

#if !defined(__AROSEXEC_SMP__)
    kprintf("\n[HyperV:DEBUG] %s: RUNNING TASK\n", __func__);

    HVDEBUGDumpTaskStats(KernelBase, FindTask(NULL));
        kprintf("[HyperV:DEBUG] %s:\n", __func__);
#else
    kprintf("\n[HyperV:DEBUG] %s: RUNNING TASK(S)\n", __func__);

    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        kprintf("[HyperV:DEBUG] %s:\n", __func__);
    }

    kprintf("\n[HyperV:DEBUG] %s: SPINNING TASK(S)\n", __func__);

    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        kprintf("[HyperV:DEBUG] %s:\n", __func__);
    }
#endif
    kprintf("\n[HyperV:DEBUG] %s: READY TASK(S)\n", __func__);

    ForeachNode(&SysBase->TaskReady, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        if (curTask->tc_SigWait & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  recv signal %08x\n", __func__, curTask->tc_SigWait & curTask->tc_SigRecvd);
        else if (curTask->tc_SigExcept & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  exception %08x pending\n", __func__, curTask->tc_SigExcept & curTask->tc_SigRecvd);
        kprintf("[HyperV:DEBUG] %s:\n", __func__);
    }
    
    kprintf("\n[HyperV:DEBUG] %s: WAITING TASK(S)\n", __func__);

    ForeachNode(&SysBase->TaskWait, curTask)
    {
        HVDEBUGDumpTaskStats(KernelBase, curTask);
        kprintf("[HyperV:DEBUG] %s:     signals - wait %08x, recvd %08x, except %08x\n", __func__, curTask->tc_SigWait, curTask->tc_SigRecvd, curTask->tc_SigExcept);
        if (curTask->tc_SigWait & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  TASK SHOULD HAVE BEEN AWOKEN!?! (%08x)\n", __func__, curTask->tc_SigWait & curTask->tc_SigRecvd);
        else if (curTask->tc_SigExcept & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  TASK has a pending exception (%08x)\n", __func__, curTask->tc_SigExcept & curTask->tc_SigRecvd);
        kprintf("[HyperV:DEBUG] %s:\n", __func__);
    }
}
