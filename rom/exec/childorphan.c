/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Make any children of this task orphans.
    Lang: english
*/
#include "exec_intern.h"
#include "exec_util.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, ChildOrphan,

/*  SYNOPSIS */
	AROS_LHA(ULONG, tid, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 124, Exec)

/*  FUNCTION
	ChildOrphan() will detach the specified task from the its parent
	task child task tree. This is useful if the parent task will be
	exiting, and no longer needs to be told about child task events.

	Note that the default Task finaliser will orphan any remaining
	children when the task exits. This function can be used if you just
	do not wish to be told about a particular task.

    INPUTS
	tid	--  The ID of the task to orphan, or 0 for all tasks. Note
		    that this is NOT the pointer to the task.

    RESULT
	Will return 0 on success or CHILD_* on an error.

	The child/children will no longer participate in child task
	actions.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *this = FindTask(NULL);
    struct ETask *et, *child;

    et = GetETask(this);
    if(et == NULL)
	return CHILD_NOTNEW;

    if(tid == 0L)
    {
    	Forbid();
	ForeachNode(&et->et_Children, child)
	{
	    /*
		Don't need to Remove(), because I'll blow away the entire
		list at the end of the loop.
	     */
	    child->et_Parent = NULL;
	}
	NEWLIST(&et->et_Children);
    	Permit();
    }
    else
    {
    	Forbid();
	child = FindChild(tid);
	if(child != NULL)
	{
	    child->et_Parent = NULL;
	    Remove((struct Node *)child);
	}
	else
	{
    	    Permit();
	    return CHILD_NOTFOUND;
	}
	Permit();
    }
    return 0;

    AROS_LIBFUNC_EXIT
} /* ChildOrphan */
