/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to lock a sempahore.
    Lang: english
*/

#include <exec/semaphores.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, AttemptSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 96, Exec)

/*  FUNCTION
	Tries to get an exclusive lock on a signal semaphore. If the semaphore
	is already in use by another task, this function does not wait but
	returns false instead.

    INPUTS
	sigSem - Pointer so semaphore structure.

    RESULT
	TRUE if the semaphore could be obtained, FALSE otherwise.

    NOTES
	The lock must be freed with ReleaseSemaphore().

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseSemaphore()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("AttemptSemaphore");
    struct Task *me = FindTask(NULL);

    return InternalAttemptSemaphore(sigSem, me, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphore */
