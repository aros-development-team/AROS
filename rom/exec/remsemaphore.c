/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: Remove a semaphore from the list of public semaphores.
*/

#include "exec_intern.h"
#include <exec/semaphores.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(void, RemSemaphore,

/*  SYNOPSIS */
        AROS_LHA(struct SignalSemaphore *, sigSem, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 101, Exec)

/*  FUNCTION
        Removes a semaphore from the system public semaphore list.

    INPUTS
        sigSem - Pointer to semaphore structure

    RESULT

    NOTES
        Semaphores are shared between the tasks that use them and must
        therefore lie in public (or at least shared) memory.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the semaphore list */
    Forbid();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->SemListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    /* Remove the semaphore */
    Remove(&sigSem->ss_Link);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->SemListSpinLock);
#endif
    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemSemaphore */

