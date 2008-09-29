/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dispatch() - Tell the system that we have switched tasks.
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <exec/alerts.h>

#include <proto/arossupport.h>
#include <aros/asmcall.h>

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void, Dispatch,

/*  LOCATION */
	struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
	Inform SysBase that the task has been switched. This function
	only does all the non-system-dependant dispatching. It is up
	to the implementation to ensure that the tasks do actually get
	switched.

	The tc_Switch and tc_Launch functions will be called with
	SysBase in register A6.

    INPUTS
	None.

    RESULT
	The current task will have changed.

    NOTES

    EXAMPLE

    BUGS
	Not a good function to call.

    SEE ALSO
	Switch(), Reschedule()

    INTERNALS
	You can use your CPU dependant function as a wrapper around this
	function if you want. But you have to make sure then that you
	do NOT call Dispatch() from the exec.library function table.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *this, *task;

    this = SysBase->ThisTask;

    /* Check the stack... */

    /* Get the task which is ready to run */
    if(( task = (struct Task *)RemHead(&SysBase->TaskReady)))
    {
	if(this->tc_Flags & TF_SWITCH)
	{
	    AROS_UFC1(void, this->tc_Switch,
	    AROS_UFCA(struct ExecBase *, SysBase, A6));
	}

	this->tc_TDNestCnt = SysBase->TDNestCnt;
	this->tc_IDNestCnt = SysBase->IDNestCnt;
		
	/*  Oh dear, the previous task has just vanished...
	    you should have called Switch() instead :-)
		
	    We don't change the state of the old task, otherwise it
	    may never get freed.(See RemTask() for details).
		
	    We especially don't add it to the ReadyList !
	*/
	task->tc_State = TS_RUN;
		
	SysBase->TDNestCnt = task->tc_TDNestCnt;
	SysBase->IDNestCnt = task->tc_IDNestCnt;
		
	SysBase->ThisTask = task;

	/* Check the stack of the task we are about to launch */

	if( task->tc_SPReg <= task->tc_SPLower
	    || task->tc_SPReg >= task->tc_SPUpper )
	{
	    /* POW! */
	    Alert(AT_DeadEnd|AN_StackProbe);
	}
		
	if(task->tc_Flags & TF_LAUNCH)
	{
	    AROS_UFC1(void, task->tc_Launch,
	    AROS_UFCA(struct ExecBase *, SysBase, A6));
	}
		
	/* Increase the dispatched counter */
	SysBase->DispCount++;
    }
    else
    {
	kprintf("Eh? No tasks left to Dispatch()\n");

    /*
	We have reached a point where there are no ready tasks.
	What can you do? Well you can basically go into some kind of
	loop that can be interrupted easily. If you do this however
	you must re-enable interrupts.

	If you seem to go into endless loops with nothing happening,
	it could be that you have not enabled interrupts, or alternatively
	you could be running under emulation, disabled all signals and be
	getting some kind of process error (SIGBUS, SIGSEGV etc), which
	can't be delivered.

	If you do go idle, increase the counter please.

	You could also create a task (the idle task), which basically
	does nothing, and switch to that, in which case you should never
	get to here.

		SysBase->IdleCount++;
    */
    }

    /* Aha, the task pointed to be SysBase->ThisTask is correct. */

    AROS_LIBFUNC_EXIT
} /* Dispatch() */
