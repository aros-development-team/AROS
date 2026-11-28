/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: Task scheduler for native RISC-V kernels.

    Uniprocessor builds use the generic scheduler unchanged. SMP builds
    use this port of the aarch64-native scheduler: the ready list is
    shared, each CPU only dispatches tasks whose affinity includes it,
    and a task's state and the list it is on change together, in kernel
    context, under the lock of that list (as on all-pc).
*/

#include <aros/config.h>

#if !defined(__AROSEXEC_SMP__)

#include "../../../rom/kernel/kernel_scheduler.c"

#else /* __AROSEXEC_SMP__ */

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>

#include "kernel_cpu.h"
#include "kernel_riscv.h"
#include "kernel_spinlock.h"

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include <aros/types/spinlock_s.h>

#include <etask.h>

#define __AROS_KERNEL__
#include "exec_intern.h"

#define DSCHED(x)

extern spinlock_t *Kernel_52_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
extern void Kernel_53_KrnSpinUnLock(spinlock_t *, void *);

extern int AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)(void);

#undef KrnSpinLock
#undef KrnSpinUnLock
#define KrnSpinLock(lock, hook, mode)   Kernel_52_KrnSpinLock((lock), (hook), (mode), NULL)
#define KrnSpinUnLock(lock)             Kernel_53_KrnSpinUnLock((lock), NULL)

/*
 * iet_CpuAffinity is a mask buffer or the TASKAFFINITY_ANY sentinel.
 * NULL is not a valid mask and matches no CPU - but masks are only
 * meaningful once exec has assigned one to every task and set
 * EXECF_CPUAffinity (exec_smp.c); until then only the boot CPU runs.
 */
static inline BOOL core_AffinityMatch(struct Task *t, int cpunum)
{
    void *aff;

    if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity))
        return TRUE;

    if (!(t->tc_Flags & TF_ETASK) || !t->tc_UnionETask.tc_ETask)
        return FALSE;

    aff = (void *)(IPTR)GetIntETask(t)->iet_CpuAffinity;
    if ((IPTR)aff == TASKAFFINITY_ANY)
        return TRUE;
    if (!aff || ((IPTR)aff == TASKAFFINITY_ALL_BUT_SELF))
        return FALSE;

    return (((ULONG *)aff)[cpunum / 32] & (1UL << (cpunum % 32))) != 0;
}

/* Runlist spinlock guarding a task in this state; NULL if it has no runlist. */
static inline spinlock_t *core_TaskListSpinLock(ULONG state)
{
    switch (state)
    {
        case TS_RUN:
            return &PrivExecBase(SysBase)->TaskRunningSpinLock;
        case TS_READY:
            return &PrivExecBase(SysBase)->TaskReadySpinLock;
        case TS_WAIT:
            return &PrivExecBase(SysBase)->TaskWaitSpinLock;
        case TS_SPIN:
            return &PrivExecBase(SysBase)->TaskSpinningLock;
        default:
            return NULL;
    }
}

