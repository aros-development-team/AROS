/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/execbase.h>
#include <exec/lists.h>
#include <proto/exec.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"

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

/* Overridden by exec's, which reports the overrun; without exec the task is only parked */
void __attribute__((weak)) Exec_StackProbe(struct Task *task, struct ExecBase *SysBase)
{
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
    BOOL doSwitch = TRUE;
#if defined(__AROSEXEC_SMP__)
    BOOL parkedByWait = FALSE;
    unsigned long intFlags = 0;
#endif
    struct ETask * taskEtask = NULL;

    DSCHED(bug("[Kernel]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", __func__);)

    cpuNo = KrnGetCPUNumber();
    task = GET_THIS_TASK;

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

#if defined(__AROSEXEC_SMP__)
    /* Wait() parked a TS_WAIT task on TaskWait and still holds its lock */
    if (task->tc_State == TS_WAIT)
        parkedByWait = TRUE;
    else
    {
        /* Task lock before list locks; interrupts off while it is held */
        __save_flags(intFlags);
        __cli();
        KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
        /* RemTask() on another CPU may have taken it off TaskRunning */
        if (task->tc_State != TS_TOMBSTONED)
        {
            exec_TaskRemoveRunning(task);
            if (task->tc_State == TS_REMOVED)
                task->tc_State = TS_TOMBSTONED;
        }
    }
#else
    if (task->tc_State != TS_RUN && task->tc_State != TS_REMOVED)
        REMOVE(&task->tc_Node);
#endif

    DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Task removed from list, state = %08X" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_State);)

    if ((task->tc_State != TS_WAIT) &&
#if defined(__AROSEXEC_SMP__)
        (task->tc_State != TS_SPIN) &&
        (task->tc_State != TS_TOMBSTONED) &&
#endif
        (task->tc_State != TS_REMOVED))
        task->tc_State = TS_READY;

    /* if the current task has gone out of stack bounds, suspend it to prevent further damage to the system */
    if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " ERROR! Task (%s) went out of stack limits" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_Node.ln_Name);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET "  - Lower Bound = 0x%p, Upper Bound = 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_SPLower, task->tc_SPUpper);
        bug("[Kernel:%03u]" DEBUGCOLOR_SET "  - SP = 0x%p" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_SPReg);

        /* a task on its way out is left as it is */
        if ((task->tc_State != TS_REMOVED)
#if defined(__AROSEXEC_SMP__)
            && (task->tc_State != TS_TOMBSTONED)
#endif
           )
        {
            task->tc_SigWait    = 0;
            task->tc_State      = TS_WAIT;
            Exec_StackProbe(task, SysBase);
        }
#if defined(LOWSTACKWARN)
    }
    else if (task->tc_SPReg <= task->tc_SPLower + 1024)
    {
        bug("[Kernel:%03u]" DEBUGCOLOR_SET " WARNING - Task (%s) close to stack limits" DEBUGCOLOR_RESET "\n", cpuNo, task->tc_Node.ln_Name);
#endif
    }
    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    if (task->tc_State == TS_READY)
    {
        if (task->tc_Flags & TF_SWITCH)
            AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));

        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p as ready" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        exec_TaskEnqueueReady(task);
#else
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    else if (task->tc_State == TS_SPIN)
    {
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p to spin" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
        exec_TaskEnqueueSpinning(task);
    }
#endif
    else if(
#if defined(__AROSEXEC_SMP__)
        (!parkedByWait) &&
        (task->tc_State != TS_TOMBSTONED) &&
#endif
        (task->tc_State != TS_REMOVED))
    {
        DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Setting '%s' @ 0x%p to wait" DEBUGCOLOR_RESET "\n", cpuNo, __func__, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        exec_TaskEnqueueWait(task);
#else
        Enqueue(&SysBase->TaskWait, &task->tc_Node);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    if (!parkedByWait)
    {
        KrnSpinUnLock(&task->tc_SpinLock);
        __restore_flags(intFlags);
    }
#endif
}

/* Dispatch a "new" ready task on this cpu */
struct Task *core_Dispatch(void)
{
    struct Task *newtask;
    struct Task *task = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    BOOL newtaskLocked = FALSE, sawContended;
    unsigned long intFlags;
#endif
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuid_t cpuNo = KrnGetCPUNumber();
#endif

