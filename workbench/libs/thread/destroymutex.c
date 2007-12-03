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
        AROS_LH1(BOOL, DestroyMutex,

/*  SYNOPSIS */
        AROS_LHA(Mutex, mutex, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 11, Thread)

/*  FUNCTION
        Destroys a mutex.

    INPUTS
        mutex - the mutex to destroy.

    RESULT
        TRUE if the mutex was destroyed, otherwise FALSE.

    NOTES
        You cannot destroy a mutex that is currently locked or has tasks
        waiting to lock it.

    EXAMPLE
        DestroyMutex(mutex)

    BUGS

    SEE ALSO
        CreateMutex(), LockMutex(), TryLockMutex(), UnlockMutex()

    INTERNALS
        Mutexes are implemented as thin wrappers around Exec semaphores.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    assert(mutex != NULL);

    /* we can only destroy the mutex if its not held and noone is waiting */
    Forbid();
    if (((struct SignalSemaphore *) mutex)->ss_QueueCount >= 0) {
        Permit();
        return FALSE;
    }
    Permit();

    FreeMem(mutex, sizeof(struct SignalSemaphore));

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* DestroyMutex */
