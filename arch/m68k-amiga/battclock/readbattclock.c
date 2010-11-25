/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: ReadBattClock() function.
    Lang: english
*/
#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <utility/date.h>
#include <proto/utility.h>

#include "battclock_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battclock.h>

	AROS_LH0(ULONG, ReadBattClock,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct BattClockBase*, BattClockBase, 2, Battclock)

/*  FUNCTION
	Return the value stored in the battery back up clock. This value
	is the number of seconds that have elapsed since midnight on the
	1st of January 1978 (00:00:00 1.1.1978).

	If the value of the battery clock is invalid, then the clock will
	be reset.

    INPUTS

    RESULT
	The number of seconds since 1.1.1978 00:00:00

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WriteBattClock(), ResetBattClock()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    volatile UBYTE *p = BattClockBase->clockptr;
    struct UtilityBase *UtilityBase = BattClockBase->UtilityBase;
    struct ClockData cd;
    UBYTE reg;
    ULONG t;

    D(bug("ReadBattClock\n"));
    if (!p)
    	return 0;
    reg = 0;
    cd.sec = getbcd(p, reg);
    cd.min = getbcd(p, reg + 2);
    cd.hour = getbcd(p, reg + 4);
    if (BattClockBase->clocktype == MSM6242B)
    	reg = 6;
    else
    	reg = 7;
    cd.mday = getbcd(p, reg);
    cd.month = getbcd(p, reg + 2);
    cd.year = getbcd(p, reg + 4) + 1900;
    cd.wday = 0;
    t = Date2Amiga(&cd);
    D(bug("%02d:%02d %02d.%02d.%d = %d\n", cd.hour, cd.min, cd.mday, cd.month, cd.year, t));
    return t;

    AROS_LIBFUNC_EXIT

} /* ReadBattClock */
