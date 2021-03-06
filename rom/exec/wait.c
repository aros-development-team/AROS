/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Wait for some signal.
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

    struct Task *thisTask = GET_THIS_TASK;
    ULONG rcvd;

    D(bug("[Exec] Wait(%08lX)\n", signalSet);)
    Disable();

    /* If at least one of the signals is already set do not wait. */
    while (!(thisTask->tc_SigRecvd & signalSet))
    {
        /* Set the wait signal mask */
        thisTask->tc_SigWait = signalSet;

        D(bug("[Exec] Wait: Moving '%s' @ 0x%p to Task Wait queue\n", thisTask->tc_Node.ln_Name, thisTask);)
        D(bug("[Exec] Wait: Task state = %08x\n", thisTask->tc_State);)

        /*
            Clear TDNestCnt (because Switch() will not care about it),
            but memorize it first. IDNestCnt is handled by Switch().
            */
        thisTask->tc_TDNestCnt = TDNESTCOUNT_GET;
        D(bug("[Exec] Wait: Task TDNestCount = %d\n", thisTask->tc_TDNestCnt);)
        TDNESTCOUNT_SET(-1);

        thisTask->tc_State = TS_WAIT;
        // nb: on smp builds switch will move us.
#if !defined(__AROSEXEC_SMP__)
        /* Move current task to the waiting list. */
        Enqueue(&SysBase->TaskWait, &thisTask->tc_Node);
#endif

        /* And switch to the next ready task. */
        KrnSwitch();

        /*
            OK. Somebody awakened us. This means that either the
            signals are there or it's just a finished task exception.
            Test again to be sure (see above).
        */
        D(bug("[Exec] Wait: Awoken...\n");)

        /* Restore TDNestCnt. */
        TDNESTCOUNT_SET(thisTask->tc_TDNestCnt);
    }
    /* Get active signals. */
    rcvd = (thisTask->tc_SigRecvd & signalSet);

    /* And clear them. */
#if defined(__AROSEXEC_SMP__)
    __AROS_ATOMIC_AND_L(thisTask->tc_SigRecvd, ~signalSet);
#else
    thisTask->tc_SigRecvd &= ~signalSet;
#endif
    Enable();

    /* All done. */
    return rcvd;

    AROS_LIBFUNC_EXIT
}

