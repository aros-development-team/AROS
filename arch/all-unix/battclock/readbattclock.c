/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/

#include "battclock_intern.h"
#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>
#if 0
#include <aros/host-conf.h>
#endif

#include <time.h>


AROS_LH0(ULONG, ReadBattClock, struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    /*
	This is mostly an example.
	It is quite possible that this time value is not for the local
	timezone, so it has to be converted.
    */
    time_t t;
    struct tm *tm;

    time(&t);
    tm = localtime(&t);

#if 1
    {
    	struct ClockData date;
	
	date.year  = tm->tm_year + 1900;
	date.month = tm->tm_mon + 1;
	date.mday  = tm->tm_mday;
	date.hour  = tm->tm_hour;
	date.min   = tm->tm_min;
	date.sec   = tm->tm_sec;
	
	return Date2Amiga(&date);
    }
    
#else
    /*
	This time is however relative to 1.1.1970, so we have to subtract a
	large number of seconds so that things actually work correctly.

	There is a problem here that 8 years before the Amiga clock dies,
	the Unix system clock will have problems as it will wrap around.
	Still, I'll be dead, and by then they won't be using 32-bit clocks
	I expect...
    */
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    return (t - 252460800 + tm->tm_gmtoff);
#else
    return (t - 252460800);
#endif
#endif

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
