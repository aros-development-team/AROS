/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Implementation of simpler mutexes
*/

#define AROS_ALMOST_COMPATIBLE
#include <aros/system.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "aros_intern.h"

/* I don't have CAS yet, so I need to fake it for now. */
unsigned long __CompareAndSwap(unsigned long *ptr, unsigned long val, struct ExecBase *sysBase);
#define CompareAndSwap(ptr,val)	__CompareAndSwap(ptr,val,SysBase)

/**************************************************************************

    NAME */
#include <aros/mutex.h>
#include <proto/aros.h>
	AROS_LH1(void, MutexInit,

/*  SYNOPSIS */
	AROS_LHA(struct Mutex *, mutex, A0),

/*  LOCATION */
	struct ArosBase *, ArosBase, 6, Aros)

/*  FUNCTION
	Initialise a mutex.

    INPUTS
	mutex	-   The mutex to initialise.

    RESULT
	The mutex will be initialised.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

**************************************************************************/
{
    AROS_LIBFUNC_INIT

    KASSERT(mutex != NULL);

    /* Hmm, this is hard */
    NEWLIST(&mutex->m_Waiters);
    mutex->m_Locker = NULL;

    AROS_LIBFUNC_EXIT
}

/**************************************************************************

    NAME */
#include <aros/mutex.h>
#include <proto/aros.h>
	AROS_LH2(void, MutexObtain,

/*  SYNOPSIS */
	AROS_LHA(struct Mutex *, mutex, A0),
	AROS_LHA(ULONG, timeout, D0),

/*  LOCATION */
	struct ArosBase *, ArosBase, 7, Aros)

/*  FUNCTION
	Obtain an exclusive lock on a Mutex. If the mutex is already
	locked, then this function will block until it is released.

	If timeout is non-zero, then the function will only wait for the
	given number of microseconds. This feature is not implemented.
	
	This kind of mutex is non-recursive. This means that if you attempt
	to obtain it whilst you already have it, the system will deadlock.
	If you are looking for a recursive lock, try SignalSemaphores from
	the exec.library.
	
    INPUTS
	    mutex   -	The mutex to lock.
	    timeout -	How long in microseconds to wait (not implemented).
	    
    RESULT
	When this function returns you will own the mutex.
	
    NOTES
	Attempting to lock this mutex when you already have it is a very
	bad idea.

	Currently the timeout is not supported.

	This function uses SIGF_SINGLE.
	
    EXAMPLE

    BUGS
	
    SEE ALSO

**************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *thisTask = FindTask(NULL);
    struct Task *ownTask;

    KASSERT(mutex != NULL);
    /*
     *	This function uses the kernel.resource's non-existent
     *	CompareAndSwap() function.
     */
    while((ownTask = (struct Task *)CompareAndSwap(&mutex->m_Locker, thisTask)) != NULL)
    {
	/*
	 *  If ownTask == NULL, then we own this mutex. We must check to
	 *  see that if are woken up without being granted ownership.
	 */
	if(ownTask != thisTask)
	{
	    struct MutexWaiter waiter;
	    
	    /*
	     *	This mutex is locked by somebody else. Add a MutexWaiter
	     *	node to the list.
	     */
	    waiter.m_Waiter = thisTask;
	    waiter.m_Condition = 0;
	    waiter.m_IsCondition = FALSE;
	    waiter.m_MilliToWait = timeout;

	    /*
	     *	Alas, I need to Forbid(), otherwise I can't guarantee that
	     *	the list will be valid. It would be nice to do without
	     *	this, but alas that is not possible. Eventually we would
	     *	move to a simplelock_t for mutexes only.
	     */
	    Forbid();
	    AddTail((struct List *)&mutex->m_Waiters, (struct Node *)&waiter);

	    /*
	     *	Now switch to some other task - I'm glad I don't need to
	     *	use Switch(). Could be interesting with interrupt threads.
	     */
	    thisTask->tc_SigRecvd &= SIGF_SINGLE;
	    Wait(SIGF_SINGLE);

	    /* We have woken up, remove our node from the list. */
	    Remove((struct Node *)&waiter);
	    Permit();
	}
	else
	    break;
    }

    AROS_LIBFUNC_EXIT
}

/**************************************************************************

    NAME */
#include <aros/mutex.h>
#include <proto/aros.h>
	AROS_LH1(void, MutexRelease,

/*  SYNOPSIS */
	AROS_LHA(struct Mutex *, mutex, A0),

/*  LOCATION */
	struct ArosBase *, ArosBase, 8, Aros)

/*  FUNCTION
	Release mutex that you had previous obtained. Attempting to release
	a mutex that you do not own is an error.

	This function will wake up the next task that is waiting for this
	mutex.
	
    INPUTS
	mutex	-   A mutex that you own.

    RESULT
	The mutex will be released to somebody else. You should not access
	the data which it was protecting.

    NOTES
	If you don't own this mutex, you will likely cause an Alert().

    EXAMPLE

    BUGS

    SEE ALSO

**************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *thisTask = FindTask(NULL), *nextTask = NULL;
    struct MutexWaiter *mw;

    KASSERT(mutex != NULL);
    KASSERT(mutex->m_Locker == thisTask);

    /*
     *	Now, go through the list looking for the first node which wishes to
     *	own this mutex without a condition.
     *
     *  I need to protect the mutex list. Unfortunately at the moment I need
     *  to Forbid().
     */
    Forbid();
    
    ForeachNode(&mutex->m_Waiters, mw)
    {
	if(mw->m_IsCondition == FALSE)
	{
	    /*
	     *	This node wants the mutex. Here's what I do:
	     *	1. Assign the mutex to the owner.
	     *	2. Signal() them to cause them to wake up.
	     *
	     *	Thats all... although apparently this can give rise to a
	     *	problem known as convoys. (See Sect 7.5.5 of "UNIX
	     *	Internals The New Frontiers", Uresh Valhalia, Prentice
	     *	Hall, New Jersey 1996 for more information.
	     */
	    nextTask = mw->m_Waiter;
	    break;
	}
    }

    if(nextTask != NULL)
    {
	/*
	 *  To prevent a convoy, I would not assign the mutex to nextTask
	 *  here, since there may be another task that comes along in
	 *  between that attempts to gain the mutex. This however breaks
	 *  the FIFO nature. I think the two problems are mutually
	 *  exclusive.
	 */
	mutex->m_Locker = nextTask;
	Signal(nextTask, SIGF_SINGLE);
    }
    else
	mutex->m_Locker = NULL;

    Permit();
	    
    AROS_LIBFUNC_EXIT
}

