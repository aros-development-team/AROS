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
        int pthread_mutex_unlock(

/*  SYNOPSIS */
        pthread_mutex_t *mutex)

/*  FUNCTION
        Release one acquisition of a mutex owned by the calling task. A recursive
        mutex remains owned until every nested acquisition has been released.

    INPUTS
        mutex - Mutex owned by the caller. No other task may destroy it during
            this operation.

    RESULT
        0 after ReleaseSemaphore(). EINVAL if mutex is NULL. EPERM if a
        non-NORMAL mutex is not owned by the caller. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Use from task context. The call
        does not wait for ownership or check cancellation. Releasing the last
        acquisition allows another waiting task to proceed. It does not free
        the mutex storage.

    BUGS
        NORMAL performs no ownership check before ReleaseSemaphore(). Passing
        an unowned or already unlocked mutex is invalid and may corrupt its
        state. A successful return is not independent proof of prior ownership.
        An uninitialized semaphore also takes a lazy-initialization path; that
        path does not make unlocking without an acquisition valid.

    SEE ALSO
        pthread_mutex_lock(),
        pthread_mutex_trylock(),
        pthread_mutex_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    // initialize static mutexes
    if (SemaphoreIsInvalid(&mutex->semaphore))
        _pthread_mutex_init(mutex, NULL, TRUE);

    if (mutex->kind != PTHREAD_MUTEX_NORMAL && !SemaphoreIsMine(&mutex->semaphore))
        return EPERM;

    ReleaseSemaphore(&mutex->semaphore);

    return 0;
}
