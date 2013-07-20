/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadEClock() - read the base frequency of timers.
    Lang: english
*/

#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>

#include <timer_intern.h>

/* See rom/timer/readeclock.c for documentation */

AROS_LH1(ULONG, ReadEClock,
    AROS_LHA(struct EClockVal *, dest, A0),
    struct TimerBase *, TimerBase, 10, Timer)
{
    AROS_LIBFUNC_INIT

    ULONG eclock, old;

    Disable();
    old = dest->ev_lo = TimerBase->tb_eclock.ev_lo;
    dest->ev_hi = TimerBase->tb_eclock.ev_hi;
    eclock = GetEClock(TimerBase);
    Enable();
    dest->ev_lo += eclock;
    if (old > dest->ev_lo)
    	dest->ev_hi++;
    return TimerBase->tb_eclock_rate;

    AROS_LIBFUNC_EXIT
} /* CmpTime */

