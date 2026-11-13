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
        int pthread_cond_signal(

/*  SYNOPSIS */
        pthread_cond_t *cond)

/*  FUNCTION
        Signal the first registered waiter on cond. If no task is waiting, the
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
        Signalling does not remove or reserve the selected waiter. Repeated
        calls before that waiter removes itself can target it again, coalescing
        signals and leaving other waiters asleep. Do not assume successive
        calls wake distinct waiters.

    SEE ALSO
        pthread_cond_wait(),
        pthread_cond_timedwait(),
        pthread_cond_broadcast()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_broadcast(cond, TRUE);
}
