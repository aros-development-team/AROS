/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release a lock obtained with Procure().
    Lang: english
*/
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#include "semaphores.h"
#include "exec_intern.h"

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SemaphoreRequest *sr = NULL;

    /* Arbitrate for the semaphore structure */
    Forbid();
    bidMsg->ssm_Semaphore = NULL;

    /*
     *	Two cases, the request is in the list, which means it hasn't been
     *	granted, or the request is not in the list, in which case it has
     *	been granted. We need to check if the request is in the list.
     */
    ForeachNode(&sigSem->ss_WaitQueue, sr)
    {
	if (sr == (struct SemaphoreRequest *)bidMsg)
	{
	    /* Found it. Remove it from the semaphore's waiting queue. */
	    Remove(&bidMsg->ssm_Message.mn_Node);
	    sigSem->ss_QueueCount--;

	    /* And reply the message. */
	    ReplyMsg(&bidMsg->ssm_Message);

	    /* All done */
	    Permit();
	    return;
	}
    }

    /* No, it must have been fulfilled. Release the semaphore and done. */
    ReleaseSemaphore(sigSem);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* Vacate */
