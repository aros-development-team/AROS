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
        int pthread_setcanceltype(

/*  SYNOPSIS */
        int type,
        int *oldtype)

/*  FUNCTION
        Set the calling thread's cancellation type, optionally saving the
        previous value. A newly created worker starts with PTHREAD_CANCEL_DEFERRED.

    INPUTS
        type - PTHREAD_CANCEL_DEFERRED or PTHREAD_CANCEL_ASYNCHRONOUS.
        oldtype - Writable output for the previous value, or NULL.

    RESULT
        0 on success. EINVAL for any other type; neither the setting nor the
        optional output is changed on that error. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. The call updates only the current
        thread record and does not wait. It neither clears a pending request
        nor tests it. Arrange cleanup before enabling cancellation and use
        pthread_testcancel() when an immediate deferred check is required.

    BUGS
        The default build does not enable the exception handler needed for
        remote asynchronous cancellation. Accepting the asynchronous constant
        does not establish that capability. Switching types does not process
        an already pending request. Use only from a registered worker; an
        unregistered task aliases the initial task's state.

    SEE ALSO
        pthread_cancel(),
        pthread_setcancelstate()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;

    D(bug("%s(%d, %p)\n", __FUNCTION__, type, oldtype));

    if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS)
        return EINVAL;

    thread = pthread_self();
    inf = GetThreadInfo(thread);

    if (oldtype)
        *oldtype = inf->canceltype;

    inf->canceltype = type;

    return 0;
}
