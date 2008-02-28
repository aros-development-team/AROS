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

#include <asm/amcc440.h>

#include "lowlevel.h"

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

    EClockUpdate(TimerBase);
    
    uint32_t tbu, tbl;
    Disable();
    do {
        tbu = rdspr(TBUU);
        tbl = rdspr(TBLU);
    } while(tbu != rdspr(TBUU));
    
    dest->ev_hi = tbu;
    dest->ev_lo = tbl;
    Enable();
    return 666666666;

    AROS_LIBFUNC_EXIT
} /* CmpTime */

