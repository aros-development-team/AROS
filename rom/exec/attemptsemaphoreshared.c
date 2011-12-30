/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to lock a semaphore shared.
    Lang: english
*/

#include <exec/semaphores.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, AttemptSemaphoreShared,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 120, Exec)

/*  FUNCTION
	Tries to get a shared lock on a signal semaphore. If the lock cannot
	be obtained false is returned. There may be more than one shared lock
	at a time but an exclusive lock prevents all other locks. The lock
	must be released with ReleaseSemaphore().

    INPUTS
	sigSem - pointer to semaphore structure

    RESULT
	True if the semaphore could be obtained, false otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseSemaphore()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("AttemptSemaphoreShared");

    return InternalAttemptSemaphore(sigSem, NULL, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphoreShared */
