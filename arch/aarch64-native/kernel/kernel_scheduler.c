/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <asm/cpu.h>

//#include <kernel_base.h>
struct KernelBase;
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#include "kernel_cpu.h"

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include <aros/types/spinlock_s.h>

#include <etask.h>

#undef bug
#include "exec_intern.h"
#undef bug
#include "kernel_intern.h"

#define DSCHED(x)

/* When the last CPU-usage window was closed (microseconds). */
static ULONG core_TaskUsageStamp = 0;

#if defined(__AROSEXEC_SMP__)
/* iet_CpuAffinity is a cpumask buffer or the TASKAFFINITY_ANY sentinel,
 * never a raw bitmask. NULL means "run anywhere". */
static inline BOOL core_AffinityMatch(struct Task *t, uint32_t cpumask)
{
    void *aff = (void *)(IPTR)GetIntETask(t)->iet_CpuAffinity;

    if (!aff || (IPTR)aff == TASKAFFINITY_ANY)
        return TRUE;

    return (((uint32_t *)aff)[0] & cpumask) != 0;
}

/* Single attempt: everyone else takes task-lock before list-lock, so
 * blocking here (we hold the list lock) would be an AB-BA deadlock. */
static inline BOOL core_TrySpinLockWrite(spinlock_t *lock)
{
    unsigned int lock_value, result;

    asm volatile(
            "1:     ldaxr   %w0, [%2]       \n\t"   // Load the lock value, gaining exclusive access
            "       cbnz    %w0, 2f         \n\t"   // Taken - fail without spinning
            "       stxr    %w1, %w3, [%2]  \n\t"   // Try to exclusively write the lock value
            "       cbnz    %w1, 1b         \n\t"   // Exclusive access lost - re-examine the lock
            "       b       3f              \n\t"
            "2:     clrex                   \n\t"   // Drop the dangling exclusive monitor
            "3:                             \n\t"
            : "=&r"(lock_value), "=&r"(result)
            : "r"(&lock->lock), "r"(0x80000000)
            : "memory"
    );

    if (lock_value != 0)
        return FALSE;

    lock->s_Owner = GET_THIS_TASK;
    return TRUE;
}
#endif

/* Check if the currently running task on this cpu should be rescheduled */
BOOL core_Schedule(void)
{
#if defined(DEBUG)
    int cpunum = GetCPUNumber();
    (void)cpunum;
#endif
    struct Task *task = GET_THIS_TASK;
    BOOL corereschedule = TRUE;

    DSCHED(bug("[Kernel:%02d] core_Schedule()\n", cpunum));

    FLAG_SCHEDSWITCH_CLEAR;

    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    if (!(task->tc_Flags & TF_EXCEPT))
    {
#if defined(__AROSEXEC_SMP__)
        /* IRQ+FIQ off: the IPI and the timer IRQ take this lock as a
         * write, and re-entering it self-deadlocks this CPU. */
        unsigned int __if = EXEC_IRQFIQ_DISABLE();
        KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                    SPINLOCK_MODE_READ);
#endif
        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            corereschedule = FALSE;
        else
        {
            struct Task *nexttask;
#if defined(__AROSEXEC_SMP__)
            int cpunum = GetCPUNumber();
            uint32_t cpumask = (1 << cpunum);
#endif
            /*
                    If there are tasks ready for this cpu that have equal or lower priority,
                    and the current task has used its alloted time - reschedule so they can run
                */
            for (nexttask = (struct Task *)GetHead(&SysBase->TaskReady); nexttask != NULL; nexttask = (struct Task *)GetSucc(nexttask))
            {
#if defined(__AROSEXEC_SMP__)
                if (core_AffinityMatch(nexttask, cpumask))
                {
#endif
                    if (nexttask->tc_Node.ln_Pri <= task->tc_Node.ln_Pri)
                    {
                        /* If the running task did not used it's whole quantum yet, let it work */
                        if (!FLAG_SCHEDQUANTUM_ISSET)
                            corereschedule = FALSE;
                    }
                    break;
#if defined(__AROSEXEC_SMP__)
                }
#endif
            }
        }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
        EXEC_IRQFIQ_RESTORE(__if);
#endif
    }

    DSCHED
        (
            if (corereschedule)
                bug("[Kernel:%02d] '%s' @ 0x%p needs rescheduled ..\n", cpunum, task->tc_Node.ln_Name, task);
        )

    return corereschedule;
}

