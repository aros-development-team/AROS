/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>

#include <proto/exec.h>

#include "exec_util.h"

extern struct ExecBase* SysBase;

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

    ml = AllocMem (sizeof (struct MemList), MEMF_ANY | MEMF_CLEAR);
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


