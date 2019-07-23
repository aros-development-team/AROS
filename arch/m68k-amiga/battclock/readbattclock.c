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
    UBYTE retry;

    D(bug("ReadBattClock\n"));
    if (!p)
    	return 0;
    
    /* 
       Repeat reading for the second time if the number of seconds read
       at the beginning and at the end of RTC access differs. Do it at most twice
       in order to avoid situatuions where RTC was missing and number of seconds
       receives just noise.
    */

    Disable();
    for (retry = 0; retry < 2; ++retry)
    {
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
        
        /* If number of seconds didn't change since last read then break the loop */
        if (cd.sec == getbcd(p, 0))
            break;
    }
    Enable();

    if (cd.year < 1978)
    	cd.year += 100;
    cd.wday = 0;
    t = Date2Amiga(&cd);
    D(bug("%02d:%02d %02d.%02d.%d = %d\n", cd.hour, cd.min, cd.mday, cd.month, cd.year, t));
    return t;

    AROS_LIBFUNC_EXIT

} /* ReadBattClock */
