/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get a shared lock on a semaphore.
    Lang: english
*/
#include "exec_intern.h"
#include "semaphores.h"
#include <exec/semaphores.h>
#include <aros/atomic.h>
#include <proto/exec.h>

#define CHECK_INITSEM 1

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
	#include <proto/exec.h>

	AROS_LH1(void, ObtainSemaphoreShared,

/*  SYNOPSIS */
	AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 113, Exec)

/*  FUNCTION
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

*****************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    ASSERT_VALID_PTR(sigSem);
    
    /* Get pointer to current task */
    struct Task *me = SysBase->ThisTask;

#if CHECK_INITSEM
    if (sigSem->ss_Link.ln_Type != NT_SIGNALSEM)
    {
        kprintf("\n\nObtainSemaphoreShared called on a not intialized semaphore!!! "
	        "sem = %x  task = %x (%s)\n\n", sigSem, me, me->tc_Node.ln_Name);

       Alert(AN_SemCorrupt);
    }
#endif

    /* Arbitrate for the semaphore structure */
    Forbid();

    /*
	ss_QueueCount == -1 indicates that the semaphore is
	free, so we increment this straight away. If it then
	equals 0, then we are the first to allocate this semaphore.

	Note: This will need protection for SMP machines.
    */
    sigSem->ss_QueueCount++;

    if( sigSem->ss_QueueCount == 0 )
    {
	/*
	    We now own the semaphore. This is quick.
	    A shared semaphore does not have an owner, so we
	    mark the semaphore as shared by ss_Owner == NULL
	*/
        sigSem->ss_Owner = NULL;
        sigSem->ss_NestCount++;
    }

    /*
	The semaphore is in use, but it could be shared. if it is,
	ss_Owner == NULL. Or it could already be exclusively owned
	by me. if it is, ss_Owner == me.
    */
    else if( (sigSem->ss_Owner == me) || ( sigSem->ss_Owner == NULL ) )
    {
	/* Yes, just increase the nesting count */
        sigSem->ss_NestCount++;
    }

    /*
	Otherwise it is an exclusive semaphore owned by someone else,
	and we have to wait for it. This is pretty simple, we simply do
	the same as for ObtainSemaphore(), but set that this is a
	shared semaphore.
    */
    else
    {
	/*
	    We need a node to mark our semaphore request. Lets use some
	    stack memory. This is nasty, but to mark that this is a 
	    shared waiter we mark the ss_Waiter field with an odd
	    address. This is valid simply because we never run on an
	    architecture where an odd address is a valid task structure.
	*/
        struct SemaphoreRequest sr;
        
        sr.sr_Waiter = (struct Task *) ((UBYTE *) me + 1);

        bug("Task = %8lx, Waiter = %8lx\n", me, sr.sr_Waiter);

	/*
	    Have to clear the signal to make sure that we don't
	    return immediately. We then add the SemReq to the
	    waiters list of the semaphore. We were the last to
	    request, so we must be the last to get the semaphore.
	*/
    	
#warning this must be atomic
        AROS_ATOMIC_AND(me->tc_SigRecvd, ~SIGF_SINGLE);
        
        AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);

	/*
	    Finally, we simply wait, ReleaseSemaphore() will fill in
	    who owns the semaphore.
	*/
        Wait(SIGF_SINGLE);
    }

    /* All Done! */
    Permit();

    AROS_LIBFUNC_EXIT
} /* ObtainSemaphoreShared */
