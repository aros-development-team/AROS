/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: GetSysTime() - Find out what time it is.
*/
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include <timer_intern.h>

#include <devices/timer.h>
#include <proto/timer.h>

/* See rom/timer/getsystime.c for documentation */

AROS_LH1(void, GetSysTime,
    AROS_LHA(struct timeval *, dest, A0),
    struct Device *, TimerBase, 11, Timer)
{
    AROS_LIBFUNC_INIT

    struct TimerBase *timerBase = (struct TimerBase *)TimerBase;
    struct timeval now;
    ULONG set;

    /* each call a later time than the one before, as AmigaOS's */
    Disable();
    ClockRead(timerBase, timerBase->tb_CurrentTime.tv_secs, &now, &timerBase->tb_lastsystime, TRUE);
    set = timerBase->tb_CurrentTime.tv_micro;
    Enable();
    /* the e-clock's fraction, and the one TR_SETSYSTIME set */
    dest->tv_secs = now.tv_secs;
    dest->tv_micro = ClockMicros(timerBase, now.tv_micro) + set;
    if (dest->tv_micro >= 1000000) {
        dest->tv_micro -= 1000000;
        dest->tv_secs++;
    }
    D(bug("systime=%d/%d\n", dest->tv_secs, dest->tv_micro));

    AROS_LIBFUNC_EXIT
} /* GetSysTime */
