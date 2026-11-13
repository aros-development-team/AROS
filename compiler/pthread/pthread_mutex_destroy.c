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
        int pthread_mutex_destroy(

/*  SYNOPSIS */
        pthread_mutex_t *mutex)

/*  FUNCTION
        Clear an unused mutex after checking that its semaphore can be acquired
        and no condition wait is recorded. No caller-owned storage is freed.

    INPUTS
        mutex - Unlocked mutex with no users or waiters. Prevent new operations
            for the entire destruction and storage-release interval.

    RESULT
        0 on success, including an uninitialized static semaphore (left as is).
        EINVAL if mutex is NULL. EBUSY if AttemptSemaphore() fails or the
        condition-wait count is nonzero. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. The ownership probe does not
        wait. An initialized mutex is zeroed after the probe is released.
        Reinitialize before reuse. EBUSY is a diagnostic, not a substitute for
        establishing that all potential users have finished.

    BUGS
        Exec permits recursive acquisition by the owner, so the probe does not
        reject a mutex still held by the caller. It can report success and clear
        that live lock. Concurrent acquisition after the probe is also not
        excluded by the implementation. Destroy only after external quiescence.

    SEE ALSO
        pthread_mutex_init(),
        pthread_mutex_unlock(),
        pthread_cond_wait()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // probably a statically allocated mutex
    if (SemaphoreIsInvalid(&mutex->semaphore))
        return 0;

    if (/*mutex->incond ||*/ AttemptSemaphore(&mutex->semaphore) == FALSE)
        return EBUSY;

    if (mutex->incond)
    {
        ReleaseSemaphore(&mutex->semaphore);
        return EBUSY;
    }

    ReleaseSemaphore(&mutex->semaphore);
    memset(mutex, 0, sizeof(pthread_mutex_t));

    return 0;
}
