/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Examine and/or modify the signals of a task.
    Lang: english
*/

#define DEBUG 0

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/*****************************************************************************

    NAME */

	AROS_LH2(ULONG, SetSignal,

/*  SYNOPSIS */
	AROS_LHA(ULONG, newSignals, D0),
	AROS_LHA(ULONG, signalSet,  D1),

/*  LOCATION */
	struct ExecBase *, SysBase, 51, Exec)

/*  FUNCTION
	Change or read the set of signals sent to the current task.

    INPUTS
	newSignals - New values for the signals.
	signalSet  - Mask of signals affected by 'newSignals'.

    RESULT
	Old signal set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocSignal(), FreeSignal(), Wait(), Signal(), SetExcept()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *thisTask = GET_THIS_TASK;
    ULONG *sig;
    ULONG old = 0;

    if (thisTask)
    {
        /* Protect the signal mask against access by other tasks. */
        Disable();

        /* Get address */
        sig = &thisTask->tc_SigRecvd;

        /* Change only the bits in 'mask' */
        old = *sig;
        *sig = (old & ~signalSet) | (newSignals & signalSet);

        Enable();
    }

    return old;
    AROS_LIBFUNC_EXIT
} /* SetSignal() */

