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

#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_mutexattr_destroy(

/*  SYNOPSIS */
        pthread_mutexattr_t *attr)

/*  FUNCTION
        Clear a mutex attribute object after its last use.

    INPUTS
        attr - Non-NULL writable mutex attribute object that is no longer in use.

    RESULT
        0 on success; EINVAL if attr is NULL. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. The call zeroes the object; it
        allocates and releases no resources and does not block. Existing
        mutexes retain their copied kind. Exclude concurrent access and call
        pthread_mutexattr_init() before using the object again.

    SEE ALSO
        pthread_mutexattr_init(),
        pthread_mutex_init()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, attr));

    if (attr == NULL)
        return EINVAL;

    memset(attr, 0, sizeof(pthread_mutexattr_t));

    return 0;
}
