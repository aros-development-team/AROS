/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: ReadEClock() - read the base frequency of timers.
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

    struct EClockVal *last = &TimerBase->tb_lasteclock;
    ULONG hi, lo;

    Disable();
    hi = TimerBase->tb_eclock.ev_hi;
    lo = TimerBase->tb_eclock.ev_lo + GetEClock(TimerBase);
    if (lo < TimerBase->tb_eclock.ev_lo)
        hi++;
    /* never before the last one: see ClockRead() */
    if (hi < last->ev_hi || (hi == last->ev_hi && lo < last->ev_lo)) {
        hi = last->ev_hi;
        lo = last->ev_lo;
    }
    last->ev_hi = hi;
    last->ev_lo = lo;
    Enable();
    dest->ev_hi = hi;
    dest->ev_lo = lo;

    return TimerBase->tb_eclock_rate;

    AROS_LIBFUNC_EXIT
} /* ReadEClock */
