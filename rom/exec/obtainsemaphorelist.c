/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock all semaphores in the list at once.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, ObtainSemaphoreList,

/*  SYNOPSIS */
	AROS_LHA(struct List *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 97, Exec)

/*  FUNCTION
	This function takes a list of semaphores and locks all of them at
	once. It is only possible for one task to attempt to lock all the
	semaphores at once (since it uses the ss_MultipleLink field), so
	you will need to protect the entire list (with another semaphore
	perhaps?).

	If somebody attempts to lock more than one semaphore on this list
	with ObtainSemaphore() it is possible for a deadlock to occur due
	to two tasks waiting for a semaphore that the other has obtained.

    INPUTS
	sigSem - pointer to list full of semaphores

    RESULT
	The entire semaphore list will be locked.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SignalSemaphore *ss;
    struct Task * const me = FindTask(NULL);
    WORD failedObtain = 0;

    /*
     *	The algorithm here is to attempt to lock all the semaphores in the
     *	list, and if any fail, to post a SemaphoreRequest for the
     *	semaphore.
     *
     *	If we succeed in locking them all, we can return successfully,
     *	otherwise we must wait for the remaining semaphores to become
     *	available.
     *
     *	Note that we sleep on each un-obtained semaphore as we pass through
     *	the list. This way by the time we get to the end of the list we can
     *	be sure that we have obtained all the semaphores. It is possible
     *	that whilst we are waiting for one semaphore, one later in the list
     *	will be granted to us. In that case we do not have to wait for it.
     */
     
    Forbid();

    ForeachNode(sigSem, ss)
    {
	/* QueueCount == -1 means unlocked */
	ss->ss_QueueCount++;
	if(ss->ss_QueueCount != 0)
	{
    	    /* sem already locked by me? */
            if (ss->ss_Owner != me) 
            {
		/*
		 *	Locked by someone else, post a wait message. We use the field
		 *	ss_MultipleLink, which is why this function requires an
		 *	external arbitrator.
		 */
		ss->ss_MultipleLink.sr_Waiter = me;
		AddTail
		(
		    (struct List *)&ss->ss_WaitQueue,
		    (struct Node *)&ss->ss_MultipleLink
		);
		failedObtain++;
	    }
	    else
	    {
	    	/* Already locked by me */
	    	ss->ss_NestCount++;
	    }
 	}
	else
	{
	    /* We have it... */
	    ss->ss_NestCount++;
	    ss->ss_Owner = me;
	}
    }

    if(failedObtain > 0)
    {
	ss = (struct SignalSemaphore *)sigSem->lh_Head;

	while(ss->ss_Link.ln_Succ != NULL)
	{
	    if(ss->ss_Owner != me)
	    {
		/*
		 *  Somebody else has this one. Wait, then check again.
		 *  Check again because the signal could have been for a
		 *  different semaphore in the list we are waiting for.
		 */
		Wait(SIGF_SINGLE);
	    }
	    else
	    {
		/* We got it, go on to the next one */
		ss = (struct SignalSemaphore *)ss->ss_Link.ln_Succ;
		failedObtain--;
	    }
	}
    }

#ifndef NO_CONSISTENCY_CHECKS
    if(failedObtain != 0)
    {
	kprintf("\n\nObtainSemaList: Obtained count mismatch %d\n", failedObtain);
	Alert(AN_BadSemaphore);
    }
#endif

    Permit();

    AROS_LIBFUNC_EXIT
} /* ObtainSemaphoreList */
