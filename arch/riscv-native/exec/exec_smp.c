/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: exec SMP support for native RISC-V targets: the scheduler-list
          moves exec asks for (krnSysCallReschedTask/krnSysCallSwitch,
          from aarch64-native) and the switch to per-CPU affinity.
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

/* No TS_SPIN machinery here: our KrnSpinLock never calls its failhook,
 * so spinners just spin. all-pc has the wired-up version. */

/*
 * Move a task to the list matching newState, for krnSysCallReschedTask.
 * Callers MUST NOT hold tc_SpinLock - we take it, so the whole
 * (read state, pick list, mutate, write state) is atomic to observers.
 * Lock order: tc_SpinLock outer, list-locks inner.
 */
void Exec_ReschedTask(struct Task *task, ULONG newState)
{
    spinlock_t *fromLock = NULL;
    /* Raw masking, not Disable(): this also runs from the FIQ handler,
     * where Disable()'s syscall would nest an exception. */
    unsigned int __if = EXEC_IRQFIQ_DISABLE();

    Kernel_52_KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE, NULL);

    if (newState == TS_READY)
    {
        /* Only migrate a parked or freshly added task: another CPU may
         * have won the wake race already. The signal bits are set, so a
         * running task sees them anyway. */
        switch (task->tc_State)
        {
            case TS_WAIT:
            case TS_INVALID:
            case TS_ADDED:
                break;
            default:
                Kernel_53_KrnSpinUnLock(&task->tc_SpinLock, NULL);
                EXEC_IRQFIQ_RESTORE(__if);
                return;
        }
    }

    switch (task->tc_State)
    {
        case TS_RUN:
            fromLock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
            break;
        case TS_READY:
            fromLock = &PrivExecBase(SysBase)->TaskReadySpinLock;
            break;
        case TS_WAIT:
            fromLock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
            break;
        default:
            /* Not on a standard scheduler list. */
            break;
    }

    if (fromLock)
    {
        Kernel_52_KrnSpinLock(fromLock, NULL, SPINLOCK_MODE_WRITE, NULL);
        Remove(&task->tc_Node);
        Kernel_53_KrnSpinUnLock(fromLock, NULL);
    }

    task->tc_State = newState;

    switch (newState)
    {
        case TS_READY:
            exec_TaskEnqueueReady(task);
            break;
        case TS_WAIT:
            exec_TaskEnqueueWait(task);
            break;
        default:
            /* TS_REMOVED, TS_TOMBSTONED: no enqueue. */
            break;
    }

    Kernel_53_KrnSpinUnLock(&task->tc_SpinLock, NULL);
    EXEC_IRQFIQ_RESTORE(__if);
}

/* RemTask's self-removal: detach and tombstone for the service task,
 * then return - unlike KrnSwitch() this does NOT dispatch. */
void Exec_SuicideSwitch(void)
{
    struct Task *task = GET_THIS_TASK;
    unsigned int __fiq = EXEC_FIQ_DISABLE();

    Disable();
    /* tc_SpinLock outer, list lock inner. */
    Kernel_52_KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE, NULL);
    Kernel_52_KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
        SPINLOCK_MODE_WRITE, NULL);
    Remove(&task->tc_Node);
    Kernel_53_KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL);
    task->tc_State = TS_TOMBSTONED;
    Kernel_53_KrnSpinUnLock(&task->tc_SpinLock, NULL);
    Enable();
    EXEC_FIQ_RESTORE(__fiq);
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

    if (!ok)
    {
        bug("[Exec] %s: out of memory for affinity masks - staying on one CPU\n", __func__);
        return TRUE;
    }

    /* Only now can another CPU be given work safely */
    krnRVStartCPUs(KernelBase, SysBase);

    return TRUE;
}

/* Before the idle task (exec_idle.c, -127) so it is created pinned */
ADD2INITLIB(Exec_RVSMPInit, -126)

#endif /* __AROSEXEC_SMP__ */
