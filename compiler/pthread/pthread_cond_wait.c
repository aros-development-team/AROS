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
        int pthread_cond_wait(

/*  SYNOPSIS */
        pthread_cond_t *cond,
        pthread_mutex_t *mutex)

/*  FUNCTION
        Register the caller as a waiter, release one acquisition of mutex,
        wait for a condition or CTRL_C signal, and reacquire mutex before
        removing the waiter. A return does not establish that the application's
        predicate is true; always test it again while holding mutex.

    INPUTS
        cond - Initialized condition variable, or a supported static initializer.
        mutex - Initialized mutex owned exactly once by the calling task.
            Use the same mutex for all concurrent waits on cond and for the
            protected predicate. Retain both objects until all users finish.

    RESULT
        0 after waking and reacquiring a valid mutex. EINVAL if cond or mutex
        is NULL. Errors are returned directly. Pending enabled cancellation
        observed after a CTRL_C wake exits the worker instead of returning.

    NOTES
        Include <pthread.h>; link with pthread. Call from the initial program
        task or a registered worker. There is no timeout. The mutex is reacquired
        before cancellation cleanup; arrange for a cleanup handler to release it.
        Other CTRL_C wakes can produce a zero result without a condition signal.
        This implementation does not return EINTR for those wakes.
        Initialize the objects before concurrent use.

    EXAMPLE
        // mutex is locked; ready is protected by mutex.
        int error = 0;
        while (!ready) {
            error = pthread_cond_wait(&cond, &mutex);
            if (error != 0)
                break;
        }
        // Examine ready or handle the error before using protected data.

    BUGS
        Mutex ownership and matching are not validated, and lock/unlock errors
        are ignored. Waiting while holding multiple recursive acquisitions
        does not fully release the mutex and can deadlock other participants.
        Signal allocation failure uses the task's CTRL_E bit; the fallback
        does not clear an already pending bit and can consume unrelated use
        of that signal. Unregistered tasks cannot safely process cancellation.

    SEE ALSO
        pthread_cond_timedwait(),
        pthread_cond_signal(),
        pthread_cond_broadcast()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    return _pthread_cond_timedwait(cond, mutex, NULL, FALSE);
}
