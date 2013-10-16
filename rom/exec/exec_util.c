/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/

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

/*i***************************************************************************

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
        This is an internal exec.library function not exported from the
        library.

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

BOOL
Exec_InitETask(struct Task *task, struct ExecBase *SysBase)
{
    struct Task *thistask = FindTask(NULL);
    /*
     *  We don't add this to the task memory, it isn't free'd by
     *  RemTask(), rather by somebody else calling ChildFree().
     *  Alternatively, an orphaned task will free its own ETask.
     */
    struct ETask *et =
        AllocMem(sizeof(struct IntETask), MEMF_PUBLIC | MEMF_CLEAR);
    D(bug("[TSS] Create new ETask=%p for task=%p with size %d\n",
        et, task, sizeof(struct IntETask)));

    task->tc_UnionETask.tc_ETask = et;
    if (!et)
        return FALSE;
    task->tc_Flags |= TF_ETASK;

    if (thistask != NULL)
    {
        /* Clone parent's TaskStorage */
        struct ETask *thiset = GetETask(thistask);
        if (thiset) {
            IPTR *thists = thiset->et_TaskStorage;

            if (thists) {
                IPTR *ts = AllocMem(thists[__TS_FIRSTSLOT] * sizeof(thists[0]), MEMF_ANY);
                if (!ts) {
                    FreeMem(et, sizeof(struct IntETask));
                    return FALSE;
                }
                CopyMem(thists, ts, thists[__TS_FIRSTSLOT] * sizeof(thists[0]));
                et->et_TaskStorage = ts;
            }
        }
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

    /* Get a unique identifier for this task */
    Forbid();
    while(et->et_UniqueID == 0)
    {
	/*
	 *	Add some fuzz on wrapping. It's likely that the early numbers
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

    return TRUE;
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
            if (parent)
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
    IPTR *ts = et->et_TaskStorage;

    FreeVec(et->et_Result2);

#ifdef DEBUG_ETASK
    FreeVec(IntETask(et)->iet_Me);
#endif
    D(bug("Exec_ExpungeETask: Freeing et=%x, ts=%x, size=%d\n",
          et, ts, ts ? (ULONG)ts[__TS_FIRSTSLOT] : 0
    ));
    FreeMem(et, sizeof(struct IntETask));
    if (ts)
        FreeMem(ts, ts[__TS_FIRSTSLOT] * sizeof(ts[0]));
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
