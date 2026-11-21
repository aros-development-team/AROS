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
        A task that this pthread runtime did not create gets an identifier of
        its own, not below PTHREAD_THREADS_MAX: the same value on every call
        and different from every other task's. There is no separate failure
        result; if that identifier cannot be stored, 0 is returned.

    NOTES
        Include <pthread.h>; link with pthread. The call does not wait for a
        worker to finish and transfers no ownership. Compare identifiers with
        pthread_equal(). A worker identifier is valid only for that worker's
        lifetime, including any joinable completion record.

        The identifier of a task this runtime did not create only tells
        callers apart. The task has no entry in the thread table, so functions
        that take a pthread_t treat it like any identifier outside the table
        and fail.

    BUGS
        Cancellation state and cleanup handlers of a task this runtime did
        not create are still those of the initial task.

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
        return GetForeignThreadId();

    return thread;
}
