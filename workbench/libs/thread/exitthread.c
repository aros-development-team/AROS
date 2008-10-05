/*
 * thread.library - threading and synchronisation primitives
 *
 * Copyright © 2008 Markus Weiss
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "thread_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, ExitThread,

/*  SYNOPSIS */
        AROS_LHA(void *, result, A0),

/*  LOCATION */
        struct ThreadBase *, ThreadBase, 20, Thread)

/*  FUNCTION
        Exits the calling thread.

    INPUTS
        result    - pointer to storage for the thread's return value. You can
                    pass NULL here if you don't care about the return value.

    RESULT
        None, this function never returns.

    NOTES
	This function is similar to the exit() function of arosc library.
	
    EXAMPLE
	ExitThread(5);

    BUGS

    SEE ALSO
        WaitThread(), WaitAllThreads(), exit()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct _Thread *thread = _getthreadbytask(FindTask(NULL), ThreadBase);
    if (thread == NULL)
	/* if called from a task that is not a thread */
        return;

    thread->result = result;

    longjmp(thread->jmp, 1);

    AROS_LIBFUNC_EXIT
} /* ExitThread */
