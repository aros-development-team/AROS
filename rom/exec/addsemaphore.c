/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:55:56  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:03  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, AddSemaphore,

/*  SYNOPSIS */
	__AROS_LHA(struct SignalSemaphore *, sigSem, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 100, Exec)

/*  FUNCTION
	Adds a semaphore to the system public semaphore list. Since the
	semaphore gets initialized by this function it must be free at
	this time. Also the ln_Name field must be set.

    INPUTS
	sigSem - Pointer to semaphore structure

    RESULT

    NOTES
	Semaphores are shared between the tasks that use them and must
	therefore lie in public (or at least shared) memory.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    __AROS_FUNC_INIT

    /* Intialize semaphore */
    sigSem->ss_Link.ln_Type=NT_SIGNALSEM;
    InitSemaphore(sigSem);

    /* Arbitrate for the semaphore list */
    Forbid();
    /* Add the semaphore */
    Enqueue(&SysBase->SemaphoreList,&sigSem->ss_Link);

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* AddSemaphore */

