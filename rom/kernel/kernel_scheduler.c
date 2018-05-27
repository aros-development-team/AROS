/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include "exec_platform.h"

#define D(x)

/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */
BOOL core_Schedule(void)
{
    struct Task *task = GET_THIS_TASK;

    D(bug("[KRN] core_Schedule()\n"));

    FLAG_SCHEDSWITCH_CLEAR;

    if (task->tc_State == TS_REMOVED)
        return FALSE;

    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    if (!(task->tc_Flags & TF_EXCEPT))
    {
        BYTE pri;

        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            return FALSE;

        /* Does the TaskReady list contain tasks with priority equal to or lower than current task?
         * If so, then check further... */
        pri = ((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri;
        if (pri <= task->tc_Node.ln_Pri)
        {
            /* If the running task did not used it's whole quantum yet, let it work */
            if (!FLAG_SCHEDQUANTUM_ISSET)
                return FALSE;
        }
    }

    D(bug("[KRN] Rescheduling required\n"));

    /* Select new task to run */
    return TRUE;
}

/* Actually switch away from the task */
void core_Switch(void)
{
    struct Task *task = GET_THIS_TASK;
    ULONG showAlert = 0;

    D(bug("[KRN] core_Switch(): Old task = %p (%s)\n", task, task->tc_Node.ln_Name));

    if (task->tc_State != TS_RUN)
        Remove(&task->tc_Node);

    if ((task->tc_State != TS_WAIT) && (task->tc_State != TS_REMOVED))
        task->tc_State = TS_READY;

#ifndef __mc68000
    if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
    {
        bug("[KRN] Task %s went out of stack limits\n", task->tc_Node.ln_Name);
        bug("[KRN] Lower %p, upper %p, SP %p\n", task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);
        /*
         * Suspend the task to stop it from causing more harm. In some rare cases, if the task is holding
         * lock on some global/library semaphore it will most likelly mean immenent freeze. In most cases
         * however, user will be shown an alert.
         */
        task->tc_SigWait    = 0;
        task->tc_State      = TS_WAIT;

        showAlert = AN_StackProbe;
    }
#endif

    task->tc_IDNestCnt = IDNESTCOUNT_GET;

    if (task->tc_State == TS_READY)
    {
        if (task->tc_Flags & TF_SWITCH)
            AROS_UFC1NR(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
    }
    else if (task->tc_State != TS_REMOVED)
    {
        D(bug("[KRN] Setting '%s' @ 0x%p to wait\n", task->tc_Node.ln_Name, task));
        Enqueue(&SysBase->TaskWait, &task->tc_Node);
    }
    if (showAlert)
        Alert(showAlert);
}

/*
 * Task dispatcher. Basically it may be the same one no matter
 * what scheduling algorithm is used (except SysBase->Elapsed reloading)
 */
struct Task *core_Dispatch(void)
{
    struct Task *task;

    D(bug("[KRN] core_Dispatch()\n"));

    task = (struct Task *)REMHEAD(&SysBase->TaskReady);
    if (!task)
    {
        /* Is the list of ready tasks empty? Well, go idle. */
        D(bug("[KRN] No ready tasks, entering sleep mode\n"));

        /*
         * Idle counter is incremented every time when we enter here,
         * not only once. This is correct.
         */
        SysBase->IdleCount++;
        FLAG_SCHEDSWITCH_SET;

        return NULL;
    }

    if (task->tc_State == TS_READY)
    {
        IDNESTCOUNT_SET(task->tc_IDNestCnt);
        SET_THIS_TASK(task);
        SCHEDELAPSED_SET(SCHEDQUANTUM_GET);
        FLAG_SCHEDQUANTUM_CLEAR;
        /*
         * Check the stack of the task we are about to launch.
         * Unfortunately original m68k programs can change stack manually without updating SPLower or SPUpper.
         * For example WB3.1 C:SetPatch adds exec/OpenDevice() patch that swaps stacks manually.
         * Result is that _all_ programs that call OpenDevice() crash if stack is checked.
         */
#ifndef __mc68000
        if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
            task->tc_State     = TS_WAIT;
        else
#endif
            task->tc_State     = TS_RUN;
    }

    if (task->tc_State != TS_RUN)
    {
        D(bug("[KRN] Skipping '%s' @ 0x%p (state %08x)\n", task->tc_Node.ln_Name, task, task->tc_State));

        core_Switch();
        task = core_Dispatch();
    }
    else
    {
        D(bug("[KRN] New task = %p (%s)\n", task, task->tc_Node.ln_Name));

        SysBase->DispCount++;
        if (task->tc_Flags & TF_LAUNCH)
            AROS_UFC1NR(void, task->tc_Launch, AROS_UFCA(struct ExecBase *, SysBase, A6));
    }

    /* Leave interrupt and jump to the new task */
    return task;
}
