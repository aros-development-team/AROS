/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:58  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:09  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:21  digulla
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
    AROS_LIBFUNC_EXIT
} /* Vacate */

