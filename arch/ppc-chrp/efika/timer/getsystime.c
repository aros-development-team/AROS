/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetSysTime() - Find out what time it is.
    Lang: english
*/
#include "lowlevel.h"
#include <proto/exec.h>

#include <devices/timer.h>
#include <proto/timer.h>

/* See rom/timer/getsystime.c for documentation */

AROS_LH1(void, GetSysTime,
    AROS_LHA(struct timeval *, dest, A0),
    struct Device *, TimerBase, 11, Timer)
{
    AROS_LIBFUNC_INIT

    struct TimerBase *timerBase = (struct TimerBase *)TimerBase;

    Disable();
    EClockUpdate(TimerBase);
    dest->tv_secs = timerBase->tb_CurrentTime.tv_secs;
    dest->tv_micro = timerBase->tb_CurrentTime.tv_micro;
    Enable();

    AROS_LIBFUNC_EXIT
} /* GetSysTime */
