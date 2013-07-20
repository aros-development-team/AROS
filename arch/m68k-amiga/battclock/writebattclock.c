/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.

    Desc: WriteBattClock()
    Lang: english
*/
#include "battclock_intern.h"

#include <utility/date.h>
#include <proto/utility.h>

#include <proto/battclock.h>

/* See rom/battclock/writebattclock.c for documentation */

AROS_LH1(void, WriteBattClock,
    AROS_LHA(ULONG, time, D0),
    struct BattClockBase*, BattClockBase, 3, Battclock)
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
