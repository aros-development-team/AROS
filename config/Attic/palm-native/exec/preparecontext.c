/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <asm/ptrace.h>

/*****************************************************************************

    NAME */

	AROS_LH3(BOOL, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(struct Task *, task, A0),
	AROS_LHA(APTR, entryPoint,   A1),
	AROS_LHA(APTR, fallBack,     A2),

/*  LOCATION */
	struct ExecBase *, SysBase, 6, Exec)

/*  FUNCTION
	Allocates the space required to hold a new set of registers on the
	Stack given by stackPointer and clears the area except for pc which
	is set to the address given by entryPoint.

    INPUTS
	task         - Pointer to the new task
	entryPoint   - Address of the function to call when the new context
		       becomes active.
	fallBack     - Address to be called when the entryPoint function ended
		       with an rts.

    RESULT
	The new Stackpointer with the underlying context.

    NOTES
	This function is for internal use by exec only.

	This function is processor dependant.

    EXAMPLE

    BUGS

    SEE ALSO
	SwitchTasks()

    INTERNALS

    HISTORY

******************************************************************************/
{
	AROS_LIBFUNC_INIT
	ULONG * sp = (APTR)task->tc_SPReg;
	struct pt_regs *pr=(struct pt_regs *)((ULONG)task->tc_SPReg - sizeof(struct pt_regs) - sizeof(APTR));
	int i = 0;

#warning Might need more args (SysBase)!
	*sp = (ULONG)fallBack;

	while (i < sizeof(struct pt_regs)) {
		((char *)pr)[i] = 0;
		i++;
	}

	pr->usp = sp;
	pr->pc  = (ULONG)entryPoint;

	task->tc_SPReg = (APTR)pr;

	return TRUE;
	AROS_LIBFUNC_EXIT
}
