/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:24  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */
	#include <exec/semaphores.h>
	#include <clib/exec_protos.h>

	__AROS_LH2I(void, Vacate,

/*  SYNOPSIS */
	__AROS_LA(struct SignalSemaphore  *, sigSem, A0),
	__AROS_LA(struct SemaphoreMessage *, bidMsg, A1),

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
    __AROS_FUNC_INIT

    /* Arbitrate for the semaphore structure */
    Forbid();

    /* Check if the message is still posted. */
    if(bidMsg->ssm_Message.mn_Node.ln_Type==NT_MESSAGE)
    {
	/* Yes. Remove it from the semaphore's waiting queue. */
	Remove(&bidMsg->ssm_Message.mn_Node);

	/* And reply the message. */
	ReplyMsg(&bidMsg->ssm_Message);
    }else
	/* The semaphore is already locked. Release the lock. */
	ReleaseSemaphore(sigSem);

    /* Clear the semaphore field. */
    bidMsg->ssm_Semaphore=NULL;

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* Vacate */