    DSCHED(bug("[Kernel:%03u]" DEBUGFUNCCOLOR_SET " %s()" DEBUGCOLOR_RESET "\n", cpuNo, __func__);)

#if defined(__AROSEXEC_SMP__)
    __save_flags(intFlags);
    __cli();
rescan:
    sawContended = FALSE;
    KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#endif
    for (newtask = (struct Task *)GetHead(&SysBase->TaskReady); newtask != NULL; newtask = (struct Task *)GetSucc(newtask))
    {
#if defined(__AROSEXEC_SMP__)
        if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) || (GetIntETask(newtask) && core_APIC_CPUInMask(cpuNo, GetIntETask(newtask)->iet_CpuAffinity)))
        {
            /* The list lock is already held, so the task lock can only be
               tried; a busy one is being moved by another CPU. */
            if (!KrnSpinTryLock(&newtask->tc_SpinLock, SPINLOCK_MODE_WRITE))
            {
                sawContended = TRUE;
                continue;
            }
            newtaskLocked = TRUE;
#endif
            REMOVE(&newtask->tc_Node);
            break;
#if defined(__AROSEXEC_SMP__)
        }
#endif
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
    if (!newtask && sawContended)
        goto rescan;
    if (!newtaskLocked)
        __restore_flags(intFlags);
#endif

    if ((!newtask) && (task) && (task->tc_State != TS_WAIT))
    {
#if defined(__AROSEXEC_SMP__)
        /* The current task again, under its lock, only if parked on TaskSpinning */
        __save_flags(intFlags);
        __cli();
        KrnSpinLock(&task->tc_SpinLock, NULL, SPINLOCK_MODE_WRITE);
        if (task->tc_State == TS_SPIN)
        {
            struct Task *parked;

            KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                    SPINLOCK_MODE_WRITE);
            ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, parked)
            {
                if (parked == task)
                {
                    REMOVE(&task->tc_Node);
                    break;
                }
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
            if (parked == task)
            {
                DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Waking spinning task '%s' @ 0x%p" DEBUGCOLOR_RESET "\n",
                    cpuNo, __func__, task->tc_Node.ln_Name, task);)
                task->tc_State = TS_READY;
                newtaskLocked = TRUE;
                newtask = task;
            }
        }
        if (!newtaskLocked)
        {
            KrnSpinUnLock(&task->tc_SpinLock);
            __restore_flags(intFlags);
        }
#else
        newtask = task;
#endif
    }

    if (newtask != NULL)
    {
        BOOL launchtask = FALSE, sleeptask = FALSE;

        if (newtask->tc_State == TS_READY)
        {
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Preparing to run '%s' @ 0x%p" DEBUGCOLOR_RESET "\n",
                cpuNo, __func__, newtask->tc_Node.ln_Name, newtask);)

            IDNESTCOUNT_SET(newtask->tc_IDNestCnt);
            SET_THIS_TASK(newtask);
            SCHEDELAPSED_SET(SCHEDQUANTUM_GET);
            FLAG_SCHEDQUANTUM_CLEAR;
        }
        if  ((newtask->tc_State == TS_READY) || (newtask->tc_State == TS_RUN))
        {
            /* Check the stack of the task we are about to launch. */
            if ((newtask->tc_SPReg <= newtask->tc_SPLower) ||
                (newtask->tc_SPReg > newtask->tc_SPUpper))
            {
#if defined(__AROSEXEC_SMP__)
                /* It is on TaskRunning (SET_THIS_TASK above, or still running): off it before TaskWait */
                exec_TaskRemoveRunning(newtask);
#endif
                newtask->tc_State     = TS_WAIT;
                sleeptask = TRUE;
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
            bug("[Kernel:%03u]" DEBUGCOLOR_SET " ERROR! Task (%s) went out of stack limits" DEBUGCOLOR_RESET "\n", KrnGetCPUNumber(), newtask->tc_Node.ln_Name);
            /* With its wait mask cleared, ordinary signals no longer wake it */
            newtask->tc_SigWait = 0;
            newtask->tc_IDNestCnt = IDNESTCOUNT_GET;
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Moving '%s' @ 0x%p to wait queue" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newtask->tc_Node.ln_Name, newtask);)
#if defined(__AROSEXEC_SMP__)
            exec_TaskEnqueueWait(newtask);
#else
            Enqueue(&SysBase->TaskWait, &newtask->tc_Node);
#endif
            Exec_StackProbe(newtask, SysBase);
        }
#if defined(__AROSEXEC_SMP__)
        if (newtaskLocked)
        {
            KrnSpinUnLock(&newtask->tc_SpinLock);
            __restore_flags(intFlags);
        }
#endif

        if (!launchtask)
        {
            /* if the new task shouldn't run - force a reschedule */
            DSCHED(bug("[Kernel:%03u]" DEBUGCOLOR_SET " %s: Skipping '%s' @ 0x%p (state %08X)" DEBUGCOLOR_RESET "\n", cpuNo, __func__, newtask->tc_Node.ln_Name, newtask, newtask->tc_State);)

            /* A parked task was handled above; core_Switch() is for a task leaving the CPU */
            if (!sleeptask)
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
