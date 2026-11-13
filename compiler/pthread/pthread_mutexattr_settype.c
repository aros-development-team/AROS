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
        int pthread_mutexattr_settype(

/*  SYNOPSIS */
        pthread_mutexattr_t *attr,
        int kind)

/*  FUNCTION
        Set the kind to be copied by subsequent pthread_mutex_init() calls.

    INPUTS
        attr - Initialized writable mutex attribute object.
        kind - PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_RECURSIVE or
            PTHREAD_MUTEX_ERRORCHECK. PTHREAD_MUTEX_DEFAULT aliases NORMAL.

    RESULT
        0 on success. EINVAL if attr is NULL or kind is outside the supported
        range; the object is unchanged on failure. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. This changes no existing mutex,
        allocates no resources and does not block. Exclude concurrent access
        to the attribute object. Recursive locks require a matching unlock for
        each acquisition. ERRORCHECK diagnoses a blocking lock by the owner
        with EDEADLK and an unlock by a non-owner with EPERM.

    BUGS
        NORMAL does not implement a strict nonrecursive lock: pthread_mutex_lock()
        permits repeated acquisition by its owner, while pthread_mutex_trylock()
        returns EBUSY for that case. Do not depend on owner recursion for NORMAL.

    SEE ALSO
        pthread_mutexattr_init(),
        pthread_mutex_init(),
        pthread_mutex_lock(),
        pthread_mutex_trylock()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL || !(kind >= PTHREAD_MUTEX_NORMAL && kind <= PTHREAD_MUTEX_ERRORCHECK))
        return EINVAL;

    attr->kind = kind;

    return 0;
}
