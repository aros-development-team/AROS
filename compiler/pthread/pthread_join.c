/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_join(

/*  SYNOPSIS */
        pthread_t thread,
        void **value_ptr)

/*  FUNCTION
        Wait for a joinable worker to finish, optionally copy its result pointer,
        and release its thread record for reuse. A worker that has already
        finished is collected without waiting for another completion signal.

    INPUTS
        thread - Valid joinable worker identifier. Must not identify the caller.
            Exactly one caller may join it; do not detach it concurrently.
        value_ptr - Writable pointer output, or NULL to discard the result.
            The result is the start routine's return value, the value passed
            to pthread_exit(), or PTHREAD_CANCELED after cancellation.

    RESULT
        0 after collection. ESRCH if the record is absent or no longer active.
        EINVAL if it is detached, or changes identity/state before collection.
        On failure, value_ptr is not written. Errors are returned directly.
        Pending enabled cancellation at the entry check exits the caller.

    NOTES
        Include <pthread.h>; link with pthread. Call from the initial program
        task or a registered worker, in task context. The wait has no timeout.
        The result is copied, not freed; arranging its lifetime and disposal
        remains the application's responsibility. Successful collection ends
        the identifier's lifetime. Worker destructors and runtime leave hooks
        precede the completion indication.

    BUGS
        Self-join is not detected and can block indefinitely. Multiple joiners
        overwrite the single waiter field and can strand earlier callers.
        Cancellation is checked only before entering the wait loop. The loop
        waits for SIGBREAKF_CTRL_F, not the CTRL_C cancellation signal, and does
        not recheck cancellation after waking. This call cannot be relied on
        as an interruptible wait. It also consumes that task signal.

    SEE ALSO
        pthread_create(),
        pthread_exit(),
        pthread_cancel()

*****************************************************************************/
{
    ThreadInfo *inf;
    struct Task *task;

    D(bug("%s(%u, %p)\n", __FUNCTION__, thread, value_ptr));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->parent == NULL)
        return ESRCH;

    if (inf->detached)
        return EINVAL;

    pthread_testcancel();

    // waiter, finished and detached are decided with the exiting thread
    // under thread_sem. A thread detached meanwhile clears its slot.
    ObtainSemaphore(&thread_sem);
    if (inf->parent == NULL)
    {
        ReleaseSemaphore(&thread_sem);
        return ESRCH;
    }
    task = inf->task;
    inf->waiter = GET_THIS_TASK;
    while (!inf->finished && !inf->detached && inf->task == task)
    {
        ReleaseSemaphore(&thread_sem);
        Wait(SIGF_PARENT);
        ObtainSemaphore(&thread_sem);
    }
    if (!inf->finished || inf->task != task)
    {
        if (inf->task == task)
            inf->waiter = NULL;
        ReleaseSemaphore(&thread_sem);
        return EINVAL;
    }

    if (value_ptr)
        *value_ptr = inf->ret;
    memset(inf, 0, sizeof(ThreadInfo));
    ReleaseSemaphore(&thread_sem);

    return 0;
}
