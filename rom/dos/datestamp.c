/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#define timeval sys_timeval
#include <sys/time.h>
#undef timeval
#include <unistd.h>
#include "dos_intern.h"

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
	The original function could DateStamp() only returns even multiples
	of 50 ticks.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
#define SECONDS_PER_DAY     (60UL * 60 * 24)
#define SECONDS_PER_MINUTE  (60UL)
#define uSEC_PER_SEC	    (1000000UL)
#define TICKS_PER_SEC	    (50UL)
#define uSEC_PER_TICK	    (uSEC_PER_SEC / TICKS_PER_SEC)
#define AMIGA_UNIX_OFFSET   (2922UL) /* Days between 1.1.78 (Amiga) and 1.1.70 (Unix) */
    struct sys_timeval stv;

    gettimeofday (&stv, NULL);

    stv.tv_sec =

    date->ds_Days = stv.tv_sec / SECONDS_PER_DAY - AMIGA_UNIX_OFFSET;
    stv.tv_sec %= SECONDS_PER_DAY;
    date->ds_Minute = stv.tv_sec / SECONDS_PER_MINUTE;
    stv.tv_sec %= SECONDS_PER_MINUTE;
    date->ds_Tick = (stv.tv_usec + stv.tv_sec * uSEC_PER_SEC) /
	    uSEC_PER_TICK;

    return date;
    AROS_LIBFUNC_EXIT
} /* DateStamp */
