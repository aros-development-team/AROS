/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Convert the date from machine to human form.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/date.h>
#include <proto/utility.h>

	AROS_LH2(void, Amiga2Date,

/*  SYNOPSIS */
	AROS_LHA(ULONG             , seconds, D0),
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
	seconds     -	Number of seconds since 1.1.78 00:00:00
	result	    -	The ClockData structure to store the information
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
	24-02-97    iaint   Reimplemented, actually works now :)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    static const ULONG dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    ULONG days;
    UWORD leap, temp, year;

    days = seconds / 86400;
    result->wday = days % 7;

    result->sec = seconds % 60;
    seconds /= 60;
    result->min = seconds % 60;
    seconds /= 60;
    result->hour = seconds % 24;

    /*	Calculate the current year.

	Firstly, if the year is less than 1980, then the leap year
	handling is not required...

    */
    if(days < 1096)
    {
	result->year = 1978;

	if(days > 729)
	    leap = TRUE;
	else
	    leap = FALSE;

	year = (days / 365);
	days = days - (year * 365);
    }
    else
    {
	/*
	    We need to get into a year that follows a leap year, there
	    are two cases, >2100 and <=2100

	    If the year is after 2100, which is not a leap year, then
	    start point is 2101.

	    The first day in year 2101 is ...
	*/
	if(days > 44925)
	{
	    days -= 44926;
	    result->year = 2101;
	}
	/*
	    Otherwise, we just set everything up so that we are relative
	    to 1981.
	*/
	else
	{
	    result->year = 1981;
	    days -= 1096;
	}

	/*
	    From here, we know that every remaining set of 4 years
	    has 1 leap year...
	*/
	year = days / 1461;
	days -= year * 1461;
	result->year += year * 4;

	if(days > 1095)
	    leap = TRUE;
	else
	    leap = FALSE;

	year = days / 365;
	days -= year * 365;

	/* Now days is the number of days in the current year... */
    } /* (not less than 1981) */

    /* days now contains the days since the beginning of the current year */
    for(temp = 0; (temp == 1) ? (days >= 28 + leap) : (days >= dim[temp]); temp++)
	days -= (temp == 1) ? (28 + leap) : dim[temp];

    result->month = temp + 1;
    result->mday = days + 1;
    result->year += year;

    AROS_LIBFUNC_EXIT

} /* Amiga2Date */
