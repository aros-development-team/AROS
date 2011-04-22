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

    D(bug("[Timer] BeginIO(0x%p)\n", timereq));

    EClockUpdate(TimerBase);

    if (common_BeginIO(timereq, TimerBase))
    {
	D(bug("[BeginIO] Updating hardware interrupt request\n"));

	outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
	Timer0Setup(TimerBase);
    }

    D(bug("[Timer] BeginIO() done\n", timereq));

    AROS_USERFUNC_EXIT
}
