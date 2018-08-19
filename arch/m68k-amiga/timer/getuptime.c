/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetUpTime()
    Lang: english
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
    ULONG tb_eclock_to_usec;

    Disable();
    tb_eclock_to_usec = timerBase->tb_eclock_to_usec + GetEClock(timerBase);
    Enable();
    dest->tv_secs = timerBase->tb_Elapsed.tv_secs;
    dest->tv_micro = (ULONG)(((long long)tb_eclock_to_usec * timerBase->tb_eclock_micro_mult) >> 16);

    // can only overflow once, e-clock interrupts happen more than once a second
    if (dest->tv_micro >= 1000000) {
    	dest->tv_micro -= 1000000;
    	dest->tv_secs++;
    }
    D(bug("uptime=%d/%d\n", dest->tv_secs, dest->tv_micro));

    AROS_LIBFUNC_EXIT
} /* GetUpTime */
