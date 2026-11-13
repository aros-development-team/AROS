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
        int pthread_cond_destroy(

/*  SYNOPSIS */
        pthread_cond_t *cond)

/*  FUNCTION
        Clear a condition variable after checking that its internal semaphore
        is available and its waiter list is empty.

    INPUTS
        cond - Unused condition variable. Prevent all new waits and signals
            before destruction and until its storage has been released.

    RESULT
        0 on success, including an uninitialized static condition (left as is).
        EINVAL if cond is NULL. EBUSY if its semaphore cannot be acquired
        immediately or its waiter list is nonempty. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. The probe does not wait. No
        caller storage is freed. Reinitialize before reuse. A waiter remains
        on the internal list until it has reacquired its application mutex,
        so a signalled waiter can still make this call report EBUSY.

    BUGS
        The implementation releases its semaphore before zeroing the object.
        The empty-list probe cannot make concurrent destruction safe. Establish
        that all potential users have finished outside this call.

    SEE ALSO
        pthread_cond_init(),
        pthread_cond_wait(),
        pthread_cond_signal()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    if (cond == NULL)
        return EINVAL;

    // probably a statically allocated condition
    if (SemaphoreIsInvalid(&cond->semaphore))
        return 0;

    if (AttemptSemaphore(&cond->semaphore) == FALSE)
        return EBUSY;

#ifdef __AROS__
    if (!IsMinListEmpty(&cond->waiters))
#else
    if (!IsListEmpty((struct List *)&cond->waiters))
#endif
    {
        ReleaseSemaphore(&cond->semaphore);
        return EBUSY;
    }

    ReleaseSemaphore(&cond->semaphore);
    memset(cond, 0, sizeof(pthread_cond_t));

    return 0;
}
