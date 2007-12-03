/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/tasks.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <assert.h>

/*****************************************************************************

    NAME */
        AROS_LH1(void, SignalCondition,

/*  SYNOPSIS */
        AROS_LHA(void *, cond, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 18, Thread)

/*  FUNCTION
        Signals a thread waiting on condition variable.

    INPUTS
        cond - the condition to signal.

    RESULT
        This function always succeeds.

    NOTES
        Before calling this function you should lock the mutex that protects
        the condition. WaitCondition() atomically unlocks the mutex and waits
        on the condition, so by locking the mutex first before sending the
        signal, you ensure that the signal cannot be missed. See
        WaitCondition() for more details.

        If no threads are waiting on the condition, nothing happens. If more
        than one thread is waiting, only one will be signalled. Which one is
        undefined.

    EXAMPLE
        LockMutex(mutex);
        SignalCondition(cond);
        UnlockMutex(mutex);

    BUGS

    SEE ALSO
        CreateCondition(), DestroyCondition(), WaitCondition(),
        BroadcastCondition()

    INTERNALS
        SIGF_SIGNAL is used to signal the selected waiting thread.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct _Condition *c = (struct _Condition *) cond;
    struct _CondWaiter *waiter;

    assert(c != NULL);

    /* safely remove a waiter from the list */
    ObtainSemaphore(&c->lock);
    waiter = (struct _CondWaiter *) REMHEAD(&c->waiters);
    if (waiter != NULL)
        c->count--;
    ReleaseSemaphore(&c->lock);

    /* noone waiting */
    if (waiter == NULL)
        return;

    /* signal the task */
    Signal(waiter->task, SIGF_SINGLE);

    /* all done */
    FreeMem(waiter, sizeof(struct _CondWaiter));

    AROS_LIBFUNC_EXIT
} /* SignalCondition */
