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

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_cancel(

/*  SYNOPSIS */
        pthread_t thread)

/*  FUNCTION
        Mark a live worker as having a pending cancellation request and signal
        it with SIGBREAKF_CTRL_C. Cancellation takes effect when the worker
        processes the request with cancellation enabled. Acceptance does not
        wait for the worker to finish.

    INPUTS
        thread - Identifier of a live worker created by pthread_create().
            Coordinate its lifetime with joining, detaching and other requests.

    RESULT
        0 if the request was recorded. ESRCH if no allocated record is found,
        the record has no parent, or cancellation is already pending.
        An enabled asynchronous request for the calling worker exits through
        pthread_testcancel() and does not return. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. A new worker uses enabled,
        deferred cancellation. Disabled cancellation remains pending. Use
        pthread_join() on a joinable worker to observe completion and obtain
        PTHREAD_CANCELED; do not free worker-owned resources on request success.
        Cleanup handlers run when cancellation reaches pthread_exit().

    BUGS
        The default build does not enable USE_ASYNC_CANCEL. Selecting
        PTHREAD_CANCEL_ASYNCHRONOUS does not make a signal forcibly terminate
        another worker. Deferred cancellation requires a reached cancellation
        check; pthread_join() does not repeat that check while blocked.

        Record access is not serialized here, and finished joinable workers
        are not rejected by a finished-state check. Passing such an identifier
        can signal a stale task pointer. Repeated requests return ESRCH even
        while the target still exists. Do not target the initial program task:
        it has no worker exit jump point.

    SEE ALSO
        pthread_setcancelstate(),
        pthread_setcanceltype(),
        pthread_join(),
        pthread_exit()

*****************************************************************************/
{
    ThreadInfo *inf;

    D(bug("%s(%u)\n", __FUNCTION__, thread));

    inf = GetThreadInfo(thread);

    if (inf == NULL || inf->parent == NULL || inf->canceled == TRUE)
        return ESRCH;

    inf->canceled = TRUE;

    // we might have to cancel the thread immediately
    if (inf->canceltype == PTHREAD_CANCEL_ASYNCHRONOUS && inf->cancelstate == PTHREAD_CANCEL_ENABLE)
    {
        struct Task *task;

        task = GET_THIS_TASK;

        if (inf->task == task)
            pthread_testcancel(); // cancel ourselves
        else
            Signal(inf->task, SIGBREAKF_CTRL_C); // trigger the exception handler 
    }
    else
    {
        // for the timed waits
        Signal(inf->task, SIGBREAKF_CTRL_C);
    }

    return 0;
}
