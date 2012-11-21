/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DateStamp() - Get the current date.
    Lang: english
*/
#include <devices/timer.h>
#include <proto/timer.h>
#include "dos_intern.h"

#define SECONDS_PER_DAY     (60UL * 60 * 24)
#define SECONDS_PER_MINUTE  (60UL)
#define uSEC_PER_SEC        (1000000UL)
#define TICKS_PER_SEC       (50UL)
#define uSEC_PER_TICK       (uSEC_PER_SEC / TICKS_PER_SEC)

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(struct DateStamp *, DateStamp,

/*  SYNOPSIS */
        AROS_LHA(struct DateStamp *, date, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 32, Dos)

/*  FUNCTION
        Fills the structure with the current time. Time is measured from
        Jan 1, 1978.

    INPUTS
        date - The structure to fill.

    RESULT
        date->ds_Days is filled with the days from Jan 1, 1978.
        date->ds_Minute is filled with the number of minutes elapsed in the
        day. date->ds_Tick is the number of ticks elapsed in the current
        minute. A tick happens 50 times a second. DateStamp() ensures that
        the day and minute are consistent. All three elements are zero if
        the date is unset.

    NOTES
        The original function could only return even multiples of 50 ticks.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* We get the date from the timer.device before splitting it up */
    struct timeval tv;
    GetSysTime(&tv);

    date->ds_Days = tv.tv_secs / SECONDS_PER_DAY;
    tv.tv_secs %= SECONDS_PER_DAY;
    date->ds_Minute = tv.tv_secs / SECONDS_PER_MINUTE;
    tv.tv_secs %= SECONDS_PER_MINUTE;
    date->ds_Tick = (tv.tv_micro + tv.tv_secs * uSEC_PER_SEC) /
            uSEC_PER_TICK;

    return date;
    AROS_LIBFUNC_EXIT
} /* DateStamp */
