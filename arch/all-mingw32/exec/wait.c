/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Wait for some signal.
    Lang: english
*/
#define DEBUG 0

#include <aros/debug.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/kernel.h>

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

    ULONG rcvd;
    struct Task *me;

    /* Get pointer to current task - I'll need it very often */
    me = FindTask (NULL);

    D(bug("[Exec] Wait(0x%08lX) called by %s\n", signalSet, me->tc_Node.ln_Name));
    /* Protect the task lists against access by other tasks. */
    Disable();

    /* If at least one of the signals is already set do not wait. */
    while(!(me->tc_SigRecvd&signalSet))
    {
	D(bug("[Exec] Signals are not set, putting the task to sleep\n"));
	/* Set the wait signal mask */
	me->tc_SigWait=signalSet;

	/*
	    Clear TDNestCnt (because Switch() will not care about it),
	    but memorize it first. IDNestCnt is handled by Switch().
	*/
	me->tc_TDNestCnt=SysBase->TDNestCnt;
	SysBase->TDNestCnt=-1;

	/* Move current task to the waiting list. */
	me->tc_State=TS_WAIT;

	/*
		remove it from what!?
		sheutlin
	*/
//      me->tc_Node.ln_Pred->ln_Succ = me->tc_Node.ln_Succ;
//	me->tc_Node.ln_Succ->ln_Pred = me->tc_Node.ln_Pred;

	Enqueue(&SysBase->TaskWait,&me->tc_Node);

	/* And switch to the next ready task. */
	KrnSwitch();

//	Reschedule(me);

	/*
	    OK. Somebody awakened me. This means that either the
	    signals are there or it's just a finished task exception.
	    Test again to be sure (see above).
	*/

	/* Restore TDNestCnt. */
	SysBase->TDNestCnt=me->tc_TDNestCnt;
    }
    /* Get active signals. */
    rcvd=me->tc_SigRecvd&signalSet;

    /* And clear them. */
    me->tc_SigRecvd&=~signalSet;

    /* All done. */
    Enable();

    return rcvd;
    AROS_LIBFUNC_EXIT
}

