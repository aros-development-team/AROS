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
        int pthread_cond_init(

/*  SYNOPSIS */
        pthread_cond_t *cond,
        const pthread_condattr_t *attr)

/*  FUNCTION
        Initialize a condition variable with an empty waiter list. Select the
        absolute-deadline clock from attr, or CLOCK_REALTIME by default.

    INPUTS
        cond - Non-NULL writable condition storage, not already in use.
        attr - Attributes initialized with pthread_condattr_init(), or NULL.
            On AROS, CLOCK_REALTIME and CLOCK_MONOTONIC are accepted.

    RESULT
        0 on success. EINVAL if cond is NULL or attr specifies an unsupported
        clock on AROS. Validation failure leaves cond unchanged.
        Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. No waiter or timer is allocated.
        The caller retains the storage until no task can wait or signal and
        pthread_cond_destroy() has completed. Attributes are copied and need
        not outlive the condition. Initialize before publishing to other tasks.
        Use pthread_condattr_init(), not manual zeroing: clock identifier 0
        denotes CLOCK_MONOTONIC on AROS, not the realtime default.

    BUGS
        Process-sharing attributes are ignored. Reinitialization of an active
        condition is not detected. Static lazy initialization elsewhere in
        this implementation is not serialized against simultaneous first use.

    SEE ALSO
        pthread_cond_wait(),
        pthread_cond_timedwait(),
        pthread_cond_destroy()

*****************************************************************************/
{
    D(bug("%s(%p, %p)\n", __FUNCTION__, cond, attr));

    if (cond == NULL)
        return EINVAL;

#ifdef CLOCK_REALTIME
    if (attr != NULL &&
        attr->cond_clock != CLOCK_REALTIME && attr->cond_clock != CLOCK_MONOTONIC)
        return EINVAL;
#endif

    InitSemaphore(&cond->semaphore);
    NEWLIST((struct List *)&cond->waiters);
#ifdef CLOCK_REALTIME
    /* The clock every absolute pthread_cond_timedwait() deadline on this
     * condvar is read against. NULL attr (and the static initializer, and
     * the timedwait lazy-init path) means the POSIX realtime default. */
    cond->cond_clock = (attr != NULL) ? attr->cond_clock : CLOCK_REALTIME;
#else
    cond->cond_clock = (attr != NULL) ? attr->cond_clock : 0;
#endif

    return 0;
}
