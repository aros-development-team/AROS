/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a task.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include <aros/config.h>
#include <proto/exec.h>
#include "etask.h"
#include "exec_util.h"

#include "exec_debug.h"
#ifndef DEBUG_AddTask
#   define DEBUG_AddTask 0
#endif
#undef DEBUG
#if DEBUG_AddTask
#   define DEBUG 1
#endif
#include <aros/debug.h>

/*****************************************************************************

    NAME */

	AROS_LH3(APTR, AddTask,

/*  SYNOPSIS */
	AROS_LHA(struct Task *,     task,      A1),
	AROS_LHA(APTR,              initialPC, A2),
	AROS_LHA(APTR,              finalPC,   A3),

/*  LOCATION */
	struct ExecBase *, SysBase, 47, Exec)

/*  FUNCTION
	Add a new task to the system. If the new task has the highest
	priority of all and task switches are allowed it will be started
	immediately.

	You must initialise certain fields, and allocate a stack before
	calling this function. The fields that must be initialised are:
	tc_SPLower, tc_SPUpper, tc_SPReg, and the tc_Node structure.

	If any other fields are initialised to zero, then they will be
	filled in with the system defaults.

	The value of tc_SPReg will be used as the location for the stack
	pointer. You can place any arguments you wish to pass to the Task
	onto the stack before calling AddTask(). However note that you may
	need to consider the stack direction on your processor.

	Memory can be added to the tc_MemEntry list and will be freed when
	the task dies. The new task's registers are set to 0.

    INPUTS
	task	  - Pointer to task structure.
	initialPC - Entry point for the new task.
	finalPC   - Routine that is called if the initialPC() function
		    returns. A NULL pointer installs the default finalizer.

    RESULT
	The address of the new task or NULL if the operation failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemTask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *t;
    
    D(bug("Call AddTask (%08lx (\"%s\"), %08lx, %08lx)\n"
	, task
	, task->tc_Node.ln_Name
	, initialPC
	, finalPC
    ));

    t = NewAddTask(task, initialPC, finalPC, NULL);
    
    ReturnPtr ("AddTask", struct Task *, t);
    
    AROS_LIBFUNC_EXIT
} /* AddTask */
