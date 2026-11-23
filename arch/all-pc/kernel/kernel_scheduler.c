/*
    Copyright (C) 2017-2023, The AROS Development Team. All rights reserved.
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <proto/exec.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#include "kernel_cpu.h"

#include <exec_platform.h>

#include <aros/types/spinlock_s.h>

#include <etask.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "apic.h"

#define LOWSTACKWARN
#define SCHEDULERASCII_DEBUG

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

#if (DEBUG > 0)
#define DSCHED(x) x
#else
#define DSCHED(x)
#endif

#if (DEBUG > 0) && defined(SCHEDULERASCII_DEBUG)
#define DEBUGCOLOR_SET       "\033[31m"
#define DEBUGFUNCCOLOR_SET   "\033[31;1m"
#define DEBUGCOLOR_RESET     "\033[0m"
#else
#define DEBUGCOLOR_SET
#define DEBUGFUNCCOLOR_SET
#define DEBUGCOLOR_RESET
#endif

#if defined(__AROSEXEC_SMP__)
void core_InitScheduleData(struct X86SchedulerPrivate *schedData)
{
    DSCHED(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s(0x%p)" DEBUGCOLOR_RESET "\n", __func__, schedData);)
    schedData->Granularity = SCHEDGRAN_VALUE;
    schedData->Quantum = SCHEDQUANTUM_VALUE;
}
#endif

/* Check if the currently running task on this cpu should be rescheduled.. */
BOOL core_Schedule(void)
{
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuid_t cpuNo;
#endif
    struct Task *task;
    BOOL corereschedule = TRUE;

    DSCHED(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", __func__);)

    task = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuNo = KrnGetCPUNumber();
#endif

    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: running Task @ 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: '%s', state %08X" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task->tc_State);
    )

    FLAG_SCHEDSWITCH_CLEAR;

    if (task)
    {
        if (
#if defined(__AROSEXEC_SMP__)
            (task->tc_State == TS_TOMBSTONED) ||
#endif
            (task->tc_State == TS_REMOVED))
        {
            /* always let finalising tasks finish... */
            corereschedule = FALSE;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: letting finalising task run.." DEBUGCOLOR_RESET "\n", cpuNo, __func__);
#endif
        }
        else if (!(task->tc_Flags & TF_EXCEPT))
        {
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                        SPINLOCK_MODE_READ);
#endif
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
#if defined(__AROSEXEC_SMP__)
                    if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) || (GetIntETask(nexttask) && core_APIC_CPUInMask(cpuNo, GetIntETask(nexttask)->iet_CpuAffinity)))
                    {
#endif
                     if (
#if defined(__AROSEXEC_SMP__)
                            (task->tc_State != TS_SPIN) &&

#endif
                            (nexttask->tc_Node.ln_Pri <= task->tc_Node.ln_Pri))
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
#endif
        }

#if defined(__AROSEXEC_SMP__)
        if ((!corereschedule) && (task->tc_State == TS_SPIN))
            task->tc_State = TS_RUN;
#endif
    }

    DSCHED
        (
            if (corereschedule)
                bug("[Kernel:%03u]" DEBUGCOLOR_SET " '%s' @ 0x%p needs rescheduled .." DEBUGCOLOR_RESET "\n", cpuNo, task->tc_Node.ln_Name, task);
        )

    return corereschedule;
}

/*
    Switch the currently running task on this cpu
    if it is running , switch it to the ready state
    on SMP builds, if exec has set TS_SPIN,
    switch it to the spinning list.
*/
void core_Switch(void)
{
    cpuid_t cpuNo;
    struct Task *task;
    ULONG showAlert = 0;
    ULONG newState;
    BOOL doSwitch = TRUE;
#if defined(__AROSEXEC_SMP__)
    BOOL park;
#endif

    DSCHED(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", __func__);)

    cpuNo = KrnGetCPUNumber();
    task = GET_THIS_TASK;

#if defined(__AROSEXEC_SMP__)
    /* Wait() asked for the task to be parked. The request is for this switch only. */
    park = FLAG_SCHEDPARK_ISSET;
    if (park)
        FLAG_SCHEDPARK_CLEAR;
#endif

    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Current running Task @ 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task);
    )
    if ((!task) || (task->tc_State == TS_INVALID))
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: called on invalid task!" DEBUGCOLOR_RESET "\n", cpuNo, __func__);
        doSwitch = FALSE;
    }
    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Task name '%s'" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Task state = %08X" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_State);
    )
#if defined(__AROSEXEC_SMP__)
    if (doSwitch && task->tc_State == TS_TOMBSTONED)
        doSwitch = FALSE;
