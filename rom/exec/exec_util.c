/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>

#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include "exec_util.h"

	struct ETask * Exec_FindChild(

/*  SYNOPSIS */
	ULONG	    id,
	struct ExecBase *SysBase)

/*  FUNCTION
	Scan through the current tasks children list searching for the task
	whose et_UniqueID field matches.

    INPUTS
	id	-   The task ID to match.

    RESULT
	Address of the ETask structure that matches, or
	NULL otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct ETask *et;
    struct ETask *thisET;

    thisET = GetETask(FindTask(NULL));
    if (thisET != NULL)
    {
	ForeachNode (&thisET->et_Children, et)
	{
	    if (et->et_UniqueID == id)
		return et;
	}
	ForeachNode(&thisET->et_TaskMsgPort.mp_MsgList, et)
	{
	    if (et->et_UniqueID == id)
		return et;
	}
    }
    return NULL;
}

void
Exec_InitETask(struct Task *task, struct ExecBase *SysBase)
{
    struct Task *thistask = FindTask(NULL);
    /*
     *  We don't add this to the task memory, it isn't free'd by
     *  RemTask(), rather by somebody else calling ChildFree().
     *  Alternatively, an orphaned task will free its own ETask.
     */
    IPTR *ts = AllocMem(PrivExecBase(SysBase)->TaskStorageSize, MEMF_PUBLIC|MEMF_CLEAR);
    D(bug("[TSS] Create new TS=%x for task=%x with size %d\n",
          ts, task, PrivExecBase(SysBase)->TaskStorageSize
    ));

    /* IntETask is embedded in TaskStorage */
    struct ETask *et = (struct ETask *)ts;
    task->tc_UnionETask.tc_TaskStorage = ts;
    if (!ts)
        return;
    task->tc_Flags |= TF_ETASK;

    ts[__TS_FIRSTSLOT] = (IPTR)PrivExecBase(SysBase)->TaskStorageSize;
    if (thistask != NULL)
    {
        /* Clone TaskStorage */
        CopyMem(&thistask->tc_UnionETask.tc_TaskStorage[__TS_FIRSTSLOT+1],
                &task->tc_UnionETask.tc_TaskStorage[__TS_FIRSTSLOT+1],
                ((ULONG)thistask->tc_UnionETask.tc_TaskStorage[__TS_FIRSTSLOT])-(__TS_FIRSTSLOT+1)*sizeof(IPTR)
        );
    }
    et->et_Parent = thistask;
    NEWLIST(&et->et_Children);

    /* Initialise the message list */
    InitMsgPort(&et->et_TaskMsgPort);
    et->et_TaskMsgPort.mp_SigTask = task;
    et->et_TaskMsgPort.mp_SigBit = SIGB_CHILD;

    /* Initialise the trap fields */
    et->et_TrapAlloc = SysBase->TaskTrapAlloc;
    et->et_TrapAble = 0;

#ifdef DEBUG_ETASK
    {
	int len = strlen(task->tc_Node.ln_Name) + 1;
	IntETask(et)->iet_Me = AllocVec(len, MEMF_CLEAR|MEMF_PUBLIC);
	if (IntETask(et)->iet_Me != NULL)
	    CopyMem(task->tc_Node.ln_Name, IntETask(et)->iet_Me, len);
    }
#endif

    /* Get an unique identifier for this task */
    Forbid();
    while(et->et_UniqueID == 0)
    {
	/*
	 *	Add some fuzz on wrapping. Its likely that the early numbers
	 *	where taken by somebody else.
	 */
	if(++SysBase->ex_TaskID == 0)
	    SysBase->ex_TaskID = 1024;

	Disable();
	if (FindTaskByPID(SysBase->ex_TaskID) == NULL)
	    et->et_UniqueID = SysBase->ex_TaskID;
	Enable();
    }
    Permit();
    
    /* Finally if the parent task is an ETask, add myself as its child */
    if(et->et_Parent && ((struct Task*) et->et_Parent)->tc_Flags & TF_ETASK)
    {
	Forbid();
	ADDHEAD(&GetETask(et->et_Parent)->et_Children, et);
	Permit();
    }
}

