/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

/******************************************************************************

    NAME
	#include <proto/exec.h>

	AROS_LH0(void, Switch,

    LOCATION
	struct ExecBase *, SysBase, 9, Exec)

    FUNCTION
	Tries to switch to the first task in the ready list. This
	function works almost like Dispatch() with the slight difference
	that it may be called at any time and as often as you want and
	that it does not lose the current task if it is of type TS_RUN.

    INPUTS

    RESULT

    NOTES
	This function is CPU dependant.

	This function is for internal use by exec only.

	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO
	Dispatch()

    INTERNALS

    HISTORY

******************************************************************************/

void _Switch (struct ExecBase * SysBase)
{
    struct Task * task = FindTask (NULL);

    if (task->tc_State != TS_RUN && !(task->tc_Flags & TF_EXCEPT) )
    {
	task->tc_State = TS_READY;
	Enqueue (&SysBase->TaskReady, (struct Node *)task);
    }
} /* _Switch */
