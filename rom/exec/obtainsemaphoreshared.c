/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME
	#include <proto/exec.h>

	AROS_LH1(void, ObtainSemaphoreShared,

    SYNOPSIS
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

    LOCATION
	struct ExecBase *, SysBase, 113, Exec)

    FUNCTION
	Get a shared lock on a semaphore. If the lock cannot be obtained
	immediately this function waits. There may be more than one shared
	locks at the same time but only one exclusive one. An exclusive
	lock prevents shared locks. Shared locks are released with
	ReleaseSemaphore().

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

void _ObtainSemaphoreShared (struct SignalSemaphore * sigSem,
			    struct ExecBase * SysBase)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    struct Task *me;

    /* Get pointer to current task */
    me=SysBase->ThisTask;

    /* Arbitrate for the semaphore structure */
    Forbid();

    /* Check if there's an exclusive lock on the semaphore */
    if(sigSem->ss_NestCount>0)
    {
	/* Yes. Is it owned by the current task? */
	if(sigSem->ss_Owner!=me)
	{
	    /* No. Prepare a node for the waiting queue. */
	    struct SemaphoreNode sn;
	    sn.node.ln_Pri =SN_TYPE_OBTAIN;
	    sn.node.ln_Name=(char *)SM_SHARED;
	    sn.task	   =me;

	    /* Add it. */
	    AddTail((struct List *)&sigSem->ss_WaitQueue,&sn.node);

	    /* Wait until the semaphore is free */
	    Wait(SEMAPHORESIGF);

	    /* ss_NestCount and ss_Owner are already set by ReleaseSemaphore() */
	}else
	    /* Add one nesting level more */
	    sigSem->ss_NestCount++;
    }else
    {
	/* There's no exclusive lock on the semaphore. Get a shared one. */
	sigSem->ss_NestCount--;

	/* Invalidate the owner field - so no task may think this semaphore is his. */
	sigSem->ss_Owner=NULL;
    }

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* ObtainSemaphoreShared */

