/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WriteBattClock()
    Lang: English
*/
#include "battclock_intern.h"
#include "cmos.h"

static UBYTE ReadCMOSByte(UBYTE port);
static VOID WriteCMOSByte(UBYTE port, UBYTE value);
static UBYTE MakeBCDByte(UBYTE n);


/*****************************************************************************

    NAME */
#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>

	AROS_LH1(void, WriteBattClock,

/*  SYNOPSIS */
	AROS_LHA(ULONG, time, D0),

/*  LOCATION */
	APTR *, BattClockBase, 3, Battclock)

/*  FUNCTION
	Set the system's battery backed up clock to the time specified. The
	value should be the number of seconds since 00:00:00 on 1.1.1978.

    INPUTS
	time - The number of seconds elapsed since 00:00:00 1.1.1978

    RESULT
	The clock will be set.

    NOTES
	This may not do anything on some systems where the battery backed
	up clock either doesn't exist, or may not be writable.

    EXAMPLE

    BUGS

    SEE ALSO
	ReadBattClock, ResetBattClock

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    UBYTE century;
    UWORD status_b;

    /* Convert time to the required format */

    Amiga2Date(time, &date);

    century = date.year / 100;
    date.year %= 100;

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

    AROS_LIBFUNC_EXIT
}

static UBYTE ReadCMOSByte(UBYTE port)
{
    UBYTE value;

    asm volatile("outb	%0,$0x70" :: "a"(port));
    asm volatile("inb	$0x71,%0" : "=a"(value));

    return value;
}

static VOID WriteCMOSByte(UBYTE port, UBYTE value)
{
    asm volatile("outb	%0,$0x70" :: "a"(port));
    asm volatile("outb	%0,$0x71" :: "a"(value));

    return;
}

static UBYTE MakeBCDByte(UBYTE n)
{
    return n / 10 << 4 | n % 10;
}
