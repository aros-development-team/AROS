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
        int pthread_mutex_trylock(

/*  SYNOPSIS */
        pthread_mutex_t *mutex)

/*  FUNCTION
        Attempt to acquire a mutex without waiting for another owner. A
        recursive mutex may be reacquired by its owner. Initialize a static
        mutex lazily if its semaphore is uninitialized.

    INPUTS
        mutex - Initialized mutex, or a supported static initializer. Keep its
            storage valid until it is unlocked and no longer in use.

    RESULT
        0 with one acquisition held. EBUSY if the semaphore cannot be acquired,
        or if the caller owns a nonrecursive mutex. EINVAL if mutex is NULL.
        Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Call from task context. This
        function does not enqueue a wait or check cancellation. A successful
        call must be paired with pthread_mutex_unlock(); failure grants no
        ownership. Explicit initialization before sharing avoids the
        unsynchronized lazy-initialization path.

    BUGS
        Owner reacquisition of an ERRORCHECK mutex returns EBUSY here, whereas
        pthread_mutex_lock() returns EDEADLK. Owner reacquisition of NORMAL is
        rejected here but allowed by the blocking lock implementation.

    SEE ALSO
        pthread_mutex_lock(),
        pthread_mutex_unlock(),
        pthread_mutex_init()

*****************************************************************************/
{
    ULONG ret;

    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // initialize static mutexes
    if (SemaphoreIsInvalid(&mutex->semaphore))
        _pthread_mutex_init(mutex, NULL, TRUE);

    if (mutex->kind != PTHREAD_MUTEX_RECURSIVE && SemaphoreIsMine(&mutex->semaphore))
        return EBUSY;

    ret = AttemptSemaphore(&mutex->semaphore);

    return (ret == TRUE) ? 0 : EBUSY;
}
