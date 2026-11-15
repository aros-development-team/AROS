/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ReadEClock() - EClock derived from the BCM free-running counter.
*/

#include <aros/libcall.h>
#include <devices/timer.h>
#include <proto/exec.h>

#include "timer_intern.h"

/*
 * Scale the 1MHz system timer to tb_eclock_rate rather than counting tick
 * interrupts: the compare is re-armed for MICROHZ requests and VBlank also
 * ticks on BCM2711/2712, so an interrupt count runs fast under load.
 */
AROS_LH1(ULONG, ReadEClock,
         AROS_LHA(struct EClockVal *, dest, A0),
         struct TimerBase *, TimerBase, 10, Timer)
{
    AROS_LIBFUNC_INIT

    ULONG rate = TimerBase->tb_eclock_rate;
    unsigned int chi, clo;
    UQUAD now, ticks;

    /* Re-read CHI in case CLO carried between the two loads. */
    do
    {
        chi = *((volatile unsigned int *)(SYSTIMER_CHI));
        clo = *((volatile unsigned int *)(SYSTIMER_CLO));
    } while (chi != *((volatile unsigned int *)(SYSTIMER_CHI)));

    now = ((UQUAD)chi << 32) | clo;

    /* Split at whole seconds so now * rate cannot overflow. */
    ticks = (now / 1000000) * rate + (now % 1000000) * rate / 1000000;

    dest->ev_hi = (ULONG)(ticks >> 32);
    dest->ev_lo = (ULONG)ticks;

    return rate;

    AROS_LIBFUNC_EXIT
}
