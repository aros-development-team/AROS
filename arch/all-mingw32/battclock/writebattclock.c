/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: readbattclock.c 21132 2004-02-29 22:06:29Z stegerg $

    Desc: ReadBattClock() function.
    Lang: english
*/

#define DEBUG 0

#include "battclock_intern.h"
#include <aros/debug.h>
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
    
    D(bug("[Battclock] WriteBattClock()\n"));
    
    Amiga2Date(time, &date);

    tm.wYear	  = date.year;
    tm.wMonth	  = date.month;
    tm.wDay	  = date.mday;
    tm.wHour	  = date.hour;
    tm.wMinute    = date.min;
    tm.wSecond    = date.sec;
    /* Day of week is just informative and can be ignored */

    Forbid();
    BattClockBase->KernelIFace->SetSystemTime(&tm);
    Permit();

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