#endif
    if (!doSwitch)
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Letting Task continue to run.." DEBUGCOLOR_RESET "\n", cpuNo, __func__);
        return;
    }

    DSCHED(
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Switching away from Task" DEBUGCOLOR_RESET "\n", cpuNo, __func__);
    )

    /*
     * On SMP builds a task's state and the list it is on change together,
     * under the locks of both lists. The locks are always taken in the
     * order TaskRunning, TaskReady, TaskWait, TaskSpinning, so no CPU ever
     * finds a state that names a list the task is not on. The task's own
     * hook is called first, with no lock held.
     */
#if defined(__AROSEXEC_SMP__)
    if ((task->tc_State == TS_RUN) && !park && (task->tc_Flags & TF_SWITCH) &&
        !(task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper))
        AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));

    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                SPINLOCK_MODE_WRITE);

    /* Under the lock of the list it is on, the state cannot change any more */
    if ((task->tc_State == TS_TOMBSTONED) || (task->tc_State == TS_INVALID))
    {
        /* Removed by another CPU in the meantime, and off TaskRunning */
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
        return;
    }
#else
    if (task->tc_State != TS_RUN && task->tc_State != TS_REMOVED)
#endif
    REMOVE(&task->tc_Node);

    newState = task->tc_State;
#if defined(__AROSEXEC_SMP__)
    if (newState == TS_REMOVED)
        newState = TS_TOMBSTONED;
    else if (park && (newState == TS_RUN))
        newState = TS_WAIT;
#endif

    DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Task removed from list, state = %08X" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newState);)

    if ((newState != TS_WAIT) &&
#if defined(__AROSEXEC_SMP__)
        (newState != TS_SPIN) &&
        (newState != TS_TOMBSTONED) &&
#endif
        (newState != TS_REMOVED))
        newState = TS_READY;

    /* if the current task has gone out of stack bounds, suspend it to prevent further damage to the system */
    if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " ERROR! Task (%s) went out of stack limits" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_Node.ln_Name);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET "  - Lower Bound = 0x%p, Upper Bound = 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_SPLower, task->tc_SPUpper);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET "  - SP = 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_SPReg);

        task->tc_SigWait    = 0;
        newState            = TS_WAIT;

        showAlert = AN_StackProbe;
#if defined(LOWSTACKWARN)
    }
    else if (task->tc_SPReg <= task->tc_SPLower + 1024)
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " WARNING - Task (%s) close to stack limits" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_Node.ln_Name);
#endif
    }

    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    if (newState == TS_READY)
    {
#if !defined(__AROSEXEC_SMP__)
        if (task->tc_Flags & TF_SWITCH)
            AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));
#endif

        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p as ready" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#endif
        task->tc_State = TS_READY;
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    else if (newState == TS_SPIN)
    {
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p to spin" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                    SPINLOCK_MODE_WRITE);
        task->tc_State = TS_SPIN;
        Enqueue(&PrivExecBase(SysBase)->TaskSpinning, &task->tc_Node);
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
    }
#endif
    else if(
#if defined(__AROSEXEC_SMP__)
        (newState != TS_TOMBSTONED) &&
#endif
        (newState != TS_REMOVED))
    {
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p to wait" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL,
                    SPINLOCK_MODE_WRITE);
#endif
        task->tc_State = TS_WAIT;
        Enqueue(&SysBase->TaskWait, &task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);
#endif
    }
    else
    {
        /* The task is on no list any more */
        task->tc_State = newState;
    }

#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
#endif

    if (showAlert)
        Alert(showAlert);
}

/* Dispatch a "new" ready task on this cpu */
struct Task *core_Dispatch(void)
{
    struct Task *newtask;
    struct Task *task = GET_THIS_TASK;
    BOOL fromReady = FALSE;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuid_t cpuNo = KrnGetCPUNumber();
#endif

    DSCHED(bug("[Kernel:%03u]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", cpuNo, __func__);)

#if defined(__AROSEXEC_SMP__)
    /*
     * The task leaves TaskReady, becomes TS_RUN and is put on TaskRunning
     * in one step, under both locks. From then on it belongs to this CPU,
     * and other CPUs send their requests for it here, so they are told
     * where "here" is before they can see TS_RUN.
     */
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                SPINLOCK_MODE_WRITE);
    KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#endif
    for (newtask = (struct Task *)GetHead(&SysBase->TaskReady); newtask != NULL; newtask = (struct Task *)GetSucc(newtask))
    {
#if defined(__AROSEXEC_SMP__)
        if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) || (GetIntETask(newtask) && core_APIC_CPUInMask(cpuNo, GetIntETask(newtask)->iet_CpuAffinity)))
        {
#endif
            REMOVE(&newtask->tc_Node);
#if defined(__AROSEXEC_SMP__)
            if (GetIntETask(newtask))
                GetIntETask(newtask)->iet_CpuNumber = cpuNo;
            __atomic_store_n(&newtask->tc_State, TS_RUN, __ATOMIC_RELEASE);
            SET_THIS_TASK_LOCKED(newtask);
            fromReady = TRUE;
#endif
            break;
#if defined(__AROSEXEC_SMP__)
        }
