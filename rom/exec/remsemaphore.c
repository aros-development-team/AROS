/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include "exec_intern.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(void, RemSemaphore,

/*  SYNOPSIS */
	__AROS_LA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 101, Exec)

/*  FUNCTION
	Removes a semaphore from the system public semaphore list.

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

    /* Arbitrate for the semaphore list */
    Forbid();

    /* Remove the semaphore */
    Remove(&sigSem->ss_Link);

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* RemSemaphore */

