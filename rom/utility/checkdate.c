/*
    $Id$
    $Log$
    Revision 1.2  1997/01/27 00:32:30  ldp
    Polish

    Revision 1.1  1997/01/08 03:36:13  iaint
    A few more utility.lib functions

    Desc: CheckDate() - is a date valid ?
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH1(ULONG, CheckDate,

/*  SYNOPSIS */
        AROS_LHA(struct ClockData *, date, A0),

/*  LOCATION */
        struct Library *, UtilityBase, 22, Utility)

/*  FUNCTION
        Examine the date described in the ClockData structure and
        determine whether it is a valid date. In particular this
        checks whether the ranges of the fields are within normal
        limits.

    INPUTS
        date        -   A ClockData structure desribing the date
                        to check.

    RESULT
        If the date is valid, the number of seconds from midnight
        1-Jan-1978 AD to the date, or 0 if the date is invalud.

    NOTES
        The date 01-Jan-78 00:00:00 is actually returned as invalid.
        Also, unlike the AmigaOS v39 version, this function does check
        whether the weekday is within 0-6.

        This also assumes that the ClockDate refers to a date in the
        Gregorian calendar. (60 sec/min, 60 min/hour, 24 hr/day,
        12 months/year).

    EXAMPLE

    BUGS

    SEE ALSO
        Amiga2Date(), Date2Amiga()

    INTERNALS
	Since all the values are unsigned, we don't have to check for < 0.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Note: 60!!! This is in case of any future leap seconds... */
    if( date->sec > 60 )
        return 0;

    if( date->min > 59)
        return 0;

    if( date->hour > 23)
        return 0;

    /* XXX: When does the year become invalid? */
    if( date->year < 1978 )
        return 0;

    if( date->wday > 6)
        return 0;

    if( date->month > 11 )
        return 0;

    switch( date->month )
    {
        /* 30 days hath April, September, June and November */
        case 3:
        case 5:
        case 8:
        case 10:
            if(date->mday > 29)
                return 0;
            break;

        /* And all the rest have 31 ... */
        case 0:
        case 2:
        case 4:
        case 6:
        case 7:
        case 9:
        case 11:
            if(date->mday > 30)
                return 0;
            break;

        /* Except February with has 28, or 29 on a leap year. */
        case 1:
            if(date->mday > 28)
                return 0;
            break;

        /* This also traps invalid month numbers */
        default:
            return 0;
    } /* switch(date->month) */

    return Date2Amiga(date);

    AROS_LIBFUNC_EXIT
} /* CheckDate */
