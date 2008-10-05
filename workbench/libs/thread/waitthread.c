/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/thread.h>
#include <assert.h>

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, WaitThread,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, thread_id, D0),
        AROS_LHA(void **,  result,    A1),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 6, Thread)

/*  FUNCTION
        Blocks the current task until the requested thread exits.

    INPUTS
        thread_id - ID of thread to detach.
        result    - pointer to storage for the thread's return value. You can
                    pass NULL here if you don't care about the return value.

    RESULT
        TRUE when the thread completed successfully. FALSE if thread could not
        be waited on.

    NOTES
        A thread cannot wait on itself. A thread cannot be waited on if it is
        detached.

        If the thread has already completed, this call returns immediately with
        the thread result. Further calls to this function for that thread will
        fail.

        Multiple threads can wait for a thread to complete. They will all
        be notified when the thread completes, and will all receive the result.

    EXAMPLE
        void *ret;
        WaitThread(id, &ret);

    BUGS

    SEE ALSO
        CreateThread(), CurrentThread(), DetachThread()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct _Thread *thread;

    assert(thread_id);

    /* get the thread */
    if ((thread = _getthreadbyid(thread_id, ThreadBase)) == NULL)
        return FALSE;

    ObtainSemaphore(&thread->lock);

    /* can't wait for ourselves, that would be silly */
    if (thread->task == FindTask(NULL)) {
        ReleaseSemaphore(&thread->lock);
        return FALSE;
    }

    /* can't wait on detached threads */
    if (thread->detached) {
        ReleaseSemaphore(&thread->lock);
        return FALSE;
    }

    /* we only want to wait if the thread is still running */
    if (thread->task != NULL) {

        /* one more waiter */
        thread->exit_count++;
        ReleaseSemaphore(&thread->lock);

        /* wait for exit */
        LockMutex(thread->exit_mutex);
        WaitCondition(thread->exit, thread->exit_mutex);
        UnlockMutex(thread->exit_mutex);

        ObtainSemaphore(&thread->lock);

        /* no longer waiting */
        thread->exit_count--;
    }

    /* copy the result if the caller was interested */
    if (result != NULL)
        *result = thread->result;

    /* still more threads waiting, so we're done */
    if (thread->exit_count > 0) {
        ReleaseSemaphore(&thread->lock);
        return TRUE;
    }

    /* nobody else cares about this thread */

    /* remove it from the thread list */
    ObtainSemaphore(&ThreadBase->lock);
    REMOVE(thread);
    ReleaseSemaphore(&ThreadBase->lock);

    /* and clean it up */
    DestroyCondition(thread->exit);
    DestroyMutex(thread->exit_mutex);
    FreeVec(thread);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* WaitThread */
