/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
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

/* Linked from kernel.resource,
 * need to retrieve in a cleaner fashion .. */
extern IPTR stack[];

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

#if defined(__AROSEXEC_SMP__)
    if (cpunum == 0)
    {
#endif
        /* for our sanity we will tell exec about the correct stack for the boot task */
        BootTask->tc_SPLower = stack;
        BootTask->tc_SPUpper = stack + AROS_STACKSIZE;
#if defined(__AROSEXEC_SMP__)
    }

    for (cpu = 0; cpu < cpunum; cpu ++)
    {
        taskName = AllocVec(15, MEMF_CLEAR);
        sprintf( taskName, "CPU #%02d Idle", cpu);
        cpuMask = KrnAllocCPUMask();
        if (cpuMask)
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
            /*
             * Tell the kernel which task idles this core, so
             * KrnGetSystemAttr(KATTR_CPULoad + cpu) can derive the core's
             * load from it. Only exec knows the mapping.
             */
            if (cpu < ARM_MAXCPUS)
                arm_IdleTask[cpu] = CPUIdleTask;
#endif
        }
#if defined(__AROSEXEC_SMP__)
    }
#endif

    return TRUE;
}

#if defined(__AROSEXEC_SMP__)
/*
 * NOTE: no TS_SPIN / spinlock-failhook machinery here. arm-native's
 * KrnSpinLock never invokes its failhook parameter, so tasks are never
 * parked in TS_SPIN on this port - spinners just spin. all-pc has the
 * wired-up version (its KrnSpinLock calls the hook).
 */

/*
 * Move task from its current state's list to the list matching newState.
 * Called by krnSysCallReschedTask from rom/exec (signal.c, newaddtask.c,
 * remtask.c, service.c). On arm-native this is a direct function call,
 * not a syscall - the work is just list mutation under spinlocks.
 *
 * Callers MUST NOT hold tc_SpinLock - we take it ourselves so the
 * (state-read -> list-lock-pick -> list-mutate -> state-write) sequence
 * is atomic from any other observer that takes tc_SpinLock. Without this
 * a concurrent migrator could see a stale tc_State, acquire the wrong
 * source list-lock, and Remove from a list the task is not on.
 *
 * Lock order: tc_SpinLock outer, list-locks inner. Matches wait.c's
 * TS_WAIT migration and the exec_TaskRemove / exec_TaskEnqueue helpers.
 */
void Exec_ReschedTask(struct Task *task, ULONG newState)
{
    spinlock_t *fromLock = NULL;
    /*
     * Raw IRQ+FIQ off for the whole function. tc_SpinLock is held across it
     * (plus fromLock and the enqueue lists); an IPI (FIQ) or the IRQ-exit
     * dispatcher re-entering these on this CPU self-deadlocks. We use raw
     * masking, not Disable()/Enable(): this runs from the FIQ/IPI handler
     * too, where Disable()'s KrnCli svc would nest an exception on the
     * FIQ-handler SVC stack. See EXEC_IRQFIQ_DISABLE in exec_platform.h.
     */
    unsigned int __if = EXEC_IRQFIQ_DISABLE();

    Kernel_52_KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE, NULL);

    if (newState == TS_READY)
    {
        /*
         * Wake-up semantics: only migrate a task that is actually parked
         * (TS_WAIT) or being added (TS_INVALID from TaskLaunch).
         * Callers check the state under tc_SpinLock but must drop it
         * before calling us, so by now another CPU may already have won
         * the wake race and made the task READY - or dispatched it
         * (TS_RUN), or torn it down (terminal states). Migrating in any
         * of those cases would double-dispatch a running task or
         * resurrect a corpse. The signal bits are already set, so a
         * running/ready task will see them - just do nothing.
         */
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
            /* TS_INVALID, TS_ADDED, TS_REMOVED, TS_TOMBSTONED, TS_EXCEPT:
             * task is not on a standard scheduler list. Caller (Signal)
             * also pre-filters out terminal states under tc_SpinLock so
             * we should not normally reach here with such a state. */
            break;
    }

    if (fromLock)
    {
        /* IRQ+FIQ already masked for the whole function (see top). */
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

/*
 * krnSysCallSwitch() for RemTask's self-removal path. The caller (the
 * task removing itself) has already set TS_REMOVED. Detach it from the
 * TaskRunning list under the list lock and mark it TS_TOMBSTONED so the
 * Exec service task reclaims it, then return - unlike KrnSwitch() this
 * does NOT dispatch, so RemTask resumes on the same context to post
 * itself to the service port and only gives up the CPU at its final
 * KrnDispatch().
 */
void Exec_SuicideSwitch(void)
{
    struct Task *task = GET_THIS_TASK;
    unsigned int __fiq = EXEC_FIQ_DISABLE();

    Disable();
    /*
     * tc_SpinLock outer, list lock inner (the usual order): the state
     * write must be atomic with the list removal for observers that
     * check tc_State under tc_SpinLock (Signal's terminal-state check,
     * Exec_ReschedTask's wake filter).
     */
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

int Exec_ARMCPUSMPInit(struct ExecBase *SysBase)
{
    /*
     * Turn on CPU affinity routing in arch-neutral exec. Without this,
     * signal.c's cross-CPU paths (KrnScheduleCPU on TS_RUN, the early
     * core_DoCallIPI tunnel for non-affine CPUs) never fire and remote
     * task wakes silently fail. The cpumask helpers (alloc/get/in/
     * clear/free) and core_DoCallIPI must already be functional - this
     * flag is the gate that exposes them to signal.c and newaddtask.c.
     */
    PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;

    /*
     * Pin the primordial boot/init task to the boot CPU. It was created
     * (exec_init.c) before EXECF_CPUAffinity was set, so InitETask left it
     * with a NULL affinity == "run anywhere". That lets a secondary core
     * dispatch it mid-init, but the single-threaded coldstart sequence is
     * not migration-safe (it holds boot-CPU/boot-stack-local state, e.g.
     * stack-resident hooks), so running it elsewhere crashes. Keep it on
     * the boot CPU (always logical 0); other tasks distribute normally.
     */
    {
        struct Task *bootTask = GET_THIS_TASK;
        struct IntETask *iet = bootTask ? (struct IntETask *)GetETask(bootTask) : NULL;

        if (iet && !iet->iet_CpuAffinity)
        {
            void *aff = KrnAllocCPUMask();
            if (aff)
            {
                KrnGetCPUMask(0, aff);
                iet->iet_CpuAffinity = aff;
                iet->iet_CpuNumber = 0;
            }
        }
    }

    return TRUE;
}

ADD2INITLIB(Exec_ARMCPUSMPInit, -127)
#endif

ADD2INITLIB(Exec_ARMCPUInit, 0)
