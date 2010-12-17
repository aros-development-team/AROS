/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/

#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>

#include "battclock_intern.h"

AROS_LH0(ULONG, ReadBattClock, struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    time_t t;
    struct tm *tm;
    struct ClockData date;

    BattClockBase->SysIFace->time(&t);
    AROS_HOST_BARRIER

    /*
     * On UNIX time() returns local time instead of GMT.
     * So we have all this clutter. Note that we use host function
     * in order to convert to GMT while we could in fact use AROS one.
     * This is because theoretically AROS and our host can be set
     * to different time zones. Additionally we use host OS includes
     * for definitions of struct tm and time_t.
     */
    tm = BattClockBase->SysIFace->localtime(&t);
    AROS_HOST_BARRIER

    date.year  = tm->tm_year + 1900;
    date.month = tm->tm_mon + 1;
    date.mday  = tm->tm_mday;
    date.hour  = tm->tm_hour;
    date.min   = tm->tm_min;
    date.sec   = tm->tm_sec;

    return Date2Amiga(&date);

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
