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
        int pthread_setspecific(

/*  SYNOPSIS */
        pthread_key_t key,
        const void *value)

/*  FUNCTION
        Store value in the calling thread's slot for an allocated key. Replacing
        a value does not call its destructor or free the previous value.

    INPUTS
        key - Live key allocated by pthread_key_create(). Prevent its deletion
            or reuse during this operation and while its values remain in use.
        value - Borrowed pointer to store, or NULL to clear the slot. The caller
            owns the data and must arrange its lifetime and eventual release.

    RESULT
        0 after storing the pointer. EINVAL if key is out of range or not
        allocated when checked, ENOMEM if a task that is not a pthread could
        not get slots; the slot is unchanged on failure. Errors are returned
        directly.

    NOTES
        Include <pthread.h>; link with pthread. The allocation check can wait
        for the shared TLS semaphore. A task that is not a pthread (another
        program calling into a shared library, a raw subtask) gets slots of
        its own on its first store, and its destructors never run. No data
        storage is allocated or copied. At worker
        exit, a non-NULL value with an associated destructor is cleared and
        passed to that destructor, subject to the rules of pthread_key_create().
        Clear or dispose of a replaced value explicitly when necessary.

    BUGS
        The TLS semaphore is released before the slot is written; concurrent
        key deletion/reuse is not excluded. Recreated keys retain stale values
        in surviving threads. Program-wide teardown does not guarantee
        execution of worker TLS destructors.

    SEE ALSO
        pthread_key_create(),
        pthread_getspecific(),
        pthread_exit()

*****************************************************************************/
{
    void **values;
    TLSKey *tls;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return EINVAL;

    tls = &tlskeys[key];

    ObtainSemaphoreShared(&tls_sem);

    if (tls->used == FALSE)
    {
        ReleaseSemaphore(&tls_sem);
        return EINVAL;
    }

    ReleaseSemaphore(&tls_sem);

    if (!(values = GetTLSValues(TRUE)))
        return ENOMEM;
    values[key] = (void *)value;

    return 0;
}
