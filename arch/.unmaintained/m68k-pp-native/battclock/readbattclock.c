/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/
#include "battclock_intern.h"
#include <asm/registers.h>

/*****************************************************************************

    NAME */
#include <proto/battclock.h>
#include <proto/utility.h>
#include <utility/date.h>

	AROS_LH0(ULONG, ReadBattClock,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct BattClockBase *, BattClockBase, 2, Battclock)

/*  FUNCTION
	Return the value stored in the battery back up clock. This value
	is the number of seconds that have elapsed since midnight on the
	1st of January 1978 (00:00:00 1.1.1978).

	If the value of the battery clock is invalid, then the clock will
	be reset.

    INPUTS

    RESULT
	The number of seconds since 1.1.1978 00:00:00

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WriteBattClock, ResetBattClock

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    battclock_lib.fd and clib/battclock_protos.h

*****************************************************************************/
{
	AROS_LIBFUNC_INIT

	struct ClockData date;		
	ULONG  secs;
	ULONG rtctime = RREG_L(RTCTIME);

	date.sec  = (rtctime & SECONDS_M);
	date.min  = (rtctime & MINUTES_M) >> 16;
	date.hour = (rtctime & HOURS_M  ) >> 24;

	date.mday  = 0;
	date.month = 0;
	date.year  = 0;

	secs = Date2Amiga(&date);

	return secs;

	AROS_LIBFUNC_EXIT
} /* ReadBattClock */
