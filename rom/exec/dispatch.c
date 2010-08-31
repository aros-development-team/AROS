/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

#include "etask.h"

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void *, Dispatch,

/*  LOCATION */
	struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
	Inform SysBase that the task has been switched. This function
	only does all the non-system-dependant dispatching. It is up
	to kernel.resource to do actual context switch.

	The tc_Launch function will be called with SysBase in register A6.

    INPUTS
	None.

    RESULT
	A pointer to CPU context storage area for a new task or NULL
	if there are no ready tasks.

    NOTES
	In AmigaOS this was a private function. In AROS this function
	should be called only from within kernel.resource's lowlevel task
	switcher.
	There's no practical sense in calling this function from within
	any user software.

	This code normally runs in supervisor mode.

    EXAMPLE

    BUGS

    SEE ALSO
	Switch(), Reschedule()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task;

    /* Get the task which is ready to run */
    if(( task = (struct Task *)RemHead(&SysBase->TaskReady)))
    {	
	/*  Oh dear, the previous task has just vanished... */
	task->tc_State = TS_RUN;

	SysBase->IDNestCnt = task->tc_IDNestCnt;
	SysBase->ThisTask = task;
	SysBase->Elapsed = SysBase->Quantum;
        SysBase->SysFlags &= ~SFF_QuantumOver;

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

	/* Return context storage area. The caller is expected to
	   restore task's context from it. */
	return GetIntETask(task)->iet_Context;
    }
    else
    {

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
	return NULL;
    }

    AROS_LIBFUNC_EXIT
} /* Dispatch() */
