/*
  Copyright (C) 2014 Szilard Biro

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

int pthread_setspecific(pthread_key_t key, const void *value)
{
    pthread_t thread;
    ThreadInfo *inf;
    TLSKey *tls;

    D(bug("%s(%u)\n", __FUNCTION__, key));

    if (key >= PTHREAD_KEYS_MAX)
        return EINVAL;

    thread = pthread_self();
    tls = &tlskeys[key];

    ObtainSemaphoreShared(&tls_sem);

    if (tls->used == FALSE)
    {
        ReleaseSemaphore(&tls_sem);
        return EINVAL;
    }

    ReleaseSemaphore(&tls_sem);

    inf = GetThreadInfo(thread);
    inf->tlsvalues[key] = (void *)value;

    return 0;
}
