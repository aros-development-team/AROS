/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Add a semaphore to the public list of semaphores.
*/

#include <exec/semaphores.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, AddSemaphore,

/*  SYNOPSIS */
        AROS_LHA(struct SignalSemaphore *, sigSem, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 100, Exec)

/*  FUNCTION
        Adds a semaphore to the system public semaphore list. Since the
        semaphore gets initialized by this function it must be free at
        this time. Also the ln_Name field must be set.

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

    /* Initialize semaphore */
    InitSemaphore(sigSem);

    /* Arbitrate for the semaphore list */
    Forbid();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->SemListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    /* Add the semaphore */
    Enqueue(&SysBase->SemaphoreList,&sigSem->ss_Link);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->SemListSpinLock);
#endif
    /* All done. */
    Permit();

    AROS_LIBFUNC_EXIT
} /* AddSemaphore */

