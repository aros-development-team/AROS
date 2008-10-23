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


AROS_LH1(void, WriteBattClock,
         AROS_LHA(ULONG, time, D0),
         struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT
        
    SYSTEMTIME tm;
    struct ClockData date;
    
    Amiga2Date(time, &date);

    tm.wYear	  = date.year;
    tm.wMonth	  = date.month;
    tm.wDay	  = date.mday;
    tm.wHour	  = date.hour;
    tm.wMinute    = date.min;
    tm.wSecond    = date.sec;
    tm.wDayOfWeek = date.wday;

    BattClockBase->KernelIFace->SetSystemTime(&tm);

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
