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
        void pthread_exit(

/*  SYNOPSIS */
        void *value_ptr)

/*  FUNCTION
        Finish the calling worker with value_ptr as its result. Run registered
        cleanup handlers in reverse registration order, then return control to
        the worker starter without returning to this caller. The starter runs
        thread-specific destructors and C runtime leave hooks before marking
        the worker finished or releasing its detached record.

    INPUTS
        value_ptr - Result pointer to make available to pthread_join(); may be
            NULL. The pointer is stored unchanged, not copied or freed. Any
            pointed-to result must remain valid after the worker's stack and
            runtime have been released.

    RESULT
        Does not return for a worker created by pthread_create().

    NOTES
        Include <pthread.h>; link with pthread. Call only from a registered
        worker after its entry point has started. Cleanup handlers run in that
        worker's context and must return normally. Their execution can block.
        Held mutexes and other application resources are not released
        automatically; arrange explicit cleanup before exiting.

        Non-NULL thread-specific values with destructors are cleared before
        invocation. Reinstalled values can cause further passes, up to
        PTHREAD_DESTRUCTOR_ITERATIONS. Program-wide teardown skips these
        destructors. A joinable completion record remains until joined.

    BUGS
        The initial program task has no initialized exit jump point. Calling
        pthread_exit() there, or from an unregistered task, is unsupported.
        TLS destructors run while the runtime holds its shared TLS semaphore;
        creating or deleting keys from a destructor can contend for an
        exclusive acquisition and must not be relied on.

    SEE ALSO
        pthread_create(),
        pthread_join(),
        pthread_key_create()

*****************************************************************************/
{
    pthread_t thread;
    ThreadInfo *inf;
    CleanupHandler *handler;

    D(bug("%s(%p)\n", __FUNCTION__, value_ptr));

    thread = pthread_self();
    inf = GetThreadInfo(thread);
    inf->ret = value_ptr;

    // execute the clean-up handlers
    while ((handler = (CleanupHandler *)RemTail((struct List *)&inf->cleanup)))
        if (handler->routine)
            handler->routine(handler->arg);

    longjmp(inf->jmp, 1);
}
