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
        int pthread_mutex_lock(

/*  SYNOPSIS */
        pthread_mutex_t *mutex)

/*  FUNCTION
        Acquire a mutex, waiting for its current owner to release it if needed.
        A recursive mutex increments its nesting count when acquired by its
        owner. Initialize a static mutex lazily if its semaphore is uninitialized.

    INPUTS
        mutex - Initialized mutex, or a supported static initializer. Its
            storage must remain valid throughout all acquisition and use.

    RESULT
        0 with the mutex acquired. EINVAL if mutex is NULL. EDEADLK if the
        calling task already owns an ERRORCHECK mutex. Errors are returned
        directly; errno is not the result channel.

    NOTES
        Include <pthread.h>; link with pthread. Call from task context where
        waiting is permitted. There is no timeout or cancellation check.
        Pair every successful acquisition with pthread_mutex_unlock().
        Explicitly initialize before concurrent use; the lazy path is not
        protected against simultaneous first use.

    BUGS
        NORMAL uses the recursive Exec semaphore path without rejecting an
        owner reacquisition. It therefore permits recursion here, although
        pthread_mutex_trylock() rejects the same case with EBUSY. Use the
        RECURSIVE kind when nested ownership is required.

    SEE ALSO
        pthread_mutex_init(),
        pthread_mutex_trylock(),
        pthread_mutex_unlock()

*****************************************************************************/
{
    struct SignalSemaphore *sem;

    D(bug("%s(%p)\n", __FUNCTION__, mutex));

    if (mutex == NULL)
        return EINVAL;

    sem = &mutex->semaphore;

    // initialize static mutexes
    if (SemaphoreIsInvalid(sem))
        _pthread_mutex_init(mutex, NULL, TRUE);

    // normal mutexes would simply deadlock here
    if (mutex->kind == PTHREAD_MUTEX_ERRORCHECK && SemaphoreIsMine(sem))
        return EDEADLK;

    ObtainSemaphore(sem);

#if 0 // let's just deadlock here, to be compatible with normal mutexes on other platforms
    if (mutex->kind == PTHREAD_MUTEX_NORMAL && sem->ss_NestCount > 1) {
        // should have blocked - fix this
        ReleaseSemaphore(sem);
        return EDEADLK;
    }
#endif

    return 0;
}
