/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/ptrace.h>
#include "etask.h"
#include "exec_util.h"

//#error "PrepareContext() has been changed. Additional tagList param, etc."
//#error "This one here needs to be rewritten!"

/*****************************************************************************

    NAME */

	AROS_LH4(BOOL, PrepareContext,

/*  SYNOPSIS */
	AROS_LHA(struct Task *, task, A0),
	AROS_LHA(APTR, entryPoint,   A1),
	AROS_LHA(APTR, fallBack,     A2),
	AROS_LHA(struct TagItem *, tagList, A3),

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
	tagList      - 

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

	struct pt_regs *regs;
	
	UBYTE *sp = (UBYTE *)task->tc_SPReg;
	
	/* Push fallBack address */
	sp -= sizeof(APTR);
	*(APTR *)sp = fallBack;
	
	if (!(task->tc_Flags & TF_ETASK)) {
	  *(ULONG *)0x0bad0001=0;
	  return FALSE;
	}
	  
	GetIntETask (task)->iet_Context = AllocTaskMem (task
	    , SIZEOF_ALL_REGISTERS
	    , MEMF_PUBLIC|MEMF_CLEAR
	);
	
	if (!(regs  = (struct pt_regs *)GetIntETask(task)->iet_Context)) {
	  *(ULONG *)0x0bad0003=0;
	  return FALSE;
	}
	
	regs->usp = sp;
	regs->pc  = (ULONG)entryPoint;
	
	task->tc_SPReg = sp;
	
	return TRUE;
	AROS_LIBFUNC_EXIT
}
