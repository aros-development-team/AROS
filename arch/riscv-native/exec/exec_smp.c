/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: exec SMP support for native RISC-V targets: the spinlock fail
          hook and the switch to per-CPU affinity.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include "kernel_riscv.h"

#include "exec_intern.h"
#include "etask.h"

/*
 * The hook a caller can pass to KrnSpinLock() for a lock it may have to
 * wait long for (as on all-pc): the scheduler is told that the task only
 * spins, and the task gives up the CPU until it is scheduled again.
 */
struct Hook Exec_TaskSpinLockFailHook;

AROS_UFH3(void, Exec_TaskSpinLockFailFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, spinLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    struct Task *spinTask = GET_THIS_TASK;
    struct IntETask *thisET;

    if (spinTask)
    {
        thisET = GetIntETask(spinTask);
        if (thisET)
        {
            /* tell the scheduler that the task is waiting on a spinlock */
            spinTask->tc_State = TS_SPIN;

            thisET->iet_SpinLock = spinLock;
        }
    }

    /* schedule us away for now .. */
    Reschedule();

    AROS_USERFUNC_EXIT
}

/*
 * Give every task that exists so far an affinity mask, then switch exec
 * to per-CPU affinity (all-pc does the same in cpu_PrepareExec). A NULL
 * mask is not valid and would leave a task that no CPU dispatches, so
 * the flag is only set once every task has one. They all ran on the
 * boot CPU until now, and the boot sequence is not migration-safe, so
 * that is where they stay; tasks created from here on inherit their
 * creator's mask (InitETask).
 */
static BOOL Exec_RVAssignMask(struct Task *task)
{
    struct IntETask *iet;
    void *aff;

    if (!(task->tc_Flags & TF_ETASK) || !task->tc_UnionETask.tc_ETask)
        return TRUE;

    iet = IntETask(task->tc_UnionETask.tc_ETask);
    if (iet->iet_CpuAffinity)
        return TRUE;

    if (!(aff = KrnAllocCPUMask()))
        return FALSE;
    KrnGetCPUMask(0, aff);
    iet->iet_CpuAffinity = aff;
    iet->iet_CpuNumber = 0;

    return TRUE;
}

static int Exec_RVSMPInit(struct ExecBase *SysBase)
{
    struct Task *task;
    BOOL ok = TRUE;
    void *sysmask;

    D(bug("[Exec] %s()\n", __func__));

    /* set up the task spinning hook */
    Exec_TaskSpinLockFailHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockFailFunc;

    /* The CPUs exec may schedule on; more are added as harts come online */
    if (!(sysmask = KrnAllocCPUMask()))
    {
        bug("[Exec] %s: no memory for the system CPU mask - staying on one CPU\n", __func__);
        return TRUE;
    }
    KrnGetCPUMask(0, sysmask);
    PrivExecBase(SysBase)->CPUMask = sysmask;

    Disable();
    ok = Exec_RVAssignMask(GET_THIS_TASK);
    ForeachNode(&SysBase->TaskReady, task)
        ok = ok && Exec_RVAssignMask(task);
    ForeachNode(&SysBase->TaskWait, task)
        ok = ok && Exec_RVAssignMask(task);
    if (ok)
        PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;
    Enable();

    /* The other harts are started later, by kernel.post (platform_post.c) */
    if (!ok)
        bug("[Exec] %s: out of memory for affinity masks - staying on one CPU\n", __func__);

    return TRUE;
}

/* Before the idle task (exec_idle.c, -127) so it is created pinned */
ADD2INITLIB(Exec_RVSMPInit, -126)

#endif /* __AROSEXEC_SMP__ */
