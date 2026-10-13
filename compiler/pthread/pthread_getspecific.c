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
        void *pthread_getspecific(

/*  SYNOPSIS */
        pthread_key_t key)

/*  FUNCTION
        Read the calling thread's pointer slot for key. The pointer is returned
        unchanged; the call neither copies nor frees the pointed-to data.

    INPUTS
        key - Live key allocated by pthread_key_create(), with its lifetime
            protected against concurrent deletion and reuse.

    RESULT
        The stored pointer, or NULL if the slot is NULL or key is outside
        0..PTHREAD_KEYS_MAX-1. There is no separate error result.

    NOTES
        Include <pthread.h>; link with pthread. Call from the initial program
        task or a registered worker. This is a nonblocking slot read. A new
        worker's slots start as NULL. Values belong to the application and
        remain valid only for the lifetime established by that application.

    BUGS
        The call does not check whether an in-range key is allocated. Deleting
        or recreating a key does not clear old slot contents, so it can return
        a stale pointer. An unregistered task maps to the initial task's record
        instead of receiving independent thread-specific storage.

    SEE ALSO
        pthread_key_create(),
        pthread_setspecific(),
        pthread_self()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;
    void *value = NULL;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return NULL;

    thread = pthread_self();
    inf = GetThreadInfo(thread);
    value = inf->tlsvalues[key];

    return value;
}