/* Switch the currently running task on this cpu to ready state */
void core_Switch(void)
{
#if defined(DEBUG)
    int cpunum = GetCPUNumber();
#endif
    struct Task *task = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    unsigned int __if;
#endif

    DSCHED(bug("[Kernel:%02d] core_Switch(%08x)\n", cpunum, task->tc_State));

    /* Every outgoing task, not only TS_RUN: Wait() switches away in
     * TS_WAIT holding a Disable() level. */
    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    /* Carry the unused slice with the task, or a task preempted more
     * often than the heartbeat ticks never expires its quantum. */
    if (GetIntETask(task))
        GetIntETask(task)->iet_QuantumLeft = SCHEDELAPSED_GET;

    if (task->tc_State == TS_RUN)
    {
        DSCHED(bug("[Kernel:%02d] Switching away from '%s' @ 0x%p\n", cpunum, task->tc_Node.ln_Name, task));
#if defined(__AROSEXEC_SMP__)
        /* tc_SpinLock across the whole RUN->READY (state, list) change, or
         * an observer can see TS_READY while the task is on no list.
         * Task-lock outer, list-locks inner. IRQ+FIQ off: the IPI and the
         * timer IRQ take this same lock. */
        __if = EXEC_IRQFIQ_DISABLE();
        KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
        exec_TaskRemoveRunning(task);
#endif
        task->tc_State = TS_READY;

        /* if the current task has gone out of stack bounds, suspend it to prevent further damage to the system */
        if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
        {
            bug("[Kernel:%02d] '%s' @ 0x%p went out of stack limits\n", cpunum, task->tc_Node.ln_Name, task);
            bug("[Kernel:%02d]  - Lower 0x%p, upper 0x%p, SP 0x%p\n", cpunum, task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);

            task->tc_SigWait    = 0;
            task->tc_State      = TS_WAIT;
#if defined(__AROSEXEC_SMP__)
            exec_TaskEnqueueWait(task);
#else
            Enqueue(&SysBase->TaskWait, &task->tc_Node);
#endif

            Alert(AN_StackProbe);
        }

        if (task->tc_Flags & TF_SWITCH)
            AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));

        if (task->tc_State == TS_READY)
        {
            DSCHED(bug("[Kernel:%02d] Setting '%s' @ 0x%p as ready\n", cpunum, task->tc_Node.ln_Name, task));
#if defined(__AROSEXEC_SMP__)
            exec_TaskEnqueueReady(task);
#else
            Enqueue(&SysBase->TaskReady, &task->tc_Node);
#endif
        }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&task->tc_SpinLock);
        EXEC_IRQFIQ_RESTORE(__if);
#endif
    }
}

/* Dispatch a "new" ready task on this cpu */
struct Task *core_Dispatch(void)
{
    struct Task *newtask;
    struct Task *task = GET_THIS_TASK;
    BOOL taskStateLocked = FALSE;
#if defined(__AROSEXEC_SMP__) || defined(DEBUG)
    int cpunum = GetCPUNumber();
    (void)cpunum;
#endif
#if defined(__AROSEXEC_SMP__)
    uint32_t cpumask = (1 << cpunum);
    BOOL sawContended;
    /* IRQ+FIQ off for the whole dispatch: it holds the scheduler list
     * locks and tc_SpinLock, which the IPI and the timer IRQ also take.
     * Nestable across the recursive call below. */
    unsigned int __if = EXEC_IRQFIQ_DISABLE();
#endif

    DSCHED(bug("[Kernel:%02d] core_Dispatch()\n", cpunum));

#if defined(__AROSEXEC_SMP__)
dispatch_rescan:
    sawContended = FALSE;
    KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#endif
    for (newtask = (struct Task *)GetHead(&SysBase->TaskReady); newtask != NULL; newtask = (struct Task *)GetSucc(newtask))
    {
#if defined(__AROSEXEC_SMP__)
        if (core_AffinityMatch(newtask, cpumask))
        {
            /* Lock before pulling it off the list, so the (state, list)
             * change is atomic to anyone trusting tc_State. */
            if (!core_TrySpinLockWrite(&newtask->tc_SpinLock))
            {
                sawContended = TRUE;
                continue;
            }
            taskStateLocked = TRUE;
            Remove(&newtask->tc_Node);
            break;
        }
#else
        Remove(&newtask->tc_Node);
        break;
#endif
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);

    /* Every candidate was momentarily locked elsewhere - rescan rather
     * than idle with runnable work on the list. */
    if (!newtask && sawContended)
        goto dispatch_rescan;
#endif

    if ((!newtask) && (task) && (task->tc_State != TS_WAIT))
        newtask = task;

