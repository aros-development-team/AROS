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

int _pthread_cond_broadcast(pthread_cond_t *cond, BOOL onlyfirst)
{
    CondWaiter *waiter;

    DB2(bug("%s(%p, %d)\n", __FUNCTION__, cond, onlyfirst));

    if (cond == NULL)
        return EINVAL;

    // initialize static conditions
    if (SemaphoreIsInvalid(&cond->semaphore))
        pthread_cond_init(cond, NULL);

    // signal the waiting threads 
    ObtainSemaphore(&cond->semaphore);
    ForeachNode(&cond->waiters, waiter)
    {
        Signal(waiter->task, 1 << waiter->sigbit);
        if (onlyfirst) break;
    }
    ReleaseSemaphore(&cond->semaphore);

    return 0;
}

/****************************************************************************
    NAME */
        int pthread_cond_broadcast(

/*  SYNOPSIS */
        pthread_cond_t *cond)

/*  FUNCTION
        Signal every registered waiter on cond. If no task is waiting, the
        call has no retained effect. It does not change the application's
        predicate or transfer ownership of the associated mutex.

    INPUTS
        cond - Initialized condition variable, or a supported static initializer.
            Its storage must remain valid throughout signalling and all waits.

    RESULT
        0 after signalling, including when no waiter is present. EINVAL if cond
        is NULL. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Call from task context; acquiring
        the internal condition semaphore can block. This is not an interrupt
        or asynchronous signal-handler interface. Protect the predicate with
        the same mutex used by waiters and recheck it in a loop after waking.
        A woken task must reacquire that mutex before returning from its wait.
        Explicit initialization before sharing avoids the lazy-init race.

    BUGS
        Signals are task bits, not queued notifications. Repeated broadcasts
        may coalesce. A waiter remains listed until it has reacquired its
        mutex, and no ordering or fairness guarantee is provided.

    SEE ALSO
        pthread_cond_wait(),
        pthread_cond_timedwait(),
        pthread_cond_signal()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_broadcast(cond, FALSE);
}
