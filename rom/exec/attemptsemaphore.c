/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Try to lock a sempahore.
*/

#include <exec/semaphores.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "semaphores.h"

/*****************************************************************************

    NAME */

        AROS_LH1(ULONG, AttemptSemaphore,

/*  SYNOPSIS */
        AROS_LHA(struct SignalSemaphore *, sigSem, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 96, Exec)

/*  FUNCTION
        Tries to get an exclusive lock on a signal semaphore. If the semaphore
        is already in use by another task, this function does not wait but
        returns false instead.

    INPUTS
        sigSem - Pointer to semaphore structure.

    RESULT
        TRUE if the semaphore could be obtained, FALSE otherwise.

    NOTES
        The lock must be freed with ReleaseSemaphore().

    EXAMPLE

    BUGS

    SEE ALSO
        ReleaseSemaphore()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TraceLocation tp = CURRENT_LOCATION("AttemptSemaphore");
    struct Task *ThisTask = GET_THIS_TASK;

    return InternalAttemptSemaphore(sigSem, ThisTask, &tp, SysBase);

    AROS_LIBFUNC_EXIT
} /* AttemptSemaphore */