    if (newtask != NULL)
    {
        if (newtask->tc_State == TS_READY || newtask->tc_State == TS_RUN)
        {
            DSCHED(bug("[Kernel:%02d] Preparing to run '%s' @ 0x%p\n",
                cpunum, newtask->tc_Node.ln_Name, newtask));

            SysBase->DispCount++;
            IDNESTCOUNT_SET(newtask->tc_IDNestCnt);
            SET_THIS_TASK(newtask);
            /* Only a fresh slice when the task changes or used its own,
             * or a fast-dispatching core never expires a quantum. */
            {
                struct IntETask *iet = GetIntETask(newtask);
                ULONG left = iet ? iet->iet_QuantumLeft : 0;

                SCHEDELAPSED_SET(left ? left : SCHEDQUANTUM_GET);
            }
            FLAG_SCHEDQUANTUM_CLEAR;

            /* Check the stack of the task we are about to launch. */
            if ((newtask->tc_SPReg <= newtask->tc_SPLower) ||
                (newtask->tc_SPReg > newtask->tc_SPUpper))
            {
#if defined(__AROSEXEC_SMP__)
                if (!taskStateLocked)
                {
                    KrnSpinLock(&newtask->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
                    taskStateLocked = TRUE;
                }
#endif
                newtask->tc_State     = TS_WAIT;
            }
            else
                newtask->tc_State     = TS_RUN;
        }

        BOOL launchtask = TRUE;
        if (newtask->tc_State == TS_WAIT)
        {
#if defined(__AROSEXEC_SMP__)
            if (!taskStateLocked)
            {
                KrnSpinLock(&newtask->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
                taskStateLocked = TRUE;
            }
#endif
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL,
                        SPINLOCK_MODE_WRITE);
#endif
            Enqueue(&SysBase->TaskWait, &newtask->tc_Node);
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);
            KrnSpinUnLock(&newtask->tc_SpinLock);
            taskStateLocked = FALSE;
#endif
            launchtask = FALSE;
        }

#if defined(__AROSEXEC_SMP__)
        /* Launch path; the TS_WAIT block already unlocked. */
        if (taskStateLocked)
        {
            KrnSpinUnLock(&newtask->tc_SpinLock);
            taskStateLocked = FALSE;
        }
#endif

        if (!launchtask)
        {
            /* if the new task shouldn't run - force a reschedule */
            DSCHED(bug("[Kernel:%02d] Skipping '%s' @ 0x%p (state %08x)\n", cpunum, newtask->tc_Node.ln_Name, newtask, newtask->tc_State));

            core_Switch();
            newtask = core_Dispatch();
        }
        else
        {
            DSCHED(bug("[Kernel:%02d] Launching '%s' @ 0x%p (state %08x)\n", cpunum, newtask->tc_Node.ln_Name, newtask, newtask->tc_State));
        }
    }
    else
    {
        /* Go idle if there is nothing to do */
        DSCHED(bug("[Kernel:%02d] No ready Task(s) - entering sleep mode\n", cpunum));

        /*
         * Idle counter is incremented every time when we enter here,
         * not only once. This is correct.
         */
        SysBase->IdleCount++;
        FLAG_SCHEDSWITCH_SET;
    }

#if defined(__AROSEXEC_SMP__)
    EXEC_IRQFIQ_RESTORE(__if);
#endif
    return newtask;
}

/*
 * Usage = growth of iet_private2 (cumulative busy us, kept by cpu_Switch)
 * over the window, scaled to 0..0xffffffff. The field is not reset -
 * KATTR_CPULoad reads it too - so keep the previous sample per task.
 * 32-bit maths throughout, for the counter's ~71 minute wrap.
 */
static void core_TaskUsageWindow(struct Task *t, ULONG now)
{
    struct IntETask *iet;
    ULONG busyNow, window;

    if (!(t->tc_Flags & TF_ETASK) || !t->tc_UnionETask.tc_ETask)
        return;

    iet = IntETask(t->tc_UnionETask.tc_ETask);
    busyNow = (ULONG)iet->iet_private2;

    /* A running task has not committed its current slice yet.
     * iet_private1 == 0 means never dispatched. */
    if ((t->tc_State == TS_RUN) && iet->iet_private1)
        busyNow += now - (ULONG)iet->iet_private1;

    window = now - (ULONG)iet->iet_LastUsageStamp;

    if (iet->iet_LastUsageStamp && window)
    {
        ULONG busy = busyNow - (ULONG)iet->iet_LastBusy;

        if (busy >= window)
            iet->iet_CpuUsage = 0xffffffff;
        else
            iet->iet_CpuUsage = (ULONG)(((UQUAD)busy << 32) / window);
    }
    else
        iet->iet_CpuUsage = 0;      /* first sample */

    iet->iet_LastBusy = busyNow;
    iet->iet_LastUsageStamp = now;
}

/*
 * Refresh iet_CpuUsage for TaskTag_CPUUsage. Driven from the VBlank IRQ,
 * not the per-core CNTP heartbeat: that is an FIQ, and the list locks
 * below are also taken by the IPI handler in FIQ context.
 */
void core_TaskCPUUsage(void)
{
    struct Task *t;
    ULONG now;
#if defined(__AROSEXEC_SMP__)
    unsigned int __if;
#endif

    if (!SysBase || !__arm_arosintern.ARMI_GetTime)
        return;

    now = (ULONG)__arm_arosintern.ARMI_GetTime();
    if (core_TaskUsageStamp && ((now - core_TaskUsageStamp) < TASKUSAGE_WINDOW))
        return;
    core_TaskUsageStamp = now;

#if defined(__AROSEXEC_SMP__)
    __if = EXEC_IRQFIQ_DISABLE();

    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, t)
        core_TaskUsageWindow(t, now);
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

    KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, t)
        core_TaskUsageWindow(t, now);
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);

    KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL, SPINLOCK_MODE_READ);
#else
    t = GET_THIS_TASK;
    if (t)
        core_TaskUsageWindow(t, now);
#endif
    ForeachNode(&SysBase->TaskReady, t)
        core_TaskUsageWindow(t, now);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);

    KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    ForeachNode(&SysBase->TaskWait, t)
        core_TaskUsageWindow(t, now);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);

    EXEC_IRQFIQ_RESTORE(__if);
#endif
}
