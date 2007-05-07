/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Find out the status of a child task.
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, ChildStatus,

/*  SYNOPSIS */
	AROS_LHA(APTR, tid, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 125, Exec)

/*  FUNCTION
	Return the status of a child task. This allows the Task to
	determine whether a particular child task is still running or not.

    INPUTS
	tid	--  The ID of the task to examine. Note that this is _NOT_
		    a task pointer.

    RESULT
	One of the CHILD_* values.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task	    *this;
    struct ETask    *et;
    struct ETask    *child;
    ULONG	     status = CHILD_NOTFOUND;

    this = FindTask(NULL);
    if ((this->tc_Flags & TF_ETASK) == 0)
	return CHILD_NOTNEW;

    et = this->tc_UnionETask.tc_ETask;

    /* Sigh... */
    Forbid();

    /* Search through the running tasks list */
    ForeachNode(&et->et_Children, child)
    {
	if (child->et_UniqueID == (ULONG)tid)
	{
	    status = CHILD_ACTIVE;
	    break;
	}
    }

    ForeachNode(&et->et_TaskMsgPort.mp_MsgList, child)
    {
	if (child->et_UniqueID == (ULONG)tid)
	{
	    status = CHILD_EXITED;
	    break;
	}
    }

    Permit();
    return status;

    AROS_LIBFUNC_EXIT
} /* ChildStatus */
