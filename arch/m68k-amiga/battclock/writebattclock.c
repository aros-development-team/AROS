/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: WriteBattClock()
    Lang: english
*/
#include "battclock_intern.h"

#include <utility/date.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/battclock.h>

	AROS_LH1(void, WriteBattClock,

/*  SYNOPSIS */
	AROS_LHA(ULONG, time, D0),

/*  LOCATION */
	struct BattClockBase*, BattClockBase, 3, Battclock)

/*  FUNCTION
	Set the system's battery backed up clock to the time specified. The
	value should be the number of seconds since 00:00:00 on 1.1.1978.

    INPUTS
	time - The number of seconds elapsed since 00:00:00 1.1.1978

    RESULT
	The clock will be set.

    NOTES
	This may not do anything on some systems where the battery backed
	up clock either doesn't exist, or may not be writable.

    EXAMPLE

    BUGS

    SEE ALSO
	ReadBattClock(), ResetBattClock()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    volatile UBYTE *p = BattClockBase->clockptr;
    struct UtilityBase *UtilityBase = BattClockBase->UtilityBase;
    struct ClockData cd;
    UBYTE reg;

    if (!p)
    	return;
    Amiga2Date(time, &cd);
    stopclock(BattClockBase);
    reg = 0;
    putbcd(p, reg, cd.sec);
    putbcd(p, reg + 2, cd.min);
    putbcd(p, reg + 4, cd.hour);
    if (BattClockBase->clocktype == MSM6242B)
    	reg = 6;
    else
    	reg = 7;
    putbcd(p, reg, cd.mday);
    putbcd(p, reg + 2, cd.month);
    putbcd(p, reg + 4, cd.year - 1900);
    startclock(BattClockBase);
    return;

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */
