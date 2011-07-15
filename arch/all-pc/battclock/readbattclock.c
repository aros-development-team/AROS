/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <utility/date.h>

#include "battclock_intern.h"
#include "cmos.h"

static inline int bcd_to_dec(int x)
{
    return ( (x >> 4) * 10 + (x & 0x0f) );
}

AROS_LH0(ULONG, ReadBattClock,
	 struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    UWORD century;
    UWORD status_b;
    ULONG secs;

    ObtainSemaphore(&BattClockBase->sem);

    /* Make sure time isn't currently being updated */
    while ((ReadCMOSByte(STATUS_A) & 0x80) != 0);

    date.sec   = ReadCMOSByte(SEC);
    date.min   = ReadCMOSByte(MIN);
    date.hour  = ReadCMOSByte(HOUR);
    date.mday  = ReadCMOSByte(MDAY);
    date.month = ReadCMOSByte(MONTH);
    date.year  = ReadCMOSByte(YEAR);
    century    = ReadCMOSByte(CENTURY);
    status_b   = ReadCMOSByte(STATUS_B);

    ReleaseSemaphore(&BattClockBase->sem);

    if ((status_b & 0x04) == 0) {
	date.sec   = bcd_to_dec(date.sec);
	date.min   = bcd_to_dec(date.min);
	date.hour  = bcd_to_dec(date.hour);
	date.mday  = bcd_to_dec(date.mday);
	date.month = bcd_to_dec(date.month);
	date.year  = bcd_to_dec(date.year);
	century    = bcd_to_dec(century);
    }

    date.year = century * 100 + date.year;

    secs=Date2Amiga(&date);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
