/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadEClock() - read the base frequency of timers.
    Lang: english
*/

#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>

#include <asm/mpc5200b.h>

#include "lowlevel.h"

/* See rom/timer/readeclock.c for documentation */

AROS_LH1(ULONG, ReadEClock,
    AROS_LHA(struct EClockVal *, dest, A0),
    struct TimerBase *, TimerBase, 10, Timer)
{
    AROS_LIBFUNC_INIT

    Disable();

    EClockUpdate(TimerBase);

    dest->ev_hi = TimerBase->tb_ticks_total >> 32;
    dest->ev_lo = TimerBase->tb_ticks_total & 0xffffffff;

    Enable();
    return 33000000;

    AROS_LIBFUNC_EXIT
} /* CmpTime */

