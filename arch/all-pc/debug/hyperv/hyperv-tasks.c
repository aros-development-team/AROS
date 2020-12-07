/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include <aros/x86_64/cpucontext.h>

#include "etask.h"
#include "hyperv-cpu.h"

#include LC_LIBDEFS_FILE

void HVDEBUGDumpTaskStat(struct Task *task)
{
    struct ETask *etask = GetETask(task);

    kprintf("[HyperV:DEBUG] %s: Task @ 0x%p, State = %08x, '%s'\n", __func__, task, task->tc_State, task->tc_Node.ln_Name);
    if (etask)
    {
        struct ExceptionContext *regs = (struct ExceptionContext *)etask->et_RegFrame;
#if (0)
        kprintf("[HyperV:DEBUG] %s:     IP 0x%p, SP 0x%p (0x%p;0x%p - %u%%)\n", __func__, regs->rip, regs->rsp,  task->tc_SPLower, task->tc_SPUpper, (((IPTR)task->tc_SPUpper - (IPTR)regs->rsp) / (100 /((IPTR)task->tc_SPUpper - (IPTR)task->tc_SPLower))));
#else
        kprintf("[HyperV:DEBUG] %s:     IP 0x%p, SP 0x%p (0x%p;0x%p)\n", __func__, regs->rip, regs->rsp,  task->tc_SPLower, task->tc_SPUpper);
#endif
        kprintf("[HyperV:DEBUG] %s:     Runtime : last %08x%08x, total %u.%d\n", __func__, (IntETask(etask)->iet_private1 >> 32) & 0xFFFFFFFF, IntETask(etask)->iet_private1 & 0xFFFFFFFF, IntETask(etask)->iet_CpuTime.tv_sec, IntETask(etask)->iet_CpuTime.tv_nsec);
    }
}

void HVDEBUGDumpTasks()
{
    struct Task *curTask;

    kprintf("\n[HyperV:DEBUG] %s()\n", __func__);

#if !defined(__AROSEXEC_SMP__)
    kprintf("\n[HyperV:DEBUG] %s: RUNNING TASK\n", __func__);

    HVDEBUGDumpTaskStat(FindTask(NULL));
#else
    kprintf("\n[HyperV:DEBUG] %s: RUNNING TASK(S)\n", __func__);

    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        HVDEBUGDumpTaskStat(curTask);
    }

    kprintf("\n[HyperV:DEBUG] %s: SPINNING TASK(S)\n", __func__);

    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, curTask)
    {
        HVDEBUGDumpTaskStat(curTask);
    }
#endif
    kprintf("\n[HyperV:DEBUG] %s: READY TASK(S)\n", __func__);

    ForeachNode(&SysBase->TaskReady, curTask)
    {
        HVDEBUGDumpTaskStat(curTask);
    }
    
    kprintf("\n[HyperV:DEBUG] %s: WAITING TASK(S)\n", __func__);

    ForeachNode(&SysBase->TaskWait, curTask)
    {
        HVDEBUGDumpTaskStat(curTask);
        kprintf("[HyperV:DEBUG] %s:     signals - wait %08x, recvd %08x, except %08x\n", __func__, curTask->tc_SigWait, curTask->tc_SigRecvd, curTask->tc_SigExcept);
        if (curTask->tc_SigWait & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  TASK SHOULD HAVE BEEN AWOKEN!?! (%08x)\n", __func__, curTask->tc_SigWait & curTask->tc_SigRecvd);
        else if (curTask->tc_SigExcept & curTask->tc_SigRecvd)
            kprintf("[HyperV:DEBUG] %s:  *  TASK has a pending exception (%08x)\n", __func__, curTask->tc_SigExcept & curTask->tc_SigRecvd);
    }
}
