/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2007 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

#include <exec/lists.h>
#include <proto/thread.h>

/*****************************************************************************

    NAME */
        AROS_LH0(void, WaitAllThreads,

/*  SYNOPSIS */

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 7, Thread)

/*  FUNCTION
        Blocks the current task until all threads exit.

    INPUTS
        None.

    RESULT
        This function always succeeds.

    NOTES
        This function will ignore detached threads.

    EXAMPLE
        WaitAllThreads();

    BUGS

    SEE ALSO
        CreateThread(), CurrentThread(), DetachThread(), WaitThread()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    _Thread thread, next;

    ForeachNodeSafe(&ThreadBase->threads, thread, next)
        WaitThread(thread->id, NULL);

    AROS_LIBFUNC_EXIT
} /* WaitAllThreads */
