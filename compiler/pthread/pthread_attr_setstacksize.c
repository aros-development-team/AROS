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

/****************************************************************************
    NAME */
        int pthread_attr_setstacksize(

/*  SYNOPSIS */
        pthread_attr_t *attr,
        size_t stacksize)

/*  FUNCTION
        Set the stack size for subsequent worker creation and clear any
        previously supplied stack address. This delegates to
        pthread_attr_setstack(attr, NULL, stacksize).

    INPUTS
        attr - Initialized writable attribute object.
        stacksize - Requested size in bytes. Zero leaves process creation to
            use its default stack allocation.

    RESULT
        0 on success; EINVAL if attr is NULL. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. No memory is allocated, resized
        or freed. The change affects later pthread_create() calls only. Clearing
        a supplied stack pointer does not transfer or release that storage.
        Exclude concurrent access to attr.

    BUGS
        All size values are accepted without checking PTHREAD_STACK_MIN,
        alignment or whether the size can support the worker. Successful
        attribute assignment does not establish that creation will succeed
        or that the resulting stack is sufficient.

    SEE ALSO
        pthread_attr_init(),
        pthread_create()

*****************************************************************************/
{
    D(bug("%s(%p, %u)\n", __FUNCTION__, attr, stacksize));

    return pthread_attr_setstack(attr, NULL, stacksize);
}
