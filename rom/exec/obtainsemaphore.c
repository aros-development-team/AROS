/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:48  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:52  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:04  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:14  digulla
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

	AROS_LH1(void, ObtainSemaphore,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 94, Exec)

/*  FUNCTION
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

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h
	21-01-96    fleischer implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    struct Task *me;

    /* Get pointer to current task */
    me=SysBase->ThisTask;

    /* Arbitrate for the semaphore structure */
    Forbid();

    /* Check if the semaphore is in use by another task */
    if(sigSem->ss_NestCount&&sigSem->ss_Owner!=me)
    {
	/*
	    I wish there was some shared memory available as a semaphore node.
	    Unfortunately it isn't - and I cannot rely on being able to get
	    some from AllocMem() at this point either, so I have to use a part
	    of the stack instead :-(.
	*/
	struct SemaphoreNode sn;

	sn.node.ln_Pri =SN_TYPE_OBTAIN;
	sn.node.ln_Name=(char *)SM_EXCLUSIVE;
	sn.task        =me;

	/* Add the node to the semaphore's waiting queue. */
	AddTail((struct List *)&sigSem->ss_WaitQueue,&sn.node);

	/* Wait until the semaphore is free */
	Wait(SEMAPHORESIGF);

	/* ss_NestCount and ss_Owner are already set by ReleaseSemaphore() */
    }else
    {
	/* The semaphore is free - take it over */
	sigSem->ss_NestCount++;
	sigSem->ss_Owner=me;
    }

    /* All done */
    Permit();
    AROS_LIBFUNC_EXIT
} /* ObtainSemaphore */

