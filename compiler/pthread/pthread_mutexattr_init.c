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
        int pthread_mutexattr_init(

/*  SYNOPSIS */
        pthread_mutexattr_t *attr)

/*  FUNCTION
        Set a mutex attribute object's kind to PTHREAD_MUTEX_DEFAULT, which is
        PTHREAD_MUTEX_NORMAL in this implementation.

    INPUTS
        attr - Non-NULL writable mutex attribute object.

    RESULT
        0 on success; EINVAL if attr is NULL. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. No mutex or other resource is
        allocated, and the call does not block. pthread_mutex_init() copies the
        kind; the attribute object need not outlive the initialized mutex.
        Exclude concurrent access to attr.

    BUGS
        Only kind is initialized. The process-sharing field is left untouched,
        and pthread_mutex_init() ignores that field. This call does not provide
        process-shared synchronization.

    SEE ALSO
        pthread_mutexattr_settype(),
        pthread_mutexattr_destroy(),
        pthread_mutex_init()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    attr->kind = PTHREAD_MUTEX_DEFAULT;

    return 0;
}
