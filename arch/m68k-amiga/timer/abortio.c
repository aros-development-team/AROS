/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AbortIO() - abort a running timer request.
    Lang: english
*/
#include <timer_intern.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>

/* See rom/timer/abortio.c for documentation */

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct timerequest *, timereq, A1),
    struct TimerBase *, TimerBase, 6,Timer)
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
    	ULONG unit = (ULONG)timereq->tr_node.io_Unit;
	timereq->tr_node.io_Error = IOERR_ABORTED;
	Remove((struct Node *)timereq);
	if (unit == UNIT_WAITUNTIL || unit == UNIT_VBLANK) {
	    if (IsListEmpty(&TimerBase->tb_Lists[UNIT_VBLANK]))
	    	TimerBase->tb_vblank_on = FALSE;
	} else {
	    if (IsListEmpty(&TimerBase->tb_Lists[UNIT_MICROHZ]))
	    	TimerBase->tb_micro_on = FALSE;
	}
	ReplyMsg((struct Message *)timereq);
	ret = 0;
    }
    Enable();


    return ret;

    AROS_LIBFUNC_EXIT
} /* AbortIO */