/* Check if the currently running task on this cpu should be rescheduled */
BOOL core_Schedule(void)
{
    int cpunum = krnRVCPUNumber();
    struct Task *task = GET_THIS_TASK;
    BOOL corereschedule = TRUE;

    DSCHED(bug("[Kernel:%02d] core_Schedule()\n", cpunum));

    FLAG_SCHEDSWITCH_CLEAR;

    /* The last task was evicted: there is none to schedule away from */
    if (FLAG_SCHEDEVICT_ISSET)
        return FALSE;

    if ((task->tc_State == TS_TOMBSTONED) || (task->tc_State == TS_REMOVED))
    {
        /* always let finalising tasks finish... */
        corereschedule = FALSE;
    }
    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    else if (!(task->tc_Flags & TF_EXCEPT))
    {
        /* The run list locks are taken with interrupts off, everywhere */
        unsigned int __if = EXEC_IRQFIQ_DISABLE();

        KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                    SPINLOCK_MODE_READ);

        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            corereschedule = FALSE;
        else
        {
            struct Task *nexttask;
            /*
                    If there are tasks ready for this cpu that have equal or lower priority,
                    and the current task has used its alloted time - reschedule so they can run
                */
            for (nexttask = (struct Task *)GetHead(&SysBase->TaskReady); nexttask != NULL; nexttask = (struct Task *)GetSucc(nexttask))
            {
                if (core_AffinityMatch(nexttask, cpunum))
                {
                    if ((task->tc_State != TS_SPIN) &&
                        (nexttask->tc_Node.ln_Pri <= task->tc_Node.ln_Pri))
                    {
                        /* If the running task did not used it's whole quantum yet, let it work */
                        if (!FLAG_SCHEDQUANTUM_ISSET)
                            corereschedule = FALSE;
                    }
                    break;
                }
            }
        }

        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
        EXEC_IRQFIQ_RESTORE(__if);
    }

    /* A task that spins for a lock and is not switched away simply tries again */
    if ((!corereschedule) && (task->tc_State == TS_SPIN))
        task->tc_State = TS_RUN;

    DSCHED
        (
            if (corereschedule)
                bug("[Kernel:%02d] '%s' @ 0x%p needs rescheduled ..\n", cpunum, task->tc_Node.ln_Name, task);
        )

    return corereschedule;
}

/*
    Switch the currently running task on this cpu:
    if it is running, switch it to the ready state;
    if Wait() asked for it to be parked, to the wait state;
    if exec has set TS_SPIN, to the spinning list.

    A task's state and the list it is on change together, under the locks
    of both lists. The locks are always taken in the order TaskRunning,
    TaskReady, TaskWait, TaskSpinning, so no CPU ever finds a state that
    names a list the task is not on.
*/
void core_Switch(void)
{
    int cpunum = krnRVCPUNumber();
    struct Task *task = GET_THIS_TASK;
    ULONG showAlert = 0;
    ULONG newState;
    spinlock_t *toLock;
    struct List *toList;
    BOOL park;
    unsigned int __if;

    (void)cpunum;
    DSCHED(bug("[Kernel:%02d] core_Switch(%08x)\n", cpunum, task->tc_State));

    /* Wait() asked for the task to be parked. The request is for this switch only. */
    park = FLAG_SCHEDPARK_ISSET;
    if (park)
        FLAG_SCHEDPARK_CLEAR;

    /* The last task was evicted, and may be freed: nothing to switch from */
    if (FLAG_SCHEDEVICT_ISSET)
        return;

    if ((!task) || (task->tc_State == TS_INVALID) || (task->tc_State == TS_TOMBSTONED))
    {
        /* Nothing to switch away from, or already off every list */
        return;
    }

    /* Every outgoing task: Wait() switches away holding a Disable() level. */
    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    /* Carry the unused slice with the task, or a task preempted more
     * often than the heartbeat ticks never expires its quantum. */
    if (GetIntETask(task))
        GetIntETask(task)->iet_QuantumLeft = SCHEDELAPSED_GET;

    DSCHED(bug("[Kernel:%02d] Switching away from '%s' @ 0x%p\n", cpunum, task->tc_Node.ln_Name, task));

    /* The task's own hook is called with no lock held */
    if ((task->tc_State == TS_RUN) && !park && (task->tc_Flags & TF_SWITCH))
        AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));

    /* The run list locks are taken with interrupts off, everywhere */
    __if = EXEC_IRQFIQ_DISABLE();

    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                SPINLOCK_MODE_WRITE);

    /* Under the lock of the list it is on, the state cannot change any more */
    newState = task->tc_State;
    if ((newState == TS_TOMBSTONED) || (newState == TS_INVALID))
    {
        /* Removed by another CPU in the meantime, and off TaskRunning */
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
        EXEC_IRQFIQ_RESTORE(__if);
        return;
    }

    if (newState == TS_REMOVED)
        newState = TS_TOMBSTONED;
    else if (park && (newState == TS_RUN))
        newState = TS_WAIT;
    else if ((newState != TS_WAIT) && (newState != TS_SPIN))
        newState = TS_READY;

    /* if the current task has gone out of stack bounds, suspend it to prevent further damage to the system */
    if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
    {
        bug("[Kernel:%02d] '%s' @ 0x%p went out of stack limits\n", cpunum, task->tc_Node.ln_Name, task);
        bug("[Kernel:%02d]  - Lower 0x%p, upper 0x%p, SP 0x%p\n", cpunum, task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);

        if (newState != TS_TOMBSTONED)
        {
            task->tc_SigWait    = 0;
            newState            = TS_WAIT;
            showAlert           = AN_StackProbe;
        }
    }

    switch (newState)
    {
        case TS_READY:
            toLock = &PrivExecBase(SysBase)->TaskReadySpinLock;
            toList = &SysBase->TaskReady;
            break;
        case TS_WAIT:
            toLock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
            toList = &SysBase->TaskWait;
            break;
        case TS_SPIN:
            toLock = &PrivExecBase(SysBase)->TaskSpinningLock;
            toList = &PrivExecBase(SysBase)->TaskSpinning;
            break;
        default:
            toLock = NULL;
            toList = NULL;
            break;
    }

    Remove(&task->tc_Node);

    if (toLock)
    {
        KrnSpinLock(toLock, NULL, SPINLOCK_MODE_WRITE);
        task->tc_State = newState;
        Enqueue(toList, &task->tc_Node);
        KrnSpinUnLock(toLock);
    }
    else
    {
        /* The task is on no list any more */
        task->tc_State = newState;
    }

    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

    EXEC_IRQFIQ_RESTORE(__if);

    if (showAlert)
        Alert(showAlert);
}

