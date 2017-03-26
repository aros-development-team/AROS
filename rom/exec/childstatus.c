/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Find out the status of a child task.
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, ChildStatus,

/*  SYNOPSIS */
	AROS_LHA(ULONG, tid, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 125, Exec)

/*  FUNCTION
	Return the status of a child task. This allows the Task to
	determine whether a particular child task is still running or not.

    INPUTS
	tid	--  The ID of the task to examine. Note that it is _NOT_
		    a task pointer.

    RESULT
	One of the CHILD_* values.

    NOTES
	This function will work correctly only for child tasks that are
	processes created with NP_NotifyOnDeath set to TRUE. Otherwise
	it may report CHILD_NOTFOUND even if child already exited.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task	    *ThisTask = GET_THIS_TASK;
    struct ETask    *et;
    struct ETask    *child;
    ULONG	     status = CHILD_NOTFOUND;

    if (ThisTask)
    {
        if ((ThisTask->tc_Flags & TF_ETASK) == 0)
        return CHILD_NOTNEW;

        et = ThisTask->tc_UnionETask.tc_ETask;

        /* Sigh... */
        Forbid();

        /* Search through the running tasks list */
        ForeachNode(&et->et_Children, child)
        {
        if (child->et_UniqueID == tid)
        {
            status = CHILD_ACTIVE;
            break;
        }
        }

#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&et->et_TaskMsgPort.mp_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif
        ForeachNode(&et->et_TaskMsgPort.mp_MsgList, child)
        {
        if (child->et_UniqueID == tid)
        {
            status = CHILD_EXITED;
            break;
        }
        }
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&et->et_TaskMsgPort.mp_SpinLock);
#endif
        Permit();
    }
    return status;

    AROS_LIBFUNC_EXIT
} /* ChildStatus */
