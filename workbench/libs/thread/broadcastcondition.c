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
        AROS_LH1(void, BroadcastCondition,

/*  SYNOPSIS */
        AROS_LHA(Condition, cond, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 19, Thread)

/*  FUNCTION
        Signals all threads waiting on a condition variable.

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

        If no threads are waiting on the condition, nothing happens.

    EXAMPLE
        LockMutex(mutex);
        BroadcastCondition(cond);
        UnlockMutex(mutex);

    BUGS

    SEE ALSO
        CreateCondition(), DestroyCondition(), WaitCondition(),
        SignalCondition()

    INTERNALS
        SIGF_SIGNAL is used to signal the selected waiting thread.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    _Condition c = (_Condition) cond;
    _CondWaiter waiter;

    assert(c != NULL);

    /* safely operation on the condition */
    ObtainSemaphore(&c->lock);

    /* wake up all the waiters */
    while ((waiter = (_CondWaiter) REMHEAD(&c->waiters)) != NULL) {
        Signal(waiter->task, SIGF_SINGLE);
        FreeMem(waiter, sizeof(struct _CondWaiter));
    }

    /* none left */
    c->count = 0;

    ReleaseSemaphore(&c->lock);

    AROS_LIBFUNC_EXIT
} /* BroadcastCondition */
