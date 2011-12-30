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

    /*
     * If there's no ThisTask, the function is called from within memory
     * allocator in exec's pre-init code. We are already single-threaded,
     * just return. :)
     */
    if (!me)
    	return;

    if (!CheckSemaphore(sigSem, &tp))
    {
        return;  /* A crude attempt to recover... */
    }

    /* Arbitrate for the semaphore structure */
    Forbid();

    /*
	ss_QueueCount == -1 indicates that the semaphore is
	free, so we increment this straight away. If it then
	equals 0, then we are the first to allocate this semaphore.

	Note: This will need protection for SMP machines.
    */
    sigSem->ss_QueueCount++;
    if( sigSem->ss_QueueCount == 0 )
    {
	/* We now own the semaphore. This is quick. */
	sigSem->ss_Owner = me;
	sigSem->ss_NestCount++;
    }

    /* The semaphore was in use, but was it by us? */
    else if( sigSem->ss_Owner == me )
    {
	/* Yes, just increase the nesting count */
	sigSem->ss_NestCount++;
    }

    /*
	Else, some other task must own it. We have
	to set a waiting request here.
    */
    else
    {
	/*
	    We need a node to mark our semaphore request. Lets use some
	    stack memory.
	*/
	struct SemaphoreRequest sr;
	sr.sr_Waiter = me;

	/*
	    Have to clear the signal to make sure that we don't
	    return immediately. We then add the SemReq to the
	    waiters list of the semaphore. We were the last to
	    request, so we must be the last to get the semaphore.
	*/

    	/* This must be atomic! */
    	AROS_ATOMIC_AND(me->tc_SigRecvd, ~SIGF_SINGLE);

	AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);

	/*
	    Finally, we simply wait, ReleaseSemaphore() will fill in
	    who owns the semaphore.
	*/
	Wait(SIGF_SINGLE);
    }

    /* All Done! */
    Permit();

    AROS_LIBFUNC_EXIT
} /* ObtainSemaphore */
