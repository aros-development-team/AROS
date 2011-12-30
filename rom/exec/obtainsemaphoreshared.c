/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a shared lock on a semaphore.
    Lang: english
*/

#include <exec/semaphores.h>
#include <aros/atomic.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "semaphores.h"

#define CHECK_INITSEM 1

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
	#include <proto/exec.h>

	AROS_LH1(void, ObtainSemaphoreShared,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 113, Exec)

/*  FUNCTION
	Get a shared lock on a semaphore. If the lock cannot be obtained
	immediately this function waits. There may be more than one shared
	locks at the same time but only one exclusive one. An exclusive
	lock prevents shared locks. Shared locks are released with
	ReleaseSemaphore().

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
#undef Exec

    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("ObtainSemaphoreShared");

    InternalObtainSemaphore(sigSem, NULL, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* ObtainSemaphoreShared */
