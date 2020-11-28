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

#include <stdlib.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

pthread_t pthread_self(void)
{
    struct Task *task;
    pthread_t thread;

    D(bug("%s()\n", __FUNCTION__));

    task = FindTask(NULL);
    thread = GetThreadId(task);

    // add non-pthread processes to our list, so we can handle the main thread
    if (thread == PTHREAD_THREADS_MAX)
    {
        ThreadInfo *inf;

        ObtainSemaphore(&thread_sem);
        thread = GetThreadId(NULL);
        if (thread == PTHREAD_THREADS_MAX)
        {
            // TODO: pthread_self is supposed to always succeed, but we can fail
            // here if we run out of thread slots
            // this can only happen if too many non-pthread processes call
            // this function
            //ReleaseSemaphore(&thread_sem);
            //return EAGAIN;
            abort();
        }
        inf = GetThreadInfo(thread);
        memset(inf, 0, sizeof(ThreadInfo));
        NEWLIST((struct List *)&inf->cleanup);
        inf->task = task;
        ReleaseSemaphore(&thread_sem);
    }

    return thread;
}
