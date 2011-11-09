/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

/*
 * TODO: The following two functions are subject to removal.
 * They are used only by i386-pc port for allocating CPU context.
 * This needs to be changed.
 */

/*****************************************************************************

    NAME */
#include "exec_util.h"

	APTR Exec_AllocTaskMem (

/*  SYNOPSIS */
	struct Task * task,
	ULONG	      size,
	ULONG	      req,
	struct ExecBase *SysBase)

/*  FUNCTION
	Allocate memory which will be freed when the task is removed.

    INPUTS
	task	  - The memory will be freed when this task is removed.
	size	  - How much memory.
	req	  - What memory. See AllocMem() for details.

    RESULT
	Adress to a memory block or NULL if no memory was available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocMem(), FreeTaskMem()

    INTERNALS
	The memory is allocated and queued in the tc_MemList. This
	list is freed in RemTask().

******************************************************************************/
{
    struct MemList * ml;
    APTR mem;

    ml = AllocMem (sizeof (struct MemList), MEMF_ANY);
    mem = AllocMem (size, req);

    if (!ml || !mem)
    {
	if (ml)
	    FreeMem (ml, sizeof (struct MemList));

	if (mem)
	    FreeMem (mem, size);

	return NULL;
    }

    ml->ml_NumEntries	   = 1;
    ml->ml_ME[0].me_Addr   = mem;
    ml->ml_ME[0].me_Length = size;

    Forbid ();
    AddHead (&task->tc_MemEntry, &ml->ml_Node);
    Permit ();

    return mem;
} /* AllocTaskMem */


/*****************************************************************************

    NAME */
#include "exec_util.h"

	void Exec_FreeTaskMem (

/*  SYNOPSIS */
	struct Task * task,
	APTR	      mem,
	struct ExecBase *SysBase)

/*  FUNCTION
	Freeate memory which will be freed when the task is removed.

    INPUTS
	task	  - The memory will be freed when this task is removed.
	size	  - How much memory.
	req	  - What memory. See FreeMem() for details.

    RESULT
	Adress to a memory block or NULL if no memory was available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeMem(), FreeTaskMem()

    INTERNALS
	The memory is allocated and queued in the tc_MemList. This
	list is freed in RemTask().

******************************************************************************/
{
    struct MemList * ml, * next;

    Forbid ();

    ForeachNodeSafe (&task->tc_MemEntry, ml, next)
    {
	/*
	    Quick check: If the node was allocated by AllocTaskMem(),
	    then it has only one entry.
	*/
	if (ml->ml_NumEntries == 1
	    && ml->ml_ME[0].me_Addr == mem
	)
	{
	    Remove (&ml->ml_Node);
	    Permit ();

	    FreeMem (ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
	    FreeMem (ml, sizeof (struct MemList));

	    return;
	}
    }

    Permit ();

} /* FreeTaskMem */

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
    struct ETask *et = AllocVec(sizeof(struct IntETask), MEMF_PUBLIC|MEMF_CLEAR);
    IPTR *ts = AllocMem(PrivExecBase(SysBase)->TaskStorageSize, MEMF_PUBLIC|MEMF_CLEAR);
    if (!et || !ts)
    {
        if (et)
            FreeVec(et);
        return;
    }
    et->et_TaskStorage = ts;
    task->tc_UnionETask.tc_ETask = et;
    task->tc_Flags |= TF_ETASK;

    ts[0] = (IPTR)PrivExecBase(SysBase)->TaskStorageSize;
    if (thistask != NULL)
    {
        /* Clone TaskStorage */
        CopyMem(&thistask->tc_UnionETask.tc_ETask->et_TaskStorage[1],
                &task->tc_UnionETask.tc_ETask->et_TaskStorage[1],
                ((ULONG)thistask->tc_UnionETask.tc_ETask->et_TaskStorage[0])-sizeof(IPTR)
        );
    }

    et->et_Parent = FindTask(NULL);
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
    IPTR *ts = et->et_TaskStorage;

    if(et->et_Result2)
        FreeVec(et->et_Result2);

#ifdef DEBUG_ETASK
    FreeVec(IntETask(et)->iet_Me);
#endif
    FreeMem(ts, (ULONG)ts[0]);
    FreeVec(et);
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
