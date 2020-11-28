/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
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

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 0

#if (DEBUG > 0)
#define DSCHED(x) x
#else
#define DSCHED(x)
#endif

#if defined(__AROSEXEC_SMP__)
void core_InitScheduleData(struct X86SchedulerPrivate *schedData)
{
    DSCHED(bug("[Kernel] %s(0x%p)\n", __func__, schedData);)
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

    DSCHED(bug("[Kernel] %s()\n", __func__);)

    task = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuNo = KrnGetCPUNumber();
#endif

    DSCHED(
        bug("[Kernel:%03u] %s: running Task @ 0x%p\n", cpuNo, __func__, task);
        bug("[Kernel:%03u] %s: '%s', state %08x\n", cpuNo, __func__, task->tc_Node.ln_Name, task->tc_State);
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
            bug("[Kernel:%03u] core_Schedule: letting finalising task run..\n", cpuNo);
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
                bug("[Kernel:%03u] '%s' @ 0x%p needs rescheduled ..\n", cpuNo, task->tc_Node.ln_Name, task);
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
    BOOL doSwitch = TRUE;

    DSCHED(bug("[Kernel] %s()\n", __func__);)

    cpuNo = KrnGetCPUNumber();
    task = GET_THIS_TASK;

    DSCHED(
        bug("[Kernel:%03u] %s: Current running Task @ 0x%p\n", cpuNo, __func__, task);
    )
    if ((!task) || (task->tc_State == TS_INVALID))
    {
        bug("[Kernel:%03u] %s: called on invalid task!\n", cpuNo, __func__);
        doSwitch = FALSE;
    }
    DSCHED(
        bug("[Kernel:%03u] %s: Task name '%s'\n", cpuNo, __func__, task->tc_Node.ln_Name);
        bug("[Kernel:%03u] %s: Task state = %08x\n", cpuNo, __func__, task->tc_State);
    )
#if defined(__AROSEXEC_SMP__)
    if (task->tc_State == TS_TOMBSTONED)
        doSwitch = FALSE;
#endif
    if (!doSwitch)
    {
        bug("[Kernel:%03u] %s: Letting Task continue to run..\n", cpuNo, __func__);
        return;
    }

    DSCHED(
        bug("[Kernel:%03u] %s: Switching away from Task\n", cpuNo, __func__);
    )
    
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#else
    if (task->tc_State != TS_RUN)
#endif
    REMOVE(&task->tc_Node);
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);
    if (task->tc_State == TS_REMOVED)
        task->tc_State = TS_TOMBSTONED;
#endif

    DSCHED(bug("[Kernel:%03u] %s: Task removed from list, state = %08x\n", cpuNo, __func__, task->tc_State);)

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
        bug("[Kernel:%03u] EROR! Task went out of stack limits\n", cpuNo);
        bug("[Kernel:%03u]  - Lower Bound = 0x%p, Upper Bound = 0x%p\n", cpuNo, task->tc_SPLower, task->tc_SPUpper);
        bug("[Kernel:%03u]  - SP = 0x%p\n", cpuNo, task->tc_SPReg);

        task->tc_SigWait    = 0;
        task->tc_State      = TS_WAIT;

        showAlert = AN_StackProbe;
    }

    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    if (task->tc_State == TS_READY)
    {
        if (task->tc_Flags & TF_SWITCH)
            AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));

        DSCHED(bug("[Kernel:%03u] Setting '%s' @ 0x%p as ready\n", cpuNo, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                SPINLOCK_MODE_WRITE);
#endif
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    else if (task->tc_State == TS_SPIN)
    {
        DSCHED(bug("[Kernel:%03u] Setting '%s' @ 0x%p to spin\n", cpuNo, task->tc_Node.ln_Name, task);)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                    SPINLOCK_MODE_WRITE);
        Enqueue(&PrivExecBase(SysBase)->TaskSpinning, &task->tc_Node);
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
    }
#endif
    else if(
#if defined(__AROSEXEC_SMP__)
        (task->tc_State != TS_TOMBSTONED) &&
#endif
        (task->tc_State != TS_REMOVED))
    {
        DSCHED(bug("[Kernel:%03u] Setting '%s' @ 0x%p to wait\n", cpuNo, task->tc_Node.ln_Name, task);)
#if defined(__AROSEXEC_SMP__)
        KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL,
                    SPINLOCK_MODE_WRITE);
#endif
        Enqueue(&SysBase->TaskWait, &task->tc_Node);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);
#endif
    }
    if (showAlert)
        Alert(showAlert);
}

/* Dispatch a "new" ready task on this cpu */
struct Task *core_Dispatch(void)
{
    struct Task *newtask;
    struct Task *task = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__) || (DEBUG > 0)
    cpuid_t cpuNo = KrnGetCPUNumber();
#endif

    DSCHED(bug("[Kernel:%03u] core_Dispatch()\n", cpuNo);)

#if defined(__AROSEXEC_SMP__)
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
            break;
#if defined(__AROSEXEC_SMP__)
        }
#endif
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);
#endif

    if ((!newtask) && (task) && (task->tc_State != TS_WAIT))
    {
#if defined(__AROSEXEC_SMP__)
        if (task->tc_State == TS_SPIN)
        {
#if 0
bug("----> such unspinning should not take place!\n");
            KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                    SPINLOCK_MODE_WRITE);
            REMOVE(&task->tc_Node);
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);
#endif
            task->tc_State = TS_READY;
        }
#endif
        newtask = task;
    }

    if (newtask != NULL)
    {
        BOOL launchtask = FALSE, sleeptask = FALSE;

        if (newtask->tc_State == TS_READY)
        {
            DSCHED(bug("[Kernel:%03u] Preparing to run '%s' @ 0x%p\n",
                cpuNo, newtask->tc_Node.ln_Name, newtask);)

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
            bug("[Kernel:%03u] --> Dispatching finalizing/tombstoned task?\n", cpuNo);
            bug("[Kernel:%03u] --> Task @ 0x%p '%s', state %08x\n", cpuNo, task, task->tc_Node.ln_Name, newtask->tc_State);
#endif
        }

        if (sleeptask)
        {
            DSCHED(bug("[Kernel:%03u] Moving '%s' @ 0x%p to wait queue\n", cpuNo, newtask->tc_Node.ln_Name, newtask);)
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
            DSCHED(bug("[Kernel:%03u] Skipping '%s' @ 0x%p (state %08x)\n", cpuNo, newtask->tc_Node.ln_Name, newtask, newtask->tc_State);)

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
            DSCHED(bug("[Kernel:%03u] Launching '%s' @ 0x%p (state %08x)\n", cpuNo, newtask->tc_Node.ln_Name, newtask, newtask->tc_State);)
        }
    }
    else
    {
        /* Go idle if there is nothing to do ... */
        DSCHED(bug("[Kernel:%03u] No ready Task(s) - entering sleep mode\n", cpuNo);)

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