#endif
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
#endif

    /*
     * Nothing on TaskReady for this CPU: carry on with the outgoing task if
     * it is still ours. On SMP builds a task that core_Switch() put on
     * TaskReady or TaskWait is on a list other CPUs take from; if one of
     * them runs it by now it is TS_RUN again, but with that CPU's number.
     */
    if ((!newtask) && (task) && (task->tc_State != TS_WAIT)
#if defined(__AROSEXEC_SMP__)
        && (task->tc_State != TS_READY) && (task->tc_State != TS_INVALID)
        && ((task->tc_State != TS_RUN) || !GetIntETask(task) ||
            (GetIntETask(task)->iet_CpuNumber == cpuNo))
#endif
        )
    {
#if defined(__AROSEXEC_SMP__)
        if (task->tc_State == TS_SPIN)
        {
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Waking spinning task '%s' @ 0x%p" DEBUGCOLOR_RESET "\n",
                cpuNo, __func__, task->tc_Node.ln_Name, task);)
            /* It has nothing to wait for but the lock: let it try again */
            KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                    SPINLOCK_MODE_WRITE);
            KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                    SPINLOCK_MODE_WRITE);
            if (task->tc_State == TS_SPIN)
            {
                REMOVE(&task->tc_Node);
                task->tc_State = TS_RUN;
                SET_THIS_TASK_LOCKED(task);
                fromReady = TRUE;
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
        }
#endif
        newtask = task;
    }

    if (newtask != NULL)
    {
        BOOL launchtask = FALSE, sleeptask = FALSE;

        if (fromReady || (newtask->tc_State == TS_READY))
        {
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Preparing to run '%s' @ 0x%p" DEBUGCOLOR_RESET "\n",
                cpuNo, __func__, newtask->tc_Node.ln_Name, newtask);)

            IDNESTCOUNT_SET(newtask->tc_IDNestCnt);
#if !defined(__AROSEXEC_SMP__)
            SET_THIS_TASK(newtask);
#endif
            SCHEDELAPSED_SET(SCHEDQUANTUM_GET);
            FLAG_SCHEDQUANTUM_CLEAR;
        }
        if  ((newtask->tc_State == TS_READY) || (newtask->tc_State == TS_RUN))
        {
            /* Check the stack of the task we are about to launch. */
            if ((newtask->tc_SPReg <= newtask->tc_SPLower) ||
                (newtask->tc_SPReg > newtask->tc_SPUpper))
            {
#if !defined(__AROSEXEC_SMP__)
                newtask->tc_State     = TS_WAIT;
                sleeptask = TRUE;
#endif
                /*
                 * On SMP builds the task is this CPU's running task by now.
                 * core_Switch(), called below, makes the same test and puts
                 * it to wait.
                 */
            }
            else
            {
                newtask->tc_State     = TS_RUN;
                launchtask = TRUE;
            }
        }
        else if (
#if defined(__AROSEXEC_SMP__)
            (task->tc_State == TS_TOMBSTONED ) ||
#endif
            (task->tc_State == TS_REMOVED))
        {
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
            // The task is on its way out ...
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: --> Dispatching finalizing/tombstoned task?" DEBUGCOLOR_RESET "\n", cpuNo, __func__);
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: --> Task @ 0x%p '%s', state %08X" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task, task->tc_Node.ln_Name, newtask->tc_State);
#endif
        }

        if (sleeptask)
        {
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Moving '%s' @ 0x%p to wait queue" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newtask->tc_Node.ln_Name, newtask);)
#if defined(__AROSEXEC_SMP__)
            KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL,
                        SPINLOCK_MODE_WRITE);
#endif
            Enqueue(&SysBase->TaskWait, &newtask->tc_Node);
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);
#endif
        }

        if (!launchtask)
        {
            /* if the new task shouldn't run - force a reschedule */
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Skipping '%s' @ 0x%p (state %08X)" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newtask->tc_Node.ln_Name, newtask, newtask->tc_State);)

            core_Switch();
            newtask = core_Dispatch();
        }
        else
        {
#if defined(AROS_NO_ATOMIC_OPERATIONS)
            SysBase->DispCount++;
#else
            AROS_ATOMIC_INC(SysBase->DispCount);
#endif
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Launching '%s' @ 0x%p (state %08X)" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newtask->tc_Node.ln_Name, newtask, newtask->tc_State);)
        }
    }
    else
    {
        /* Go idle if there is nothing to do ... */
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: No ready Task(s) - entering sleep mode" DEBUGCOLOR_RESET "\n", cpuNo, __func__);)

        /*
         * Idle counter is incremented every time when we enter here,
         * not only once. This is correct.
         */
#if defined(AROS_NO_ATOMIC_OPERATIONS)
        SysBase->IdleCount++;
#else
        AROS_ATOMIC_INC(SysBase->IdleCount);
#endif
        FLAG_SCHEDSWITCH_SET;
    }

    return newtask;
}
