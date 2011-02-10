/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AbortIO() - abort a running timer request.
    Lang: english
*/
#include <timer_intern.h>
#include <exec/io.h>
#include <exec/errors.h>

/*****i***********************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>

	AROS_LH1(LONG, AbortIO,

/*  SYNOPSIS */
	AROS_LHA(struct timerequest *, timereq, A1),

/*  LOCATION */
	struct TimerBase *, TimerBase, 6,Timer)

/*  FUNCTION
	Abort a running timer.device request.

    INPUTS
	timereq     -   The request you wish to abort.

    RESULT
	0   if the request was aborted, io_Error will also be set
	    to the value IOERR_ABORTED.

	-1  otherwise (most likely that the request isn't working).

	If the request is successfully aborted, you should WaitIO() on
	the message before you try and reuse it.

    NOTES
	This function may be called from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	exec/AbortIO(), exec/WaitIO()

    INTERNALS

    HISTORY
	18-02-1997  iaint   Implemented.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG ret = -1;

    /*
	As the timer.device runs as an interrupt, we had better protect
	the "waiting timers" list from being corrupted.
    */

    Disable();
    if(timereq->tr_node.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
    {
		timereq->tr_node.io_Error = IOERR_ABORTED;
		Remove((struct Node *)timereq);
		ReplyMsg((struct Message *)timereq);
		ret = 0;
    }
    Enable();


    return ret;

    AROS_LIBFUNC_EXIT
} /* AbortIO */
