#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

/* FIXME: Can we remove usage of exec_intern.h ? */
#include "exec_intern.h"
/* FIXME: Can we remove usage of taskstorage.h ? */
#include "taskstorage.h"

//#define D(x)
#define DTSS(x)

/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */
BOOL core_Schedule(void)
{
    struct Task *task = SysBase->ThisTask;

    D(bug("[KRN] core_Schedule()\n"));

    SysBase->AttnResched &= ~ARF_AttnSwitch;

    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    if (!(task->tc_Flags & TF_EXCEPT))
    {
        BYTE pri;

        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            return FALSE;

	/* Does the TaskReady list contains tasks with priority equal or lower than current task?
         * If so, then check further... */
	pri = ((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri;
        if (pri <= task->tc_Node.ln_Pri)
        {
            /* If the running task did not used it's whole quantum yet, let it work */
            if (!(SysBase->SysFlags & SFF_QuantumOver))
                return FALSE;
        }
    }

    /* 
     * If we got here, then the rescheduling is necessary. 
     * Put the task into the TaskReady list.
     */
    D(bug("[KRN] Setting task 0x%p (%s) to READY\n", task, task->tc_Node.ln_Name));
    task->tc_State = TS_READY;
    Enqueue(&SysBase->TaskReady, &task->tc_Node);

    /* Select new task to run */
    return TRUE;
}

/* Actually switch away from the task */
void core_Switch(void)
{
    struct Task *task = SysBase->ThisTask;

    D(bug("[KRN] core_Switch(): Old task = %p (%s)\n", task, task->tc_Node.ln_Name));

    task->tc_IDNestCnt = SysBase->IDNestCnt;

    if (task->tc_Flags & TF_SWITCH)
	AROS_UFC1(void, task->tc_Switch, AROS_UFCA(struct ExecBase *, SysBase, A6));
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

	/* Idle counter is incremented every time when we enter here,
	   not only once. This is correct. */
	SysBase->IdleCount++;
        SysBase->AttnResched |= ARF_AttnSwitch;

	return NULL;
    }

    SysBase->DispCount++;

    SysBase->IDNestCnt = task->tc_IDNestCnt;
    SysBase->ThisTask = task;
    SysBase->Elapsed = SysBase->Quantum;
    SysBase->SysFlags &= ~SFF_QuantumOver;
    task->tc_State = TS_RUN;

    D(bug("[KRN] New task = %p (%s)\n", task, task->tc_Node.ln_Name));

    /*
     * Increase TaskStorage if it is not big enough
     * URGENT FIXME: Move this out of interrupts. It's not a good place to do allocations!
     */
    IPTR *oldstorage = task->tc_UnionETask.tc_TaskStorage;
    if ((int)oldstorage[__TS_FIRSTSLOT] < PrivExecBase(SysBase)->TaskStorageSize)
    {
        IPTR *newstorage;
        ULONG oldsize = (ULONG)oldstorage[__TS_FIRSTSLOT];

	DTSS(bug("[KRN] Increasing storage (%d to %d) for task 0x%p (%s)\n", oldsize, PrivExecBase(SysBase)->TaskStorageSize, task, task->tc_Node.ln_Name));

        newstorage = AllocMem(PrivExecBase(SysBase)->TaskStorageSize, MEMF_PUBLIC|MEMF_CLEAR);
        /* FIXME: Add fault handling */

        CopyMem(oldstorage, newstorage, oldsize);
        newstorage[__TS_FIRSTSLOT] = PrivExecBase(SysBase)->TaskStorageSize;
        task->tc_UnionETask.tc_TaskStorage = newstorage;
        FreeMem(oldstorage, oldsize);
    }

    /*
     * Check the stack of the task we are about to launch.
     * Unfortunately original m68k programs can change stack manually without updating SPLower or SPUpper.
     * For example WB3.1 C:SetPatch adds exec/OpenDevice() patch that swaps stacks manually.
     * Result is that _all_ programs that call OpenDevice() crash if stack is checked.
     */
#ifndef __mc68000
    if (task->tc_SPReg <= task->tc_SPLower || task->tc_SPReg > task->tc_SPUpper)
    {
	bug("[KRN] Task %s went out of stack limits\n", task->tc_Node.ln_Name);
	bug("[KRN] Lower %p, upper %p, SP %p\n", task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);
	Alert(AT_DeadEnd|AN_StackProbe);
    }
#endif
    if (task->tc_Flags & TF_LAUNCH)
	AROS_UFC1(void, task->tc_Launch, AROS_UFCA(struct ExecBase *, SysBase, A6));

    /* Leave interrupt and jump to the new task */
    return task;
}
