/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to lock a semaphore.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, Procure,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore  *, sigSem, A0),
	AROS_LHA(struct SemaphoreMessage *, bidMsg, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 90, Exec)

/*  FUNCTION
	Tries to get a lock on a semaphore in an asynchronous manner.
	If the semaphore is not free this function will not wait but
	just post a request to the semaphore. As soon as the semaphore is
	available the bidMsg will return and make you owner of the semaphore.

    INPUTS
	sigSem - pointer to semaphore structure
	bidMsg - pointer to a struct SemaphoreMessage. This should lie in
		 public or at least shared memory.

    RESULT
	Principly none. Don't know. Just ignore it.

    NOTES
	Locks obtained with Procure() must be released with Vacate().

    EXAMPLE

    BUGS

    SEE ALSO
	Vacate()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    /* Prepare semaphore message to be a sent message */
    bidMsg->ssm_Message.mn_Length=sizeof(struct SemaphoreMessage);

    /*
     *	If ln_Name field of message == 1, then this is a shared message, so
     *	we set the field to NULL. Otherwise, set it to the task requesting
     *	the semaphore. By the end, the ssm_Semaphore field contains the new
     *	owner of the semaphore.
     */
    if( (IPTR)(bidMsg->ssm_Message.mn_Node.ln_Name) == SM_SHARED )
	bidMsg->ssm_Semaphore = NULL;
    else
	bidMsg->ssm_Semaphore = (struct SignalSemaphore *)FindTask(NULL);

    /* Arbitrate for the semaphore structure - following like ObtainSema() */
    Forbid();

    sigSem->ss_QueueCount++;
    /*
	Check if we own it
    */
    if( sigSem->ss_QueueCount == 0 )
    {
	/* No, and neither does anybody else - claim it as ours. */
	sigSem->ss_Owner = (struct Task *)bidMsg->ssm_Semaphore;
	sigSem->ss_NestCount++;
	bidMsg->ssm_Semaphore = sigSem;
	ReplyMsg((struct Message *)sigSem);
    }
    /*
	Otherwise check if we already own it
    */
    else if( sigSem->ss_Owner == (struct Task *)bidMsg->ssm_Semaphore )
    {
	/* Yes we do... */
	sigSem->ss_NestCount++;
	bidMsg->ssm_Semaphore = sigSem;
	ReplyMsg((struct Message *)sigSem);
    }

    /*
	It is owned by somebody else, set up the waiter
    */
    else
    {
	struct SemaphoreRequest *sr;
	/*
	 *  Unholy Hack v1.
	 *
	 *  Whoever came up with this obviously has a twisted mind. We
	 *  pretend that the SemaphoreMessage is really a SemaphoreRequest.
	 *  Now, the code in ReleaseSemaphore that deals with this checks
	 *  (after clearing bit 0) whether the sr_Waiter field is NULL. If
	 *  so then it is really a SemaphoreMessage and should be returned.
	 *
	 *  Now the bad part about this is what we are doing. There are two
	 *  cases, in the Amiga case (bincompat), we are overwriting the
	 *  ln_Type, ln_Pri and (half the) ln_Name fields. In the other
	 *  case we are overwriting the ln_Name field completely. Now, in
	 *  either case this doesn't matter as they do not contain any
	 *  useful information that we haven't already claimed.
	 *
	 *  Thank goodness C is so type unsafe.
	 */
	sr = (struct SemaphoreRequest *)bidMsg;
	if (bidMsg->ssm_Semaphore != NULL)
	    sr->sr_Waiter = NULL;
	else
	    sr->sr_Waiter = (APTR)SM_SHARED;

	AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)bidMsg);
    }
    /* All done. */
    Permit();

    /* Huh? */
    return 0;
    AROS_LIBFUNC_EXIT
} /* Procure */
