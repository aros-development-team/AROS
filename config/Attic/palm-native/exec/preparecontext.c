/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.1  2001/05/30 00:18:07  bergers
    Exec functions derived from the i386 code. Need some more work and also task switching with the real time clock is not a good thing. Might want to change this to use one of the two timers. Currently switches tasks at rate of 1Hz.

    Revision 1.2  1998/10/20 16:43:47  hkiel
    Amiga Research OS

    Revision 1.1  1997/03/14 18:36:08  ldp
    Moved files

    Revision 1.4  1996/10/24 15:51:31  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/10/20 02:57:47  aros
    Changed AROS_LA to AROS_LHA

    Revision 1.2  1996/08/01 17:41:36  digulla
    Added standard header for all files

    Desc:
    Lang:
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
