/*
    Copyright (C) 2018-2026, The AROS Development Team. All rights reserved.

    Desc: GetUpTime() - Get time since machine was powered on.
*/
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include <timer_intern.h>

#include <devices/timer.h>
#include <proto/timer.h>

/* See rom/timer/getuptime.c for documentation */

AROS_LH1(void, GetUpTime,
    AROS_LHA(struct timeval *, dest, A0),
    struct Device *, TimerBase, 12, Timer)
{
    AROS_LIBFUNC_INIT

    struct TimerBase *timerBase = (struct TimerBase *)TimerBase;
    struct timeval now;

    Disable();
    ClockRead(timerBase, timerBase->tb_Elapsed.tv_secs, &now, &timerBase->tb_lastuptime, FALSE);
    Enable();
    dest->tv_secs = now.tv_secs;
    dest->tv_micro = ClockMicros(timerBase, now.tv_micro);
    D(bug("uptime=%d/%d\n", dest->tv_secs, dest->tv_micro));

    AROS_LIBFUNC_EXIT
} /* GetUpTime */
