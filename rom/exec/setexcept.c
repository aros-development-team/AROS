/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine and/or modify the signals which cause an exception.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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
    struct Task *me = FindTask(NULL);
    ULONG old;

    /* Protect mask of sent signals and task lists */
    Disable();

    /* Get returncode */
    old=me->tc_SigExcept;

    /* Change exception mask */
    me->tc_SigExcept=(old&~signalSet)|(newSignals&signalSet);

    /* Does this change include an exception? */
    if (me->tc_SigExcept & me->tc_SigRecvd)
    {
        /* Yes. Set the exception flag. */
        me->tc_Flags|=TF_EXCEPT;

        /* And order rescheduling */
        Reschedule();
    }
    Enable();

    return old;

    AROS_LIBFUNC_EXIT
}
