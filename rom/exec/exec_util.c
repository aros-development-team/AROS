/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
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

	APTR AllocTaskMem (

/*  SYNOPSIS */
	struct Task * task,
	ULONG	      size,
	ULONG	      req)

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

	void FreeTaskMem (

/*  SYNOPSIS */
	struct Task * task,
	APTR	      mem)

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

	struct Task * FindTaskByID(

/*  SYNOPSIS */
	ULONG	    id)

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
	First up, check ThisTask
    */
    et = GetETask(SysBase->ThisTask);
    if( et != NULL && et->et_UniqueID == id )
	return SysBase->ThisTask;

    /*	Next, go through the ready list */
    ForeachNode(&SysBase->TaskReady, t)
    {
	et = GetETask(t);
	if( et != NULL && et->et_UniqueID == id )
	    return t;
    }

    /* Finally, go through the wait list */
    ForeachNode(&SysBase->TaskWait, t)
    {
	et = GetETask(t);
	if( et != NULL && et->et_UniqueID == id )
	    return t;
    }

    return NULL;
}
