/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ReadEClock() - EClock derived from the BCM free-running counter.
*/

#include <aros/libcall.h>
#include <devices/timer.h>
#include <proto/exec.h>

#include "timer_intern.h"
#include "systimer.h"

/*
 * Scale the 1MHz system timer to tb_eclock_rate rather than counting tick
 * interrupts: the compare is re-armed for MICROHZ requests and VBlank also
 * ticks on BCM2711/2712, so an interrupt count runs fast under load.
 *
 * The value counts from the counter's zero and does not follow
 * SetSysTime(), unlike the generic tb_ticks_total path. EClock values are
 * taken as differences, so a clock change must not land in an interval.
 * Reading the counter alone also keeps this out of the shared state that
 * EClockUpdate() books on SMP. Keep it off the generic path.
 */
AROS_LH1(ULONG, ReadEClock,
         AROS_LHA(struct EClockVal *, dest, A0),
         struct TimerBase *, TimerBase, 10, Timer)
{
    AROS_LIBFUNC_INIT

    ULONG rate = TimerBase->tb_eclock_rate;
    UQUAD now = SysTimer_Read(TimerBase);
    UQUAD ticks;

    if (rate == SYSTIMER_RATE)
        ticks = now;
    else
    {
        /* Split at whole seconds so now * rate cannot overflow. */
        ticks = (now / SYSTIMER_RATE) * rate
              + (now % SYSTIMER_RATE) * rate / SYSTIMER_RATE;
    }

    dest->ev_hi = (ULONG)(ticks >> 32);
    dest->ev_lo = (ULONG)ticks;

    return rate;

    AROS_LIBFUNC_EXIT
}
