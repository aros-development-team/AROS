/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/09/13 17:51:23  digulla
    Use IPTR

    Revision 1.4  1996/08/13 13:56:05  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:15  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH2(ULONG, Procure,

/*  SYNOPSIS */
	__AROS_LHA(struct SignalSemaphore  *, sigSem, A0),
	__AROS_LHA(struct SemaphoreMessage *, bidMsg, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 90, Exec)

/*  FUNCTION
	Tries to get a lock on a semaphore in an asynchronous manner.
	If the semaphore is not free this function will not wait but
	just post a request to the semaphore. As soon as the semaphore is
	available the bisMsg will return and make you owner of the semaphore.

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)

    /* Prepare semaphore message to be a sent message */
    bidMsg->ssm_Message.mn_Length=sizeof(struct SemaphoreMessage);
    bidMsg->ssm_Message.mn_Node.ln_Type=NT_MESSAGE;

    /* Arbitrate for the semaphore structure */
    Forbid();

    /* Try to get the semaphore immediately. */
    if((IPTR)(bidMsg->ssm_Message.mn_Node.ln_Name)==SM_SHARED?
       AttemptSemaphoreShared(sigSem):AttemptSemaphore(sigSem))
    {
	/* Got it. Reply the message. */
	bidMsg->ssm_Semaphore=sigSem;
	ReplyMsg(&bidMsg->ssm_Message);
    }
    else
	/* Couldn't get it. Add message to the semaphore's waiting queue. */
	AddTail((struct List *)&sigSem->ss_WaitQueue,&bidMsg->ssm_Message.mn_Node);

    /* All done. */
    Permit();

    /* Huh? */
    return 0;
    __AROS_FUNC_EXIT
} /* Procure */

