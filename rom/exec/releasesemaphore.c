/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release a semaphore.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

#define CHECK_INITSEM 	1
#define CHECK_TASK	0 /* it seems to be legal to call ObtainSemaphore in one task and ReleaseSemaphore in another */

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

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

*****************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

#if CHECK_INITSEM
    if (sigSem->ss_Link.ln_Type != NT_SIGNALSEM)
    {
        kprintf("\n\nReleaseSemaphore called on an unintialized semaphore!!! "
	        "sem = %x  task = %x (%s)\n\n", sigSem, FindTask(0), FindTask(0)->tc_Node.ln_Name);
    }
#endif

    /* Protect the semaphore srtucture from multiple access. */
    Forbid();

    /* Release one on the nest count */
    sigSem->ss_NestCount--;
    sigSem->ss_QueueCount--;

    if(sigSem->ss_NestCount == 0)
    {
	/*
	    There are two cases here. Either we are a shared
	    semaphore, or not. If we are not, make sure that the
	    correct Task is calling ReleaseSemaphore()
	*/
	
	#if CHECK_TASK
	if( sigSem->ss_Owner != NULL && sigSem->ss_Owner != FindTask(NULL) )
	{
	    /*
		If it is not, there is a chance that the semaphore
		is corrupt. It will be afterwards anyway :-) 
	    */
	    Alert( AN_SemCorrupt );
	}
	#endif

	/*
	    Do not try and wake anything unless there are a number
	    of tasks waiting. We do both the tests, this is another
	    opportunity to throw an alert if there is an error.
	*/
	if(
	    sigSem->ss_QueueCount >= 0
	 && sigSem->ss_WaitQueue.mlh_Head->mln_Succ != NULL
	)
	{
	    struct SemaphoreRequest *sr, *srn;

	    /*
		Look at the first node, but only to see whether it
		is shared or not.
	    */
	    sr = (struct SemaphoreRequest *)sigSem->ss_WaitQueue.mlh_Head;

	    /*
		A node is shared if the ln_Name/sr_Waiter field is
		odd (ie it has bit 1 set).

		If the sr_Waiter field is != NULL, then this is a
		task waiting, otherwise it is a message.
	    */
	    if( ((IPTR)sr->sr_Waiter & SM_SHARED) == SM_SHARED )
	    {
		/* This is a shared lock, so ss_Owner == NULL */
		sigSem->ss_Owner = NULL;

		/* Go through all the nodes to find the shared ones */
		ForeachNodeSafe( &sigSem->ss_WaitQueue, sr, srn)
		{
		    srn = (struct SemaphoreRequest *)sr->sr_Link.mln_Succ;

		    if( ((IPTR)sr->sr_Waiter & SM_SHARED) == SM_SHARED )
		    {
			Remove((struct Node *)sr);

			/* Clear the bit, and update the owner count */
			sr->sr_Waiter = (APTR)((IPTR)sr->sr_Waiter & ~1);
			sigSem->ss_NestCount++;

			if(sr->sr_Waiter != NULL)
			{
			    /* This is a task, signal it */
			    Signal(sr->sr_Waiter, SIGF_SINGLE);
			}
			else
			{
			    /* This is a message, send it back to its owner */
			    ((struct SemaphoreMessage *)sr)->ssm_Semaphore = sigSem;
			    ReplyMsg((struct Message *)sr);
			}
		    }
		}
	    }

	    /*	This is an exclusive lock - awaken first node */
	    else
	    {
		/* Save typing */
		struct SemaphoreMessage *sm = (struct SemaphoreMessage *)sr;

		/* Only awaken the first of the nodes */
		Remove((struct Node *)sr);
		sigSem->ss_NestCount++;

		if(sr->sr_Waiter != NULL)
		{
		    sigSem->ss_Owner = sr->sr_Waiter;
		    Signal(sr->sr_Waiter, SIGF_SINGLE);
		}
		else
		{
		    sigSem->ss_Owner = (struct Task *)sm->ssm_Semaphore;
		    sm->ssm_Semaphore = sigSem;
		    ReplyMsg((struct Message *)sr);
		}
	    }
	} /* there are waiters */

	/*  Otherwise, there are not tasks waiting. */
	else
	{
	    sigSem->ss_Owner = NULL;
	    sigSem->ss_QueueCount = -1;

	    D(bug("ReleaseSemaphore(): No tasks - ss_NestCount == %ld\n",
		sigSem->ss_NestCount));
	}
    }
    else if(sigSem->ss_NestCount < 0)
    {
	/*
	    This can't happen. It means that somebody has released
	    more times than they have obtained.
	*/
	Alert( AN_SemCorrupt );
    }

    /* All done. */
    Permit();

    AROS_LIBFUNC_EXIT
} /* ReleaseSemaphore */
