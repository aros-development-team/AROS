/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a semaphore from the list of public semaphores.
    Lang: english
*/

#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A1),

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the semaphore list */
    Forbid();

    /* Remove the semaphore */
    Remove(&sigSem->ss_Link);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemSemaphore */

