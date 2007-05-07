/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to lock a semaphore shared.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

#define CHECK_INITSEM 1

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

    struct Task *me = FindTask(NULL);
    ULONG retval = TRUE;
    
#if CHECK_INITSEM
    if (sigSem->ss_Link.ln_Type != NT_SIGNALSEM)
    {
        kprintf("\n\nAttemptSemaphoreShared called on an unintialized semaphore!!! "
	        "sem = %x  task = %x (%s)\n\n", sigSem, me, me->tc_Node.ln_Name);
    }
#endif

    /* Protect the semaphore structure */
    Forbid();

    /* Increment the queue count. This will need SMP protection */
    sigSem->ss_QueueCount++;

    if( sigSem->ss_QueueCount == 0 )
    {
	/* The semaphore wasn't owned. We can now own it */
	sigSem->ss_Owner = NULL;
	sigSem->ss_NestCount++;
    }
    else if( ( sigSem->ss_Owner == me ) || ( sigSem->ss_Owner == NULL ) )
    {
	/* The semaphore was owned by me or is shared, just increase the nest count */
	sigSem->ss_NestCount++;
    }
    else
    {
	/* We can't get ownership, just return it. */
	sigSem->ss_QueueCount--;
	retval = FALSE;
    }
    
    /* All done. */
    Permit();

    return retval;

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphoreShared */
