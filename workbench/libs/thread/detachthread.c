/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <proto/exec.h>
#include <assert.h>

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, DetachThread,

/*  SYNOPSIS */
        AROS_LHA(ThreadIdentifier, thread_id, D0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 8, Thread)

/*  FUNCTION
        Detaches a thread from the parent process.

    INPUTS
        thread_id - ID of thread to detach.

    RESULT
        TRUE if the thread was detached, FALSE if the thread was already
        detached or another error occured.

    NOTES
        You cannot detach a thread that is already detached.

        Once detached, the thread is no longer accessible from any other
        thread.

    EXAMPLE
        DetachThread(id);

    BUGS
        Currently this doesn't really do anything other than make it so you
        can't call WaitThread() on the thread. Threads can't truly be detached
        from the parent process since they run in the same address space, and
        so when the process exits the program code and all its other resources
        a freed.
        
        thread.library protects against this by waiting for all threads to
        complete (detached or not) before allowing the main process to exit.
        
        Detached threads can't be truly implemented until a thread task and its
        allocated resources can exist independently of the process that created
        it.

    SEE ALSO
        CreateThread(), CurrentThread(), WaitThread(), WaitAllThreads()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    assert(thread_id);

    /* get thread data */
    _Thread thread = _getthreadbyid(thread_id, ThreadBase);
    if (thread == NULL)
        return FALSE;

    /* mark it detached */
    ObtainSemaphore(&thread->lock);
    thread->detached = TRUE;
    ReleaseSemaphore(&thread->lock);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* DetachThread */
