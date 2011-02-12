/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadEClock() - read the base frequency of timers.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>

#include <timer_intern.h>

	AROS_LH1(ULONG, ReadEClock,

/*  SYNOPSIS */
	AROS_LHA(struct EClockVal *, dest, A0),

/*  LOCATION */
	struct TimerBase *, TimerBase, 10, Timer)

/*  FUNCTION
	ReadEClock() reads current value of E-Clock and stores
        it in the destination EClockVal structure passed as
        argument. It also returns the frequency of EClock of the
        system.

        This call is supposed to be very fast.
    INPUTS
	dest    -   Destination EClockVal

    RESULT
	The EClock frequency (tics/s)

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	19-08-2005  schulz   Implemented.

*****************************************************************************/
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

