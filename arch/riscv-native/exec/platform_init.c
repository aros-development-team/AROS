/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/cpu.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <strings.h>
#include <stdio.h>

#include "kernel_cpu.h"

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

extern void IdleTask(struct ExecBase *);

int Exec_RISCVCPUInit(struct ExecBase *SysBase)
{
    struct Task *BootTask, *CPUIdleTask;
#if defined(__AROSEXEC_SMP__)
    int cpu, cpunum = KrnGetCPUCount();
#endif
    char *taskName;

    D(bug("[Exec] %s()\n", __PRETTY_FUNCTION__));

    BootTask = GET_THIS_TASK;

    D(bug("[Exec] %s: launched from %s @ 0x%p\n", __PRETTY_FUNCTION__, BootTask->tc_Node.ln_Name, BootTask));

#if defined(__AROSEXEC_SMP__)
    if (cpunum == 0)
    {
#endif

#if defined(__AROSEXEC_SMP__)
    }

    for (cpu = 0; cpu < cpunum; cpu ++)
    {
        taskName = AllocVec(15, MEMF_CLEAR);
#if (0)
        sprintf( taskName, "CPU #%02d Idle", cpu);
#endif
#else
    taskName = "System Idle";
#endif
        CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , KrnGetCPUMask(cpu),
#endif
                                TASKTAG_PRI        , -127,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

        if (CPUIdleTask)
        {
            D(
                bug("[Exec] %s: %s Task created @ 0x%p\n", __PRETTY_FUNCTION__, CPUIdleTask->tc_Node.ln_Name, CPUIdleTask);
#if defined(__AROSEXEC_SMP__)
                bug("[Exec] %s: CPU Affinity : %08x\n", __PRETTY_FUNCTION__, GetIntETask(CPUIdleTask)->iet_CpuAffinity);
#endif
            )
        }
#if defined(__AROSEXEC_SMP__)
    }
#endif

    return TRUE;
}

#if defined(__AROSEXEC_SMP__)
struct Hook Exec_TaskSpinLockFailHook;

AROS_UFH3(void, Exec_TaskSpinLockFailFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, thisLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    struct Task *thisTask = GET_THIS_TASK;

    /* tell the scheduler that the task is waiting on a spinlock */
    thisTask->tc_State = TS_SPIN;
    GetIntETask(thisTask)->iet_SpinLock = thisLock;

    AROS_USERFUNC_EXIT
}

void Exec_TaskSpinUnlock(spinlock_t *thisLock)
{
    struct Task *curTask, *nxtTask;

    Kernel_43_KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                SPINLOCK_MODE_WRITE, NULL);
    ForeachNodeSafe(&PrivExecBase(SysBase)->TaskSpinning, curTask, nxtTask)
    {
        if (GetIntETask(curTask)->iet_SpinLock == thisLock)
        {
            Kernel_43_KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE, NULL);
            Disable();
            Remove(&curTask->tc_Node);
            Enqueue(&SysBase->TaskReady, &curTask->tc_Node);
            Kernel_44_KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL);
            Enable();
        }
    }
    Kernel_44_KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL);
}

int Exec_RISCVCPUSMPInit(struct ExecBase *SysBase)
{
    int cpu, thiscpu = KrnGetCPUNumber();

    /* set up the task spinning hook */
    Exec_TaskSpinLockFailHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockFailFunc;

    D(bug("[Exec] %s: Task SpinLock Fail hook @ 0x%p initialised (func @ 0x%p)\n", __PRETTY_FUNCTION__, &Exec_TaskSpinLockFailHook, Exec_TaskSpinLockFailHook.h_Entry));

}

ADD2INITLIB(Exec_RISCVCPUSMPInit, -127)
#endif

ADD2INITLIB(Exec_RISCVCPUInit, 0)