/* Dispatch a "new" ready task on this cpu */
struct Task *core_Dispatch(void)
{
    struct Task *newtask;
    struct Task *task = GET_THIS_TASK;
    BOOL fromReady = FALSE;
    int cpunum = krnRVCPUNumber();
    /* The run list locks are taken with interrupts off, everywhere.
     * Nestable across the recursive call below. */
    unsigned int __if = EXEC_IRQFIQ_DISABLE();

    DSCHED(bug("[Kernel:%02d] core_Dispatch()\n", cpunum));

    /*
     * The task leaves TaskReady, becomes TS_RUN and is put on TaskRunning
     * in one step, under both locks. From then on it belongs to this CPU,
     * and other CPUs send their requests for it here, so they are told
     * where "here" is before they can see TS_RUN.
     */
    /* An evicted task is not looked at again: its memory may be gone */
    if (FLAG_SCHEDEVICT_ISSET)
        task = NULL;

    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                SPINLOCK_MODE_WRITE);
    KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);

    for (newtask = (struct Task *)GetHead(&SysBase->TaskReady); newtask != NULL; newtask = (struct Task *)GetSucc(newtask))
    {
        if (core_AffinityMatch(newtask, cpunum))
        {
            Remove(&newtask->tc_Node);
            if (GetIntETask(newtask))
                GetIntETask(newtask)->iet_CpuNumber = cpunum;
            __atomic_store_n(&newtask->tc_State, TS_RUN, __ATOMIC_RELEASE);
            SET_THIS_TASK_LOCKED(newtask);
            FLAG_SCHEDEVICT_CLEAR;
            fromReady = TRUE;
            break;
        }
    }

    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

    /*
     * Nothing on TaskReady for this CPU: carry on with the outgoing task
     * if it is still ours. A task that core_Switch() put on TaskReady or
     * TaskWait is on a list other CPUs take from; if one of them runs it
     * by now it is TS_RUN again, but with that CPU's number.
     */
    if ((!newtask) && (task) && (task->tc_State != TS_WAIT) && (task->tc_State != TS_READY) &&
        (task->tc_State != TS_INVALID) &&
        ((task->tc_State != TS_RUN) || !GetIntETask(task) ||
         (GetIntETask(task)->iet_CpuNumber == cpunum)))
    {
        if (task->tc_State == TS_SPIN)
        {
            /* It has nothing to wait for but the lock: let it try again */
            KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                        SPINLOCK_MODE_WRITE);
            KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                        SPINLOCK_MODE_WRITE);
            if (task->tc_State == TS_SPIN)
            {
                Remove(&task->tc_Node);
                task->tc_State = TS_RUN;
                SET_THIS_TASK_LOCKED(task);
                fromReady = TRUE;
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
        }
        newtask = task;
    }

    if (newtask != NULL)
    {
        BOOL launchtask = FALSE;

        if (fromReady)
        {
            DSCHED(bug("[Kernel:%02d] Preparing to run '%s' @ 0x%p\n",
                cpunum, newtask->tc_Node.ln_Name, newtask));

            SysBase->DispCount++;
            IDNESTCOUNT_SET(newtask->tc_IDNestCnt);
            /* Only a fresh slice when the task changes or used its own,
             * or a fast-dispatching core never expires a quantum. */
            {
                struct IntETask *iet = GetIntETask(newtask);
                ULONG left = iet ? iet->iet_QuantumLeft : 0;

                SCHEDELAPSED_SET(left ? left : SCHEDQUANTUM_GET);
            }
            FLAG_SCHEDQUANTUM_CLEAR;
        }

        if (newtask->tc_State == TS_RUN)
        {
            /*
             * Check the stack of the task we are about to launch. A task
             * that fails is this CPU's running task by now; core_Switch(),
             * called below, makes the same test and puts it to wait.
             */
            if ((newtask->tc_SPReg > newtask->tc_SPLower) &&
                (newtask->tc_SPReg <= newtask->tc_SPUpper))
                launchtask = TRUE;
        }
        else if ((newtask->tc_State == TS_TOMBSTONED) || (newtask->tc_State == TS_REMOVED))
        {
            /* A task on its way out finishes on this CPU */
            launchtask = TRUE;
        }

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

    EXEC_IRQFIQ_RESTORE(__if);

    return newtask;
}