/**************************************************************************

    NAME */
#include <aros/mutex.h>
#include <proto/aros.h>
	AROS_LH3(void, MutexWait,

/*  SYNOPSIS */
	AROS_LHA(struct Mutex *, mutex, A0),
	AROS_LHA(unsigned long, cond, D0),
	AROS_LHA(ULONG, timeout, D1),
	
/*  LOCATION */
	struct ArosBase *, ArosBase, 9, Aros)

/*  FUNCTION
	Wait for a condition to happen that is associated with this mutex.
	Note that the condition is entirely arbitrary and its use should be
	arranged with the other users of the mutex. If you wait on a
	condition that nobody ever satisfies, you may never wake up.

	You can also specify a timeout in milliseconds to wait for the
	condition to occur. If the condition is not fulfilled within the
	time period you will be woken up. You should recheck whatever it
	was that you were waiting for.

    INPUTS
	mutex	-   The mutex associated with the condition.
	cond	-   A condition that you are waiting to be notified
	timeout	-   A timeout in microseconds to wait for the condition.

    RESULT
	Either - the condition has occurred and you now own the mutex, or
	the condition has not occurred (and you have timed out), and you DO
	NOT own the mutex.

    NOTES
	Take careful note: You may not own the mutex when you wake up. You
	can check mutex->m_Locker == FindTask(NULL) to determine whether
	you own the mutex or not.

    EXAMPLE

    BUGS

    SEE ALSO

**************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MutexWaiter waiter;
    struct Task *thisTask;

    KASSERT(mutex != NULL);
    
    /*
     *	We currently own this mutex. But we are going to give it up after
     *	we register that we are waiting for a condition.
     */
    waiter.m_Waiter = thisTask;
    waiter.m_Condition = cond;
    waiter.m_IsCondition = TRUE;
    waiter.m_MilliToWait = timeout;

    /*
     *	Alas, I need to Forbid(), otherwise I can't guarantee that
     *	the list will be valid. It would be nice to do without
     *	this, but alas that is not possible. Eventually we would
     *	move to a simplelock_t for mutexes only.
     */
    Forbid();
    AddTail((struct List *)&mutex->m_Waiters, (struct Node *)&waiter);

    /*	Give the mutex to somebody else. */
    MutexRelease(mutex);

    /*
     *	Now switch to some other task - I'm glad I don't need to
     *	use Switch(). Could be interesting with interrupt threads.
     */
    thisTask->tc_SigRecvd &= SIGF_SINGLE;
    Wait(SIGF_SINGLE);

    /* We have woken up, remove our node from the list. */
    Remove((struct Node *)&waiter);
    Permit();

    AROS_LIBFUNC_EXIT
}

/**************************************************************************

    NAME */
#include <proto/aros.h>
	AROS_LH3(void, MutexNotify,

/*  SYNOPSIS */
	AROS_LHA(struct Mutex *, mutex, A0),
	AROS_LHA(unsigned long, cond, D0),
	AROS_LHA(BOOL, wakeup_all, D1),

/*  LOCATION */
	struct ArosBase *, ArosBase, 10, Aros)

/*  FUNCTION
	Wakeup either the first or all tasks that are waiting on the
	condition specified.

    INPUTS
	mutex	-   The mutex associated with the condition.
	cond	-   The condition.
	
    RESULT
	A task will be woken up.
	
    NOTES
	A task isn't really woken until it regains the mutex. This may
	never happen.

    EXAMPLE

    BUGS

    SEE ALSO

**************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MutexWaiter *mw;

    KASSERT(mutex != NULL);

    /*
     *	Now, go through the list looking for the first node which wishes to
     *	be woken up by this condition.
     *
     *  I need to protect the mutex list. Unfortunately at the moment I need
     *  to Forbid().
     */
    Forbid();
    
    ForeachNode(&mutex->m_Waiters, mw)
    {
	if(mw->m_IsCondition == TRUE && mw->m_Condition == cond)
	{
	    /*
	     *	This node wants the mutex when it wakes up. What I really
	     *	do is convert this condition wait into a normal wait. When
	     *	the mutex is freed, depending upon how long the task waited
	     *	for, it may or may not get the mutex next.
	     *
	     *	Case for discussion: Should this cancel the timeout?
	     */
	    mw->m_IsCondition = FALSE;
	    if(wakeup_all == FALSE)
		break;
	}
    }
    Permit();

    AROS_LIBFUNC_EXIT
}

#undef SysBase
unsigned long
__CompareAndSwap
(
    unsigned long *ptr,
    unsigned long val,
    struct ExecBase *SysBase
)
{
    unsigned long old;

    Disable();
    old = *ptr;
    if(old == 0)
	*ptr = val;

    Enable();
    return old;
}
