/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch() - Switch to the next available task.
    Lang: english
*/

#include <exec/execbase.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Switch,

/*  LOCATION */
	struct ExecBase *, SysBase, 9, Exec)

/*  FUNCTION
	Switch to the next task which wishes to be run. This function has
	a similar effect to calling Dispatch(), however it may be called
	at any time, and will not lose the current task if it is of type
	TS_RUN.

    INPUTS

    RESULT

    NOTES
	This function will preserve all its registers.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch(), Reschedule()

    INTERNALS
	If you want to have this function save all its registers, you
	should replace this function in $(KERNEL) or $(ARCH).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *this = SysBase->ThisTask;

    /*
	If the state is not TS_RUN then the task is already in a list
    */

    /*
       This task (= the task that's running in the moment) is
       moved to the task ready list with Reschedule(), if necessary.
       After that a new task is taken from the task-ready list
       and is launched by doing the context switch. This happens
       in Dispatch(). 
    */
return;

    if( (this->tc_State == TS_RUN)
	&& !(this->tc_Flags & TF_EXCEPT) )
    {
	this->tc_State = TS_READY;

	/* Use Reschedule() to put the task in the correct list. */
	Reschedule(this);
    }

    /* Call the dispatcher proper. */
    Dispatch();

    AROS_LIBFUNC_EXIT
} /* Switch() */
