/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
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

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h
	22-01-96    fleischer implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    /* Prepare semaphore message to be a sent message */
    bidMsg->ssm_Message.mn_Length=sizeof(struct SemaphoreMessage);
    bidMsg->ssm_Message.mn_Node.ln_Type=NT_MESSAGE;
    bidMsg->ssm_Semaphore = (struct SignalSemaphore *)FindTask(NULL);

    if( (IPTR)(bidMsg->ssm_Message.mn_Node.ln_Name) == SM_SHARED )
    {
	bidMsg->ssm_Semaphore = NULL;
    }

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
	/*
	    We already have a structure for waiting, and we can't
	    allocate on the stack, so we use it instead. Note that
	    the SemaphoreRequest structure must be designed so that
	    it matches the SemapohreMessage structure
	*/
	AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)bidMsg);
    }
    /* All done. */
    Permit();

    /* Huh? */
    return 0;
    AROS_LIBFUNC_EXIT
} /* Procure */
