/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <proto/exec.h>

#include "etask.h"
#include "exec_util.h"

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

    HISTORY

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

    HISTORY

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

	struct Task * Exec_FindTaskByID(

/*  SYNOPSIS */
	ULONG	    id,
	struct ExecBase *SysBase)

/*  FUNCTION
	Scan through the task lists searching for the task whose
	et_UniqueID field matches.

    INPUTS
	id	-   The task ID to match.

    RESULT
	Address of the Task control structure that matches, or
	NULL otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct Task *t;
    struct ETask *et;

    /*
	First up, check ThisTask. It could be NULL because of exec_init.c
    */
    if (SysBase->ThisTask != NULL)
    {
	et = GetETask(SysBase->ThisTask);
	if (et != NULL && et->et_UniqueID == id)
	    return SysBase->ThisTask;
    }

    /*	Next, go through the ready list */
    ForeachNode(&SysBase->TaskReady, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
	    return t;
    }

    /* Finally, go through the wait list */
    ForeachNode(&SysBase->TaskWait, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
	    return t;
    }

    return NULL;
}

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

    HISTORY

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
    }
    return NULL;
}

void
Exec_InitETask(struct Task *task, struct ETask *et, struct ExecBase *SysBase)
{
    et->et_Parent = FindTask(NULL);
    NEWLIST(&et->et_Children);

    /* Initialise the message list */
    NEWLIST(&et->et_TaskMsgPort.mp_MsgList);
    et->et_TaskMsgPort.mp_Flags = PA_SIGNAL;
    et->et_TaskMsgPort.mp_Node.ln_Type = NT_MSGPORT;
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

#if 1
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
	if(Exec_FindTaskByID(SysBase->ex_TaskID, SysBase) == NULL)
	    et->et_UniqueID = SysBase->ex_TaskID;
	Enable();
    }
    Permit();
#endif
}
