/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH1I(void, AddSemaphore,

/*  SYNOPSIS */
	__AROS_LA(struct SignalSemaphore *, sigSem, A1),

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

