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
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
        AROS_LH0(Mutex, CreateMutex,

/*  SYNOPSIS */

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 10, Thread)

/*  FUNCTION
        Creates a mutual exclusion device (aka lock).

    INPUTS
        None.

    RESULT
        The newly created mutex, or NULL if a mutex couldn't be created.

    NOTES

    EXAMPLE
        Mutex mutex = CreateMutex();

    BUGS

    SEE ALSO
        DestroyMutex(), LockMutex(), TryLockMutex(), UnlockMutex()

    INTERNALS
        Mutexes are implemented as thin wrappers around Exec semaphores.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SignalSemaphore *sem;

    if ((sem = (struct SignalSemaphore *) AllocMem(sizeof(struct SignalSemaphore), MEMF_PUBLIC)) == NULL)
        return NULL;

    InitSemaphore(sem);

    return (Mutex) sem;

    AROS_LIBFUNC_EXIT
} /* CreateMutex */
