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

#include <stdlib.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        pthread_t pthread_self(

/*  SYNOPSIS */
        void)

/*  FUNCTION
        Return the identifier associated with the calling task in this pthread
        runtime's thread table.

    INPUTS
        None.

    RESULT
        The registered worker identifier, or 0 for the initial program task.
        This implementation also returns 0 for an unregistered task; there is
        no separate failure result.

    NOTES
        Include <pthread.h>; link with pthread. The call does not wait for a
        worker to finish and transfers no ownership. Compare identifiers with
        pthread_equal(). A worker identifier is valid only for that worker's
        lifetime, including any joinable completion record.

    BUGS
        Tasks created outside this pthread runtime are not registered by this
        call. Their fallback value aliases the initial task, so it must not be
        used to access cancellation state or thread-specific storage from an
        arbitrary Exec task.

    SEE ALSO
        pthread_equal(),
        pthread_create()

*****************************************************************************/
{
    struct Task *task;
    pthread_t thread;

    D(bug("%s()\n", __FUNCTION__));

    task = GET_THIS_TASK;

    thread = GetThreadId(task);

    if (thread == PTHREAD_THREADS_MAX)
        return 0;

    return thread;
}
