/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Reschedule() - Put a task back into the ready or waiting list.
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(void, Reschedule,

/*  SYNOPSIS */
	AROS_LHA(struct Task *, task, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 8, Exec)

/*  FUNCTION
	Reschedule will place the task into one of Execs internal task
	lists. Which list it is placed in will depend upon whether the
	task is ready to run, or whether it is waiting for an external
	event to awaken it.

	It is possible that in the future, more efficient schedulers
	will be implemented. In which case this is the function that they
	need to implement.

	You should not do any costly calculations since you will be
	running in interupt mode.

    INPUTS
	task    -   The task to insert into the list.

    RESULT
	The task will be inserted into one of Exec's task lists.

    NOTES
	Not actually complete yet. Some of the task states don't have any
	supplied action.

    EXAMPLE

    BUGS
	Only in relation to the comments within about low-priority tasks
	not getting any processor time.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
	The code in here defines how "good" the task switching is.
	There are seveal things which should be taken into account:

	1. No task should block the CPU forever even if it is an
	    endless loop.

	2. Tasks with a higher priority should get the CPU more often.

	3. Tasks with a low priority should get the CPU every now and then.

	Enqueue() fulfills 2 but not 1 and 3. AddTail() fulfills 1 and 3.

	A better way would be to "deteriorate" a task, ie. decrease the
	priority and use Enqueue() but at this time, I can't do this
	because I have no good way to extend the task structure (I
	need a variable to store the original prio).
    */

   /* Somebody had better have set the tasks state properly */

    switch(task->tc_State)
    {
	case TS_READY:
            Enqueue(&SysBase->TaskReady, (struct Node *)task);
            break;

	case TS_ADDED:
	    Enqueue(&SysBase->TaskReady, (struct Node *)task);
	    break;

	/*
	    We don't need to Enqueue() onto the TaskWait list,
	    as it is not sorted - saves quite a few cycles in
	    the long run.
	*/
	case TS_WAIT:
	    AddTail(&SysBase->TaskWait, (struct Node *)task);
	    break;

	case TS_REMOVED:
	    /* Ideally we should free the task here, but we can't
	       because that would corrupt the memory lists.
	    */
	    break;

	case TS_INVALID:
	case TS_EXCEPT:
	case TS_RUN:
	    /* We should never be called with this state. */
	    ASSERT(FALSE);
	    break;
    }

    AROS_LIBFUNC_EXIT
} /* Reschedule */
