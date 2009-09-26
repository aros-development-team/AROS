/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to lock a sempahore.
    Lang: english
*/
#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

#define CHECK_INITSEM 1

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

    struct Task *me = FindTask(NULL);

#if CHECK_INITSEM
    if (sigSem->ss_Link.ln_Type != NT_SIGNALSEM)
    {
        kprintf("\n\nAttemptSemaphore called on an uninitialized semaphore!!! "
	        "sem = %x  task = %x (%s)\n\n", sigSem, me, me->tc_Node.ln_Name);
    }
#endif

    /* Arbitrate for semaphore nesting count and owner fields */
    Forbid();

    /*
	We are going to lock or fail, so we increment the
	ss_QueueCount anyway. We shall fix it up later if it was
	wrong.
    */
    sigSem->ss_QueueCount++;

    /*	If it is now equal to zero, then we have got it */
    if( sigSem->ss_QueueCount == 0 )
    {
	sigSem->ss_Owner = me;
	sigSem->ss_NestCount++;
    }
    /*	It was already owned by me, so lets just inc the nest count */
    else if( sigSem->ss_Owner == me )
    {
	sigSem->ss_NestCount++;
    }

    /*	Owned by somebody else, we fix up the owner count. */
    else
    {
	sigSem->ss_QueueCount--;
    }

    Permit();

    /*
	We own the semaphore if it is owned by me
    */
    return (sigSem->ss_Owner == me ? TRUE : FALSE);

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphore */
