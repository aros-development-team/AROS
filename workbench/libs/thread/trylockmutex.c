/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/semaphores.h>
#include <proto/exec.h>
#include <assert.h>

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, TryLockMutex,

/*  SYNOPSIS */
        AROS_LHA(void *, mutex, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 13, Thread)

/*  FUNCTION
        Tries to lock a mutex. If the lock is already held, this function
        fails.

    INPUTS
        mutex - mutex to lock.

    RESULT
        TRUE if the lock was acquired, FALSE if the lock is already held.

    NOTES

    EXAMPLE
        TryLockMutex(mutex);

    BUGS

    SEE ALSO
        CreateMutex(), DestroyMutex(), LockMutex(), UnlockMutex()

    INTERNALS
        Mutexes are implemented as thin wrappers around Exec semaphores.
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    assert(mutex != NULL);

    return (BOOL) AttemptSemaphore((struct SignalSemaphore *) mutex);

    AROS_LIBFUNC_EXIT
} /* TryLockMutex */