void
Exec_CleanupETask(struct Task *task, struct ExecBase *SysBase)
{
    struct ETask *et = NULL, *child, *nextchild, *parent;
    struct Node *tmpNode;
    BOOL expunge = TRUE;

    if(task->tc_Flags & TF_ETASK)
        et = task->tc_UnionETask.tc_ETask;
    if(!et)
	return;

    D(bug("CleanupETask: task=%x, et=%x\n", task, et));

    Forbid();
    
    /* Clean up after all the children that the task didn't do itself. */
    ForeachNodeSafe(&et->et_TaskMsgPort.mp_MsgList, child, tmpNode)
    {
        ExpungeETask(child);
    }

    /* If we have an ETask parent, tell it we have exited. */
    if(et->et_Parent != NULL)
    {
        parent = GetETask(et->et_Parent);

        /* Link children to our parent. */
        ForeachNodeSafe(&et->et_Children, child, nextchild)
        {
            child->et_Parent = et->et_Parent;
            //Forbid();
            ADDTAIL(&parent->et_Children, child);
            //Permit();
        }

        /* Notify parent only if child was created with NP_NotifyOnDeath set 
           to TRUE */
        if(parent != NULL)
        {
            REMOVE(et);

            if(
               (((struct Task *)task)->tc_Node.ln_Type == NT_PROCESS) && 
               (((struct Process*) task)->pr_Flags & PRF_NOTIFYONDEATH)
            )
            {
                PutMsg(&parent->et_TaskMsgPort, (struct Message *)et);
                expunge = FALSE;
            }
        }
    }
    else
    {
        /* Orphan all our remaining children. */
        ForeachNode(&et->et_Children, child)
            child->et_Parent = NULL;
    }

    if(expunge)
        ExpungeETask(et);

    Permit();
}

void
Exec_ExpungeETask(struct ETask *et, struct ExecBase *SysBase)
{
    IPTR *ts = (IPTR *)et;

    if(et->et_Result2)
        FreeVec(et->et_Result2);

#ifdef DEBUG_ETASK
    FreeVec(IntETask(et)->iet_Me);
#endif
    D(bug("Exec_ExpungeETask: Freeing ts=%x, size=%d\n",
          ts, (ULONG)ts[__TS_FIRSTSLOT]
    ));
    FreeMem(ts, (ULONG)ts[__TS_FIRSTSLOT]);
}

static inline void FixList(struct List *from, struct List *to)
{
    /*
     * This sequence is incomplete. It is valid only after the whole structure has been copied.
     * !!! DO NOT remove the second line !!!
     * If the list is empty, the first line actually modifies from->lh_TailPred, so that
     * to->lh_TailPred gets correct value !
     */
    to->lh_Head->ln_Pred     = (struct Node *)&to->lh_Head;
    to->lh_TailPred          = from->lh_TailPred;
    to->lh_TailPred->ln_Succ = (struct Node *)&to->lh_Tail;
}

BOOL Exec_ExpandTS(struct Task *task, struct ExecBase *SysBase)
{
    struct Task *me = FindTask(NULL);
    ULONG oldsize = task->tc_UnionETask.tc_TaskStorage[__TS_FIRSTSLOT];
    struct ETask *et_old = task->tc_UnionETask.tc_ETask;
    struct ETask *et_new;

    D(bug("[TSS] Increasing storage (%d to %d) for task 0x%p (%s)\n", oldsize, PrivExecBase(SysBase)->TaskStorageSize, task, task->tc_Node.ln_Name));

    /* Allocate new storage */
    et_new = AllocMem(PrivExecBase(SysBase)->TaskStorageSize, MEMF_PUBLIC|MEMF_CLEAR);
    if (!et_new)
        return FALSE;

    if (task == me)
    {
        /*
         * If we are updating ourselves, we need to disable task switching.
         * Otherwise our ETask can become inconsistent (data in old ETask will
         * be modified in the middle of copying).
         */
        Forbid();
    }

    /* This copies most of data. */
    CopyMem(et_old, et_new, oldsize);

    /* ETask includes lists, and we need to fix up pointers in them now */
    FixList((struct List *)&et_old->et_Children, (struct List *)&et_new->et_Children);
    FixList(&et_old->et_TaskMsgPort.mp_MsgList, &et_new->et_TaskMsgPort.mp_MsgList);

    /* Set new TSS size, and install new ETask */
    ((IPTR *)et_new)[__TS_FIRSTSLOT] = PrivExecBase(SysBase)->TaskStorageSize;
    task->tc_UnionETask.tc_ETask = et_new;

    if (task == me)
    {
        /* All done, enable multitask again */
        Permit();
    }

    FreeMem(et_old, oldsize);
    return TRUE;
}

BOOL Exec_CheckTask(struct Task *task, struct ExecBase *SysBase)
{
    struct Task *t;

    if (!task)
	return FALSE;

    Forbid();

    if (task == SysBase->ThisTask)
    {
    	Permit();
    	return TRUE;
    }

    ForeachNode(&SysBase->TaskReady, t)
    {
    	if (task == t)
    	{
    	    Permit();
    	    return TRUE;
    	}
    }

    ForeachNode(&SysBase->TaskWait, t)
    {
    	if (task == t)
    	{
    	    Permit();
    	    return TRUE;
    	}
    }
    
    Permit();
    return FALSE;
}
