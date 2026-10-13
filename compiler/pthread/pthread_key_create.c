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
        int pthread_key_create(

/*  SYNOPSIS */
        pthread_key_t *key,
        void (*destructor)(void *))

/*  FUNCTION
        Reserve the lowest free thread-specific key and associate an optional
        destructor with it. Each registered thread has a separate pointer slot
        for the key. Allocation is limited to PTHREAD_KEYS_MAX (64) keys shared
        by this pthread runtime.

    INPUTS
        key - Non-NULL writable output for the allocated key.
        destructor - Callback for non-NULL values at worker exit, or NULL.
            Keep the callback code valid for every possible invocation.

    RESULT
        0 on success, with *key set. EINVAL if key is NULL; EAGAIN if every key
        is in use. The output is unchanged on failure. Errors are returned
        directly.

    NOTES
        Include <pthread.h>; link with pthread. Allocation can wait for the TLS
        semaphore. Creating a key does not allocate or take ownership of values.
        Set each thread's value with pthread_setspecific(). The application
        must eventually delete the key after its users have finished.

        At normal worker exit, a non-NULL value is cleared before its destructor
        is called in the exiting worker. A destructor may install another value;
        at most PTHREAD_DESTRUCTOR_ITERATIONS passes are made. Values without a
        destructor are not freed. Program-wide teardown skips destructors.

    BUGS
        Reusing a deleted key does not clear old per-thread values. A thread
        can read a stale pointer, and the new destructor can receive it. Avoid
        reuse while any surviving thread may retain a value for that slot.

        Destructors execute under the shared TLS semaphore. Key creation and
        deletion require exclusive acquisition; do not rely on those operations
        from a destructor. Initial-task exit does not use the worker destructor
        path.

    SEE ALSO
        pthread_setspecific(),
        pthread_getspecific(),
        pthread_exit()

*****************************************************************************/
{
    TLSKey *tls;
    int i;

    D(bug("%s(%p, %p)\n", __FUNCTION__, key, destructor));

    if (key == NULL)
        return EINVAL;

    ObtainSemaphore(&tls_sem);

    for (i = 0; i < PTHREAD_KEYS_MAX; i++)
    {
        if (tlskeys[i].used == FALSE)
            break;
    }

    if (i == PTHREAD_KEYS_MAX)
    {
        ReleaseSemaphore(&tls_sem);
        return EAGAIN;
    }

    tls = &tlskeys[i];
    tls->used = TRUE;
    tls->destructor = destructor;

    ReleaseSemaphore(&tls_sem);

    *key = i;

    return 0;
}
