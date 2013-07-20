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

#include <proto/battclock.h>

/* See rom/battclock/readbattclock.c for documentation */

AROS_LH0(ULONG, ReadBattClock,
    struct BattClockBase*, BattClockBase, 2, Battclock)
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
    if (cd.year < 1978)
    	cd.year += 100;
    cd.wday = 0;
    t = Date2Amiga(&cd);
    D(bug("%02d:%02d %02d.%02d.%d = %d\n", cd.hour, cd.min, cd.mday, cd.month, cd.year, t));
    return t;

    AROS_LIBFUNC_EXIT

} /* ReadBattClock */
