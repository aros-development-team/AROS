/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock a semaphore.
    Lang: english
*/

#include <exec/semaphores.h>
#include <aros/atomic.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "semaphores.h"

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*    NAME */
	#include <proto/exec.h>

	AROS_LH1(void, ObtainSemaphore,

/*    SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*    LOCATION */
	struct ExecBase *, SysBase, 94, Exec)

/*    FUNCTION
	Obtain an exclusive lock on a semaphore. If the semaphore is already
	in use by another task this function will wait until the semaphore
	becomes free.

    INPUTS
	sigSem - Pointer to semaphore structure

    RESULT

    NOTES
	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseSemaphore()

    INTERNALS

*****************************************************************************/
{
#undef  Exec

    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("ObtainSemaphore");
    struct Task *me = FindTask(NULL);

    InternalObtainSemaphore(sigSem, me, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* ObtainSemaphore */
