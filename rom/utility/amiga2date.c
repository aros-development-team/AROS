/*
    $Id$
    $Log$
    Revision 1.2  1996/10/24 15:51:34  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/31 12:58:11  aros
    Merged in/modified for FreeBSD.

    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <clib/utility_protos.h>

        AROS_LH2(void, Amiga2Date,

/*  SYNOPSIS */
        AROS_LHA(unsigned long     , seconds, D0),
        AROS_LHA(struct ClockData *, result, A0),

/*  LOCATION */
        struct Library *, UtilityBase, 20, Utility)

/*  FUNCTION
        Convert the time value given as the number of seconds since the
        1st of January 1978 (00:00:00 1.1.78), to a more useful values,
        which is easier for most people to understand. These values will
        be stored in the ClockData structure whose address is passed as
        an argument.

    INPUTS
        seconds     -   Number of seconds since 1.1.78 00:00:00
        result      -   The ClockData structure to store the information
                        in.

    RESULT
        The ClockData structure will contain the converted time values.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Some information about some constants I use:

             731 =  365 + 366, the number of days between 1.1.1978 and
                    1.1.1976. Using 1976 makes working out leap years
                    simpler.
            1461 =  The number of days in four years including 1 leap year.
                    (eg 365*3 + 366)
           86400 =  The number of seconds in one day.

        I used these as constants so that they don't have to be computed
        on the fly, or read from variables.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        19-05-96    iaint   Wrote, with a little help from a Perl package.
        11-08-96    iaint   Updated for the new AROS format.
        17-08-96    iaint   Removed calls to unimplemented UDivMod32/UMult32

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const ULONG dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    ULONG days, temp, leap;

    days = seconds / 86400;
    result->wday = days % 7;

    /*
        using the number of days since 1.1.76 makes leap year calculations
        simpler.
    */

    days += 731;

    result->sec = seconds % 60;
    seconds /= 60;
    result->min = seconds % 60;
    seconds /= 60;
    result->hour= seconds % 24;

    /* Number of sets of four years since 1976 */
    temp = days / 1461;

    /* days since the beginning of the last leap year  */
    days %= 1461;

    temp = 1976 + (temp << 2);

    leap = (days <= 365);
    if(!leap) /* not a leap year */
    {
        temp++;
        days -= 366;
        result->year = temp + (days / 365);
        days %= 365;
    }

    /* days now contains the days since the beginning of the current year */
    for(temp = 0; (temp == 1) ? (days >= 28 + leap) : (days >= dim[temp]); temp++)
        days -= (temp == 1) ? (28 + leap) : dim[temp];

    result->month = temp;
    result->mday = days + 1;

    AROS_LIBFUNC_EXIT

} /* Amiga2Date */
