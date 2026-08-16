/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <asm/arm/cpu.h>

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

#if defined(__AROSEXEC_SMP__)
/*
 * iet_CpuAffinity is a pointer to a cpumask buffer (from KrnAllocCPUMask)
 * or the TASKAFFINITY_ANY sentinel - never a raw bitmask. A NULL pointer
 * means no affinity was assigned, which is treated as "run anywhere".
 * cpunum < 4 so the affinity word index is always 0.
 */
static inline BOOL core_AffinityMatch(struct Task *t, uint32_t cpumask)
{
    void *aff = (void *)(IPTR)GetIntETask(t)->iet_CpuAffinity;

    if (!aff || (IPTR)aff == TASKAFFINITY_ANY)
        return TRUE;

    return (((uint32_t *)aff)[0] & cpumask) != 0;
}

/*
 * Single-attempt write acquire. The dispatcher scans TaskReady holding
 * TaskReadySpinLock and must take each candidate's tc_SpinLock before
 * pulling it off the list - but every other tc_SpinLock user acquires
 * task-lock FIRST, list-lock second, so a blocking acquire here (list
 * before task) would be an AB-BA deadlock. Try once and skip the
 * candidate on contention instead.
 */
static inline BOOL core_TrySpinLockWrite(spinlock_t *lock)
{
    unsigned long lock_value, result;

    asm volatile(
            "1:     ldrex   %0, [%2]        \n\t"   // Load the lock value, gaining exclusive access
            "       teq     %0, #0          \n\t"   // Is the lock free?
            "       bne     2f              \n\t"   // No - fail without spinning
            "       strex   %1, %3, [%2]    \n\t"   // Try to exclusively write the lock value
            "       teq     %1, #0          \n\t"   // Did it succeed?
            "       bne     1b              \n\t"   // Exclusive access lost - re-examine the lock
            "2:     clrex                   \n\t"   // Drop any dangling exclusive monitor
            : "=&r"(lock_value), "=&r"(result)
            : "r"(&lock->lock), "r"(0x80000000)
            : "cc"
    );

    if (lock_value != 0)
        return FALSE;

    dmb();
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
        /* FIQ off: an IPI re-entering this read lock as a write would
         * self-deadlock this CPU. See EXEC_FIQ_DISABLE in exec_platform.h. */
        unsigned int __fiq = EXEC_FIQ_DISABLE();
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
        EXEC_FIQ_RESTORE(__fiq);
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
    unsigned int __fiq;
#endif

    DSCHED(bug("[Kernel:%02d] core_Switch(%08x)\n", cpunum, task->tc_State));

    /* Preserve the Disable() nest count for every outgoing task, not only
     * TS_RUN ones: Wait() switches away in TS_WAIT holding a Disable() level
     * and relies on Switch() to restore it. */
    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    if (task->tc_State == TS_RUN)
    {
        DSCHED(bug("[Kernel:%02d] Switching away from '%s' @ 0x%p\n", cpunum, task->tc_Node.ln_Name, task));
#if defined(__AROSEXEC_SMP__)
        /*
         * Hold tc_SpinLock across the RUN->READY (state, list) transition.
         * Without it, an observer on another CPU (SetTaskPri, ReschedTask)
         * that locks tc_SpinLock can see TS_READY while the task is not
         * yet on TaskReady and Remove() a node that is on no list.
         * Order: task-lock outer, list-locks inner, as everywhere else.
         * FIQ masked for the whole hold: the IPI's signal_hook takes this
         * same tc_SpinLock and would self-deadlock this CPU.
         */
        __fiq = EXEC_FIQ_DISABLE();
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
        EXEC_FIQ_RESTORE(__fiq);
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
    /* FIQ off for the whole dispatch: it holds TaskReadySpinLock,
     * tc_SpinLock, TaskWait/Spinning and (via SET_THIS_TASK) TaskRunning -
     * an IPI re-entering any of these on this CPU self-deadlocks. Nestable
     * across the recursive call below. */
    unsigned int __fiq = EXEC_FIQ_DISABLE();
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
            /*
             * Take tc_SpinLock before pulling the task off TaskReady so
             * the (state, list) transition is atomic to SetTaskPri /
             * Exec_ReschedTask, which trust tc_State under tc_SpinLock.
             * See core_TrySpinLockWrite for why this must be a trylock.
             */
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

    /*
     * Every candidate we saw was locked by another CPU (Signal /
     * SetTaskPri hold it only for a few instructions). Rescan rather
     * than going idle with runnable work on the list.
     */
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
            SCHEDELAPSED_SET(SCHEDQUANTUM_GET);
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
        /* Covers the launch path; the TS_WAIT block has already dropped
         * the lock itself. */
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
    EXEC_FIQ_RESTORE(__fiq);
#endif
    return newtask;
}
