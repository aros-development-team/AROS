/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BeginIO - Start up a timer.device request.
    Lang: english
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <asm/io.h>

#include "ticks.h"

AROS_LH1(void, BeginIO,
	 AROS_LHA(struct timerequest *, timereq, A1),
	 struct TimerBase *, TimerBase, 5, Timer)
{
    AROS_LIBFUNC_INIT

    D(bug("[Timereq 0x%p] unit %ld, command %d\n", timereq, timereq->tr_node.io_Unit, timereq->tr_node.io_Command));

#if DEBUG
    if (timereq->tr_node.io_Command == TR_ADDREQUEST)
    {
	bug("[Timereq 0x%p] Request time %d sec %d usec\n", timereq, timereq->tr_time.tv_secs, timereq->tr_time.tv_micro);
    }
#endif

    if (common_BeginIO(timereq, TimerBase))
    {
	D(bug("[Timereq 0x%p] Updating hardware interrupt request\n", timereq));

	Disable();
	Timer0Setup(TimerBase);
	Enable();
    }

#if DEBUG
    if (timereq->tr_node.io_Command == TR_ADDREQUEST)
    {
    	bug("[Timereq 0x%p] Request time %d sec %d usec\n", timereq, timereq->tr_time.tv_secs, timereq->tr_time.tv_micro);
    	bug("[Timereq 0x%p] Elapsed time %d sec %d usec\n", timereq, TimerBase->tb_Elapsed.tv_secs, TimerBase->tb_Elapsed.tv_micro);
    }
#endif

    AROS_USERFUNC_EXIT
}
