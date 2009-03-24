/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id: writebattclock.c 26020 2007-05-07 19:49:07Z verhaegs $

    Desc: WriteBattClock()
    Lang: english
*/
#include "battclock_intern.h"

int rtas_call(const char *method, int nargs, int nret, void *output, ...);

/*****************************************************************************

    NAME */
#include <proto/battclock.h>
#include <proto/utility.h>
#include <proto/rtas.h>
#include <proto/exec.h>
#include <utility/date.h>
#include <stdarg.h>

	AROS_LH1(void, WriteBattClock,

/*  SYNOPSIS */
	AROS_LHA(ULONG, time, D0),

/*  LOCATION */
	APTR *, BattClockBase, 3, Battclock)

/*  FUNCTION
	Set the systems battery backed up clock to the time specified. The
	value should be the number of seconds since 00:00:00 on 1.1.1978.

    INPUTS
	time    -   The number of seconds elapsed since 00:00:00 1.1.1978

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

    HISTORY
	27-11-96    digulla automatically created from
			    battclock_lib.fd and clib/battclock_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    ULONG out[8];

    Amiga2Date(time, &date);

    rtas_call("set-time-of-day", 7, 1, NULL, date.year, date.month, date.mday, date.hour, date.min, date.sec);

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */

int rtas_call(const char *method, int nargs, int nret, void *output, ...)
{
	va_list args;
	void *RTASBase = OpenResource("rtas.resource");
	int retval;

	va_start(args, output);
	retval = RTASCall(method, nargs, nret, output, args);
	va_end(args);

	return retval;
}
