/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Release a semaphore.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************/
#ifndef UseExecstubs

/*    NAME  */
	#include <proto/exec.h>

	AROS_LH1(void, ReleaseSemaphore,

/*    SYNOPSIS  */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*    LOCATION  */
	struct ExecBase *, SysBase, 95, Exec)

/*    FUNCTION
	Releases a lock on a semaphore obtained with either ObtainSemaphore(),
	ObtainSemaphoreShared(), AttemptSemaphore or AttemptSemaphoreShared().
	Each call to one of those functions must be accompanied with one
	call to ReleasSemaphore().

    INPUTS
	sigSem - pointer to semaphore structure

    RESULT

    NOTES
	This function preserves all registers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    exec_lib.fd and clib/exec_protos.h
	22-01-96    fleischer implementation

*****************************************************************************/
#else
void _Exec_ReleaseSemaphore (struct SignalSemaphore * sigSem,
			struct ExecBase * SysBase)
#endif
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    /* Arbitrate for the semaphore structure */
    Forbid();

    /* Lower the use count. >0 means exclusive, <0 shared locked */
    if(sigSem->ss_NestCount>0)
	sigSem->ss_NestCount--;
    else
	sigSem->ss_NestCount++;

    /*
	Now if the semaphore is free and there are other tasks waiting
	wake them up.
    */
    if(!sigSem->ss_NestCount&&sigSem->ss_WaitQueue.mlh_Head->mln_Succ!=NULL)
    {
	/* Get first node in the waiting list */
	struct SemaphoreNode *sn;
	sn=(struct SemaphoreNode *)sigSem->ss_WaitQueue.mlh_Head;

	/* Is it a shared lock? */
	if((IPTR)sn->node.ln_Name!=SM_EXCLUSIVE)
	{
	    /* Yes. Process all shared locks in the list. */
	    while(sn->node.ln_Succ!=NULL)
	    {
		/* Remember node */
		struct SemaphoreNode *on=sn;

		/* Get next node now because there is a Remove() lurking */
		sn=(struct SemaphoreNode *)sn->node.ln_Succ;

		/* Is this a shared lock? */
		if((IPTR)on->node.ln_Name!=SM_EXCLUSIVE)
		{
		    /* Yes. Remove it from the list */
		    Remove(&on->node);

		    /*
			Mark the semaphore as having one more openers.
			This happens here because the new owner(s) may need
			some time to really wake up and I don't want other
			tasks obtaining the semaphore before him.
		    */
		    sigSem->ss_NestCount--;
		    
		    /* Dito. Invalidate the owner field. */
		    sigSem->ss_Owner=NULL;
		    
		    /* Wake the new owner. Check access type. */
		    if(on->node.ln_Pri==SN_TYPE_OBTAIN)
			/* ObtainSemaphore() type. Send the semaphore signal. */
			Signal(on->task,SEMAPHORESIGF);
		    else
		    {
			/* Procure() type. Reply the semaphore message. */
			((struct SemaphoreMessage *)on)->ssm_Semaphore=sigSem;
			ReplyMsg((struct Message *)on);
		    }

		}
	    }
	}else
	{
	    /* The new owner wants an exclusive lock. Remove him from the list. */
	    Remove(&sn->node);

	    /* Mark the semaphore as having one more openers. */
	    sigSem->ss_NestCount++;
	    
	    /* Check access type */
	    if(sn->node.ln_Pri==SN_TYPE_OBTAIN)
	    {
		/*
		    ObtainSemaphore() type. Set the owner field and
		    Send the semaphore signal.
		*/
		sigSem->ss_Owner=sn->task;
		Signal(sn->task,SEMAPHORESIGF);
	    }else
	    {
		/* Procure() type. Reply the message. */
		((struct SemaphoreMessage *)sn)->ssm_Semaphore=sigSem;
		sigSem->ss_Owner=((struct Message *)sn)->mn_ReplyPort->mp_SigTask;
		ReplyMsg((struct Message *)sn);
	    }
	}
    }

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* ReleaseSemaphore */
