/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 exec platform initialization.
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
#include "kernel_ipi.h"

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/* Linked from kernel.resource */
extern IPTR stack[];
extern IPTR stack_super[];

extern void IdleTask(struct ExecBase *);

int Exec_ARMCPUInit(struct ExecBase *SysBase)
{
    struct Task *BootTask, *CPUIdleTask;
#if defined(__AROSEXEC_SMP__)
    int cpu, cpunum = KrnGetCPUCount();
    void *cpuMask = NULL;
#endif
    char *taskName;

    D(bug("[Exec] %s()\n", __PRETTY_FUNCTION__));

    BootTask = GET_THIS_TASK;

    D(bug("[Exec] %s: launched from %s @ 0x%p\n", __PRETTY_FUNCTION__, BootTask->tc_Node.ln_Name, BootTask));

    /* The boot task runs on the kernel's boot stack. (Under SMP this was
     * only done when KrnGetCPUCount() returned 0, which it never does.) */
    BootTask->tc_SPLower = stack;
    BootTask->tc_SPUpper = stack + AROS_STACKSIZE;
#if defined(__AROSEXEC_SMP__)

    for (cpu = 0; cpu < cpunum; cpu++)
    {
        taskName = AllocVec(15, MEMF_CLEAR);
        sprintf(taskName, "CPU #%02d Idle", cpu);
        /* Not optional: affinity 0 would mean the boot CPU, not this one */
        if ((cpuMask = KrnAllocCPUMask()) == NULL)
        {
            D(bug("[Exec] %s: no CPU mask for CPU #%02d's idle task\n", __PRETTY_FUNCTION__, cpu));
            FreeVec(taskName);
            continue;
        }
        KrnGetCPUMask(cpu, cpuMask);
#else
    taskName = "System Idle";
#endif
        CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , cpuMask,
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
#if defined(__AROSEXEC_SMP__)
            /* KATTR_CPULoad derives a core's load from its idle task,
             * and only exec knows the mapping. */
            if (cpu < AARCH64_MAXCPUS)
                aarch64_IdleTask[cpu] = CPUIdleTask;
#endif
        }
#if defined(__AROSEXEC_SMP__)
    }
#endif

    return TRUE;
}

#if defined(__AROSEXEC_SMP__)
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

/* Pin a task that predates EXECF_CPUAffinity to the boot CPU */
static BOOL Exec_ARMAssignBootMask(struct Task *task)
{
    struct IntETask *iet;
    void *aff;

    if (!task || !(task->tc_Flags & TF_ETASK) || !task->tc_UnionETask.tc_ETask)
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

int Exec_ARMCPUSMPInit(struct ExecBase *SysBase)
{
    struct Task *task;
    BOOL ok;

    /*
     * Every task that predates EXECF_CPUAffinity gets a mask first (the
     * coldstart sequence is not migration-safe, so the boot CPU's), as
     * all-pc does: a NULL mask is invalid and would leave a task no CPU
     * dispatches. Only then is the gate for signal.c's cross-CPU paths
     * opened.
     */
    Disable();
    ok = Exec_ARMAssignBootMask(GET_THIS_TASK);
    ForeachNode(&SysBase->TaskReady, task)
        ok = ok && Exec_ARMAssignBootMask(task);
    ForeachNode(&SysBase->TaskWait, task)
        ok = ok && Exec_ARMAssignBootMask(task);
    if (ok)
        PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;
    Enable();

    if (!ok)
        bug("[Exec] %s: no memory for task affinity masks\n", __PRETTY_FUNCTION__);

    return TRUE;
}

ADD2INITLIB(Exec_ARMCPUSMPInit, -127)
#endif

ADD2INITLIB(Exec_ARMCPUInit, 0)
