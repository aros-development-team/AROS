/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WriteBattClock()
    Lang: English
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <utility/date.h>

#include "battclock_intern.h"
#include "cmos.h"

static inline UBYTE MakeBCDByte(UBYTE n)
{
    return n / 10 << 4 | n % 10;
}

AROS_LH1(void, WriteBattClock,
	 AROS_LHA(ULONG, time, D0),
	 struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    UBYTE century;
    UWORD status_b;

    /* Convert time to the required format */

    Amiga2Date(time, &date);

    century = date.year / 100;
    date.year %= 100;

    ObtainSemaphore(&BattClockBase->sem);

    status_b = ReadCMOSByte(STATUS_B);

    if ((status_b & 0x04) == 0)
    {
        date.sec = MakeBCDByte(date.sec);
        date.min = MakeBCDByte(date.min);
        date.hour = MakeBCDByte(date.hour);
        date.mday = MakeBCDByte(date.mday);
        date.month = MakeBCDByte(date.month);
        date.year = MakeBCDByte(date.year);
        century = MakeBCDByte(century);
    }

    /* Write new time to the RTC */
    WriteCMOSByte(SEC, date.sec);
    WriteCMOSByte(MIN, date.min);
    WriteCMOSByte(HOUR, date.hour);
    WriteCMOSByte(MDAY, date.mday);
    WriteCMOSByte(MONTH, date.month);
    WriteCMOSByte(YEAR, date.year);
    WriteCMOSByte(CENTURY, century);

    ReleaseSemaphore(&BattClockBase->sem);

    AROS_LIBFUNC_EXIT
}
