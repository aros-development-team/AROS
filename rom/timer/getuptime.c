/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetUpTime() - Get time since machine was powered on.
    Lang: english
*/

#include <proto/exec.h>

#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH1(void, GetUpTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),

/*  LOCATION */
	struct Device *, TimerBase, 12, Timer)

/*  FUNCTION
	GetUpTime() will fill in the supplied timeval with the current
	uptime.

    INPUTS
	dest    -   A pointer to the timeval you want the time stored in.

    RESULT
	The timeval "dest" will be filled with the current uptime. This timer
	cannot be changed by the software and thus can be considered to be a
	monotonic clock..

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	TR_GETSYSTIME, TR_SETSYSTIME, GetSysTime

    INTERNALS

    HISTORY
	05-08-2018  schulz   Implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Disable();

    /* Query the hardware */
    EClockUpdate(GetTimerBase(TimerBase));
    dest->tv_secs  = GetTimerBase(TimerBase)->tb_Elapsed.tv_secs;
    dest->tv_micro = GetTimerBase(TimerBase)->tb_Elapsed.tv_micro;

    Enable();

    AROS_LIBFUNC_EXIT
} /* GetUpTime */
