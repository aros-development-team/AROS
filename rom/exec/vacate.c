/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Release a lock obtained with Procure().
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, Vacate,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore  *, sigSem, A0),
	AROS_LHA(struct SemaphoreMessage *, bidMsg, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 91, Exec)

/*  FUNCTION
	Release a lock obtained with Procure. This will even work if the
	message is not yet replied - the request will be cancelled and the
	message replied. In any case the ssm_Semaphore field will be set to
	NULL.

    INPUTS
	sigSem - Pointer to semaphore structure.
	bidMsg - Pointer to struct SemaphoreMessage.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	Procure()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the semaphore structure */
    Forbid();

    ((struct SemaphoreRequest *)bidMsg)->sr_Waiter = NULL;

    /* Check if the message is still posted. */
    if(bidMsg->ssm_Message.mn_Node.ln_Type==NT_MESSAGE)
    {
	/* Yes. Remove it from the semaphore's waiting queue. */
	Remove(&bidMsg->ssm_Message.mn_Node);
	sigSem->ss_QueueCount--;

	/* And reply the message. */
	ReplyMsg(&bidMsg->ssm_Message);
    }
    else
	/* The semaphore is already locked. Release the lock. */
	ReleaseSemaphore(sigSem);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* Vacate */
