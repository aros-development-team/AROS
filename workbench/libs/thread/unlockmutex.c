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
        AROS_LH1(void, UnlockMutex,

/*  SYNOPSIS */
        AROS_LHA(Mutex, mutex, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 14, Thread)

/*  FUNCTION
        Unlocks a locked mutex.

    INPUTS
        mutex - mutex to unlock.

    RESULT
        This function always succeeds.

    NOTES

    EXAMPLE
        UnlockMutex(mutex);

    BUGS

    SEE ALSO
        CreateMutex(), DestroyMutex(), LockMutex(), TryLockMutex()

    INTERNALS
        Mutexes are implemented as thin wrappers around Exec semaphores.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    assert(mutex != NULL);

    ReleaseSemaphore((struct SignalSemaphore *) mutex);

    AROS_LIBFUNC_EXIT
} /* UnlockMutex */
