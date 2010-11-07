/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetSysTime() - Find out what time it is.
    Lang: english
*/
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH1(void, GetSysTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),

/*  LOCATION */
	struct Device *, TimerBase, 11, Timer)

/*  FUNCTION
	GetSysTime() will fill in the supplied timeval with the current
	system time.

    INPUTS
	dest    -   A pointer to the timeval you want the time stored in.

    RESULT
	The timeval "dest" will be filled with the current system time.

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	TR_GETSYSTIME, TR_SETSYSTIME

    INTERNALS

    HISTORY
	18-02-1997  iaint   Implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TimerBase *timerBase = (struct TimerBase *)TimerBase;
    struct EClockVal ev;

    GetEClock(timerBase, &ev);
    
    dest->tv_secs = timerBase->tb_CurrentTime.tv_secs;
    // ugh, 64-bit multiplication and division..
    dest->tv_micro = (long long)timerBase->tb_eclock_to_usec * 1000000 / timerBase->tb_eclock_rate;
    if (equ64(dest, &timerBase->tb_lastsystime))
    	inc64(dest);
    timerBase->tb_lastsystime.tv_secs = dest->tv_secs;
    timerBase->tb_lastsystime.tv_micro = dest->tv_micro;
	D(bug("systime=%d/%d\n", dest->tv_secs, dest->tv_micro));
    AROS_LIBFUNC_EXIT
} /* GetSysTime */
