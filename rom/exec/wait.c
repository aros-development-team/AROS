/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Wait for some signal.
    Lang: english
*/
#define DEBUG 0

#include <aros/debug.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#endif

/*****************************************************************************

    NAME */

	AROS_LH1(ULONG, Wait,

/*  SYNOPSIS */
	AROS_LHA(ULONG, signalSet, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 53, Exec)

/*  FUNCTION
	Wait until some signals are sent to the current task. If any signal
	of the specified set is already set when entering this function it
	returns immediately. Since almost any event in the OS can send a
	signal to your task if you specify it to do so signals are a very
	powerful mechanism.

    INPUTS
	signalSet - The set of signals to wait for.

    RESULT
	The set of active signals.

    NOTES
	Naturally it's not allowed to wait in supervisor mode.

	Calling Wait() breaks an active Disable() or Forbid().

    EXAMPLE

    BUGS

    SEE ALSO
	Signal(), SetSignal(), AllocSignal(), FreeSignal()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ThisTask = GET_THIS_TASK;
    ULONG rcvd;

    D(bug("[Exec] Wait(%08lX)\n", signalSet);)
#if !defined(__AROSEXEC_SMP__)
    Disable();
#endif

    /* If at least one of the signals is already set do not wait. */
    while (!(ThisTask->tc_SigRecvd & signalSet))
    {
	/* Set the wait signal mask */
	ThisTask->tc_SigWait = signalSet;

        D(bug("[Exec] Moving '%s' @ 0x%p to Task Wait queue\n", ThisTask->tc_Node.ln_Name, ThisTask);)
        D(bug("[Exec] Task state = %08x\n", ThisTask->tc_State);)

        /*
            Clear TDNestCnt (because Switch() will not care about it),
            but memorize it first. IDNestCnt is handled by Switch().
            */
        ThisTask->tc_TDNestCnt = TDNESTCOUNT_GET;
        TDNESTCOUNT_SET(-1);

        ThisTask->tc_State = TS_WAIT;
        // nb: on smp builds switch will move us.
#if !defined(__AROSEXEC_SMP__)
        /* Move current task to the waiting list. */
        Enqueue(&SysBase->TaskWait, &ThisTask->tc_Node);
#endif

	/* And switch to the next ready task. */
	KrnSwitch();

	/*
	    OK. Somebody awakened us. This means that either the
	    signals are there or it's just a finished task exception.
	    Test again to be sure (see above).
	*/

	/* Restore TDNestCnt. */
	TDNESTCOUNT_SET(ThisTask->tc_TDNestCnt);
    }
    /* Get active signals. */
    rcvd = (ThisTask->tc_SigRecvd & signalSet);

    /* And clear them. */
#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_LOCKDISABLE(&IntETask(ThisTask->tc_UnionETask.tc_ETask)->iet_TaskLock, SPINLOCK_MODE_WRITE);
#endif
    ThisTask->tc_SigRecvd &= ~signalSet;
#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_UNLOCK(&IntETask(ThisTask->tc_UnionETask.tc_ETask)->iet_TaskLock);
#endif
    Enable();

    /* All done. */
    return rcvd;

    AROS_LIBFUNC_EXIT
}

