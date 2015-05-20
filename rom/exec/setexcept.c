/*
    Copyright � 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine and/or modify the signals which cause an exception.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/*****************************************************************************

    NAME */

        AROS_LH2(ULONG, SetExcept,

/*  SYNOPSIS */
        AROS_LHA(ULONG, newSignals, D0),
        AROS_LHA(ULONG, signalSet,  D1),

/*  LOCATION */
        struct ExecBase *, SysBase, 52, Exec)

/*  FUNCTION
        Change the mask of signals causing a task exception.

    INPUTS
        newSignals - Set of signals causing the exception.
        signalSet  - Mask of affected signals.

    RESULT
        Old mask of signals causing a task exception.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocSignal(), FreeSignal(), Wait(), SetSignal(), Signal()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to current task */
    struct Task *ThisTask = GET_THIS_TASK;
    ULONG old;

    /* Protect mask of sent signals and task lists */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&IntETask(ThisTask->tc_UnionETask.tc_ETask)->iet_TaskLock, SPINLOCK_MODE_WRITE);
#endif
    Disable();

    /* Get returncode */
    old = ThisTask->tc_SigExcept;

    /* Change exception mask */
    ThisTask->tc_SigExcept = (old & ~signalSet) | (newSignals & signalSet);

    /* Does this change include an exception? */
    if (ThisTask->tc_SigExcept & ThisTask->tc_SigRecvd)
    {
        /* Yes. Set the exception flag. */
        ThisTask->tc_Flags |= TF_EXCEPT;

        /* And order rescheduling */
        Reschedule();
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&IntETask(ThisTask->tc_UnionETask.tc_ETask)->iet_TaskLock);
#endif
    Enable();

    return old;

    AROS_LIBFUNC_EXIT
}
