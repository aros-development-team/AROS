/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:18  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:55  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:58  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:09  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:21  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

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
    me=SysBase->ThisTask;

    /* Protect the task lists against access by other tasks. */
    Disable();

    /* If at least one of the signals is already set do not wait. */
    while(!(me->tc_SigRecvd&signalSet))
    {
	/* Set the wait signal mask */
	me->tc_SigWait=signalSet;

	/*
	    Clear TDNestCnt (because Switch() will not care about it),
	    but memorize it first. IDNestCnt is handled by Switch().
	    This could as well be stored in a local variable which makes
	    the tc_TDNestCnt field somehow redundant.
	*/
	me->tc_TDNestCnt=SysBase->TDNestCnt;
	SysBase->TDNestCnt=-1;

	/* Move current task to the waiting list. */
	me->tc_State=TS_WAIT;
	Enqueue(&SysBase->TaskWait,&me->tc_Node);

	/* And switch to the next ready task. */
	Switch();
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

