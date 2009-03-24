/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: readbattclock.c 27752 2008-01-24 18:28:47Z jwegner $

    Desc: ReadBattClock() function.
    Lang: english
*/
#include "battclock_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battclock.h>
#include <proto/utility.h>
#include <proto/rtas.h>
#include <proto/exec.h>
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
    void *RTASBase = OpenResource("rtas.resource");
    ULONG secs;
    ULONG out[8];

	RTASCall("get-time-of-day", 0, 8, out, NULL);

    date.year  = out[0];
    date.month = out[1];
    date.mday  = out[2];
    date.hour  = out[3];
    date.min   = out[4];
    date.sec   = out[5];

    secs=Date2Amiga(&date);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
