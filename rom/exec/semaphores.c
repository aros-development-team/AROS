/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Semaphore internal handling
    Lang: english
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_util.h"
#include "semaphores.h"

BOOL CheckSemaphore(struct SignalSemaphore *sigSem, struct TraceLocation *caller, struct ExecBase *SysBase)
{
    /* TODO: Introduce AlertContext for this */

    if (KernelBase && KrnIsSuper())
    {
        /* FindTask() is called only here, for speedup */
        struct Task *me = FindTask(NULL);

        kprintf("%s called in supervisor mode!!!\n"
                "sem = 0x%p task = 0x%p (%s)\n\n", caller->function, sigSem, me, me->tc_Node.ln_Name);
        Exec_ExtAlert(ACPU_PrivErr & ~AT_DeadEnd, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);

        return FALSE;
    }

    if ((sigSem->ss_Link.ln_Type != NT_SIGNALSEM) || (sigSem->ss_WaitQueue.mlh_Tail != NULL))
    {
        struct Task *me = FindTask(NULL);

        kprintf("%s called on a not initialized semaphore!!!\n"
                "sem = 0x%p task = 0x%p (%s)\n\n", caller->function, sigSem, me, me->tc_Node.ln_Name);
        Exec_ExtAlert(AN_SemCorrupt, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);

        return FALSE;
    }

    return TRUE;
}

void InternalObtainSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase)
{
    struct Task *me = FindTask(NULL);

    /*
     * If there's no ThisTask, the function is called from within memory
     * allocator in exec's pre-init code. We are already single-threaded,
     * just return. :)
     */
    if (!me)
        return;

    /*
     * Freeing memory during RemTask(NULL). We are already single-threaded by
     * Forbid(), and waiting isn't possible because task context is being deallocated.
     */
    if (me->tc_State == TS_REMOVED)
        return;

    if (!CheckSemaphore(sigSem, caller, SysBase))
        return;  /* A crude attempt to recover... */

    /*
     * Arbitrate for the semaphore structure.
     * TODO: SMP-aware versions of this code likely need to use spinlocks here
     */
    Forbid();

    /*
     * ss_QueueCount == -1 indicates that the semaphore is
     * free, so we increment this straight away. If it then
     * equals 0, then we are the first to allocate this semaphore.
     */
    sigSem->ss_QueueCount++;

    if (sigSem->ss_QueueCount == 0)
    {
        /* We now own the semaphore. This is quick. */
        sigSem->ss_Owner = owner;
        sigSem->ss_NestCount++;
    }
    /*
     * The semaphore is in use.
     * It could be either shared (ss_Owner == NULL) or it could already be exclusively owned
     * by me (ss_Owner == me).
     * Exclusive or shared mode of this function is determined by 'owner' parameter.
     * Actually it's pointer to a task which is allowed to share the lock with us.
     * If it's equal to 'me', we are locking the semaphore in exclusive more. If it's NULL,
     * we are locking in shared mode. This helps to optimize code against speed, and remove
     * extra comparisons.
     */
    else if ((sigSem->ss_Owner == me) || (sigSem->ss_Owner == owner))
    {
        /* Yes, just increase the nesting count */
        sigSem->ss_NestCount++;
    }
    /* Else, some other task owns it. We have to set a waiting request here. */
    else
    {
        /*
         * We need a node to mark our semaphore request. Lets use some
         * stack memory.
         */
        struct SemaphoreRequest sr;
        sr.sr_Waiter = me;

        if (owner == NULL)
            sr.sr_Waiter = (struct Task *)((IPTR)(sr.sr_Waiter) | SM_SHARED);

        /*
         * Have to clear the signal to make sure that we don't
         * return immediately. We then add the SemReq to the
         * waiters list of the semaphore. We were the last to
         * request, so we must be the last to get the semaphore.
        */

        /* This must be atomic! */
        AROS_ATOMIC_AND(me->tc_SigRecvd, ~SIGF_SINGLE);

        AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);

        /*
         * Finally, we simply wait, ReleaseSemaphore() will fill in
         * who owns the semaphore.
         */
        Wait(SIGF_SINGLE);
    }

    /* All Done! */
    Permit();
}

ULONG InternalAttemptSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase)
{
    struct Task *me = FindTask(NULL);
    ULONG retval = TRUE;

    if (!CheckSemaphore(sigSem, caller, SysBase))
        return FALSE;  /* A crude attempt to recover... */

    /*
     * Arbitrate for the semaphore structure.
     * TODO: SMP-aware versions of this code likely need to use spinlocks here
     */
    Forbid();

    /* Increment the queue count */
    sigSem->ss_QueueCount++;

    if (sigSem->ss_QueueCount == 0)
    {
        /* The semaphore wasn't owned. We can now own it */
        sigSem->ss_Owner = owner;
        sigSem->ss_NestCount++;
    }
    else if ((sigSem->ss_Owner == me) || (sigSem->ss_Owner == owner))
    {
        /* The semaphore was owned by me or is shared, just increase the nest count */
        sigSem->ss_NestCount++;
    }
    else
    {
        /* We can't get ownership, just return it. */
        sigSem->ss_QueueCount--;
        retval = FALSE;
    }

    /* All done. */
    Permit();

    return retval;
}
