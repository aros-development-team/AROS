/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WriteBattClock()
    Lang: english
*/
#include "battclock_intern.h"

static int rtas_call(const char *method, int nargs, int nret, void *output, ...);

#include <proto/battclock.h>
#include <proto/utility.h>
#include <proto/rtas.h>
#include <proto/exec.h>
#include <utility/date.h>
#include <stdarg.h>

/* See rom/battclock/writebattclock.c for documentation */

AROS_LH1(void, WriteBattClock,
    AROS_LHA(ULONG, time, D0),
    APTR *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    ULONG out[8];

    Amiga2Date(time, &date);

    rtas_call("set-time-of-day", 7, 1, NULL, (ULONG)date.year, (ULONG)date.month, (ULONG)date.mday, (ULONG)date.hour, (ULONG)date.min, (ULONG)date.sec, 0);

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */

static int rtas_call(const char *method, int nargs, int nret, void *output, ...)
{
	va_list args;
	void *RTASBase = OpenResource("rtas.resource");
	int retval;

	va_start(args, output);
	retval = RTASCall(method, nargs, nret, output, args);
	va_end(args);

	return retval;
}
