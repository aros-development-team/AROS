/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch() - Switch to the next available task.
    Lang: english
*/

#include <aros/asmcall.h>

#include "etask.h"
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH0(void *, Switch,

/*  LOCATION */
	struct ExecBase *, SysBase, 9, Exec)

/*  FUNCTION
	Notify exec.library about task switch

    INPUTS
	None.

    RESULT
	A pointer to a CPU context storage area for current task.

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
	Dispatch(), Reschedule()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task = SysBase->ThisTask;

    /* store IDNestCnt into tasks's structure */  
    task->tc_IDNestCnt = SysBase->IDNestCnt;

    /* Upon leaving supervisor mode interrupts will be enabled
       if nothing changes in Dispatch() */
    SysBase->IDNestCnt = -1;

    /* TF_SWITCH flag set? Call the switch routine */
    if (task->tc_Flags & TF_SWITCH)
	AROS_UFC1(void, task->tc_Switch,
		  AROS_UFCA(struct ExecBase *, SysBase, A6));

    /* Return context storage area. The caller is now suggested to save
       task's context there. */
    return GetIntETask(task)->iet_Context;

    AROS_LIBFUNC_EXIT
} /* Switch() */