/* Position of a run list lock in the order they are taken in; -1 for none */
static inline int core_TaskListOrder(ULONG state)
{
    switch (state)
    {
        case TS_RUN:
            return 0;
        case TS_READY:
            return 1;
        case TS_WAIT:
            return 2;
        case TS_SPIN:
            return 3;
        default:
            return -1;
    }
}

/*
 * krnSysCallReschedTask(): move task to the list for newState.
 *
 * This runs in kernel context with interrupts off, which is where the run
 * lists are changed. A task's state and the list it is on change together,
 * under the locks of both lists, taken in the order TaskRunning, TaskReady,
 * TaskWait, TaskSpinning. The state says which list the task is on, and it
 * can change until that list's lock is held; so the locks are taken and the
 * state read again, and if it is not what it was they are dropped and the
 * whole test made again. Another CPU may have woken the task in the
 * meantime, and then there is nothing left to do.
 */
void core_ReschedTask(struct Task *task, ULONG newState)
{
    spinlock_t *fromLock, *toLock, *first, *second;
    struct List *toList;
    ULONG oldState;
    BOOL unspin = FALSE;
    unsigned int __if;

    if (newState == TS_UNSPIN)
    {
        newState = TS_READY;
        unspin = TRUE;
    }

    switch (newState)
    {
        case TS_READY:
            toLock = &PrivExecBase(SysBase)->TaskReadySpinLock;
            toList = &SysBase->TaskReady;
            break;
        case TS_SPIN:
            toLock = &PrivExecBase(SysBase)->TaskSpinningLock;
            toList = &PrivExecBase(SysBase)->TaskSpinning;
            break;
        case TS_REMOVED:
            toLock = NULL;
            toList = NULL;
            break;
        default:
            bug("[Kernel] %s: unexpected state %u for task 0x%p, ignored\n",
                __func__, (unsigned)newState, task);
            return;
    }

    __if = EXEC_IRQFIQ_DISABLE();

    for (;;)
    {
        BOOL move = FALSE;

        oldState = task->tc_State;

        if (newState == TS_READY)
        {
            /* Only a waiting, spinning or new task is made ready. */
            move = (oldState == TS_WAIT) || (oldState == TS_INVALID) || (oldState == TS_ADDED) ||
                   (unspin && (oldState == TS_SPIN));
        }
        else if (newState == TS_SPIN)
        {
            /* Only a running task goes to the spinning list */
            move = (oldState == TS_RUN);
        }
        else
        {
            /* Not one that is on its way out already, and on no list */
            move = (oldState != TS_REMOVED) && (oldState != TS_TOMBSTONED);
        }

        if (!move)
        {
            EXEC_IRQFIQ_RESTORE(__if);
            return;
        }

        fromLock = core_TaskListSpinLock(oldState);

        /* Both locks, in their order */
        first = fromLock;
        second = toLock;
        if (fromLock && toLock && (core_TaskListOrder(newState) < core_TaskListOrder(oldState)))
        {
            first = toLock;
            second = fromLock;
        }
        if (first == second)
            second = NULL;

        if (first)
            KrnSpinLock(first, NULL, SPINLOCK_MODE_WRITE);
        if (second)
            KrnSpinLock(second, NULL, SPINLOCK_MODE_WRITE);

        /*
         * A task on no list (a new one) is known to its creator alone.
         * Otherwise it is on the list its state names, and stays there
         * while that list's lock is held.
         */
        if (!fromLock || (task->tc_State == oldState))
            break;

        if (second)
            KrnSpinUnLock(second);
        if (first)
            KrnSpinUnLock(first);
    }

    if (!toList && (newState == TS_REMOVED) && (oldState == TS_RUN) &&
        (task != GET_THIS_TASK) && GetIntETask(task))
    {
        /*
         * It is running on another CPU, and only that CPU can take it off
         * itself. It stays on TaskRunning, marked: the CPU drops it the
         * next time it is about to return to it (core_EvictTask(),
         * rom/kernel/kernel_intr.c), and then makes it TS_INVALID.
         * RemTask() asks that CPU to reschedule and waits for that.
         */
        GetIntETask(task)->iet_Evict = TRUE;
        __atomic_store_n(&task->tc_State, TS_REMOVED, __ATOMIC_RELEASE);

        if (second)
            KrnSpinUnLock(second);
        if (first)
            KrnSpinUnLock(first);

        EXEC_IRQFIQ_RESTORE(__if);
        return;
    }

    if (fromLock)
        Remove(&task->tc_Node);

    if (toList)
    {
        task->tc_State = newState;
        Enqueue(toList, &task->tc_Node);
    }
    else
    {
        /* TS_REMOVED: on no list from here */
        task->tc_State = TS_TOMBSTONED;
    }

    if (second)
        KrnSpinUnLock(second);
    if (first)
        KrnSpinUnLock(first);

    EXEC_IRQFIQ_RESTORE(__if);
}

/*
 * What exec calls for krnSysCallReschedTask() and krnSysCallSwitch(). The
 * run lists are changed in kernel context only, so a task gets there with
 * a system call. A caller that is in kernel context already, the IPI hook
 * that carries out a Signal() for instance, makes the move directly.
 */
void core_SysCallReschedTask(struct Task *task, ULONG state)
{
    if (AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)())
        core_ReschedTask(task, state);
    else
        krnSysCall2(SC_RESCHEDTASK, task, state);
}

/* RemTask() removing the calling task: switch it away, do not dispatch */
void core_SysCallRemSwitch(void)
{
    krnSysCall(SC_REMSWITCH);
}

#endif /* __AROSEXEC_SMP__ */
