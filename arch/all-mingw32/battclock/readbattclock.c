/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: readbattclock.c 21132 2004-02-29 22:06:29Z stegerg $

    Desc: ReadBattClock() function.
    Lang: english
*/

#include "battclock_intern.h"
#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>

AROS_LH0(ULONG, ReadBattClock, struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT
        
    SYSTEMTIME tm;
    struct ClockData date;
    
    Forbid();
    BattClockBase->KernelIFace->GetSystemTime(&tm);
    Permit();
    date.year  = tm.wYear;
    date.month = tm.wMonth;
    date.mday  = tm.wDay;
    date.hour  = tm.wHour;
    date.min   = tm.wMinute;
    date.sec   = tm.wSecond;
    /* Day of week is just informative and can be ignored */
    
    return Date2Amiga(&date);

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
