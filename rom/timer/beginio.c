/*
    Copyright (C) 1995-1998 AROS
    $Id$

    Desc: BeginIO - Start up a timer.device request.
    Lang: english
*/
#include "timer_intern.h"
#include <exec/errors.h>
#include <proto/exec.h>

/*****i***********************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>
	AROS_LH1(void, BeginIO,

/*  SYNOPSIS */
	AROS_LHA(struct timerequest *, timereq, A1),

/*  LOCATION */
	struct TimerBase *, TimerBase, 5, Timer)

/*  FUNCTION
	BeginIO() will perform a timer.device command. It is normally
	called from within DoIO() and SendIO().

    INPUT
	timereq		- The request to process.

    RESULT
	The requested message will be processed.

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	exec/Abort(), exec/SendIO(), exec/DoIO()

    INTERNALS

    HISTORY
	23-01-1998  iaint	Implemented again.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct TimerBase *, TimerBase)

    ULONG unitNum;
    BOOL replyit = FALSE;
    struct timerequest *tr;

    timereq->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    unitNum = (ULONG)timereq->tr_node.io_Unit;

    switch(timereq->tr_node.io_Command)
    {
	case TR_GETSYSTIME:
	    GetSysTime(&timereq->tr_time);

	    if(!(timereq->tr_node.io_Flags & IOF_QUICK))
	    {
		ReplyMsg((struct Message *)timereq);
	    }
	    replyit = FALSE; /* Because replyit will clear the timeval */
	    break;

	case TR_SETSYSTIME:
	    Disable();
	    TimerBase->tb_CurrentTime.tv_secs = timereq->tr_time.tv_secs;
	    TimerBase->tb_CurrentTime.tv_micro = timereq->tr_time.tv_micro;
	    Enable();
	    replyit = TRUE;
	    break;

	case TR_ADDREQUEST:

	case CMD_CLEAR:
	case CMD_FLUSH:
	case CMD_INVALID:
	case CMD_READ:
	case CMD_RESET:
	case CMD_START:
	case CMD_STOP:
	case CMD_UPDATE:
	case CMD_WRITE:
	default:
	    replyit = TRUE;
	    timereq->tr_node.io_Error = IOERR_NOCMD;
	    break;
    } /* switch(command) */

    if(replyit)
    {
	timereq->tr_time.tv_secs = 0;
	timereq->tr_time.tv_micro = 0;
	if(!(timereq->tr_node.io_Flags & IOF_QUICK))
	{
	    ReplyMsg((struct Message *)timereq);
	}
    }

    AROS_LIBFUNC_EXIT
} /* BeginIO */
