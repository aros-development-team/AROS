/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Lock a semaphore.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************/
#ifndef UseExecstubs

/*    NAME */
	#include <proto/exec.h>

	AROS_LH1(void, ObtainSemaphore,

/*    SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*    LOCATION */
	struct ExecBase *, SysBase, 94, Exec)

/*    FUNCTION
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
#else
void _Exec_ObtainSemaphore (struct SignalSemaphore * sigSem,
	struct ExecBase * SysBase)
#endif
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

