/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: readeclock.c 12532 2001-10-27 19:46:10Z chodorowski $

    Desc: ReadEClock() - read the base frequency of timers.
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <exec/execbase.h>
#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>

#include "timer_intern.h"

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
    AROS_LIBBASE_EXT_DECL(struct TimerBase *,TimerBase)

    Disable();
    dest->ev_hi = (ULONG)(TimerBase->tb_ticks_total >> 32);
    dest->ev_lo = (ULONG)(TimerBase->tb_ticks_total & 0xffffffff);
    Enable();
    
    return (SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency);

    AROS_LIBFUNC_EXIT
} /* CmpTime */

