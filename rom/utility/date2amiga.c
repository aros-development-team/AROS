/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert a human understandable date to a machine form.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <utility/date.h>
#include <proto/utility.h>

	AROS_LH1(ULONG, Date2Amiga,

/*  SYNOPSIS */
	AROS_LHA(struct ClockData *, date, A0),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 21, Utility)

/*  FUNCTION
	Converts the information given in the struct ClockData *date, into
	the number of seconds that have past since the 1st of January 1978.

    INPUTS
	date	-   Contains the information about the time.

    RESULT
	The number of seconds since 1.1.1978

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	Amiga2Date(), CheckDate()

    INTERNALS
	Bit of a hack in the leap year handling.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* This array contains the number of days that have been in the year
       up to the start of the month. Does not take into account leap years.
    */
    static const UWORD dayspermonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    ULONG time;
    UWORD year, leaps;

    year = date->year - 1978;

    time = date->sec + (date->min * 60) + (date->hour * 3600);
    time += ((date->mday - 1) + dayspermonth[date->month - 1]) * 86400;
    time += year * 31536000;

    /* How do we calculate leap years? That is a very good question.
       The year 1978 is NOT a leap year, but 1980 is...

       I want to arrange it so that the last year of any four year group
       is a leap year. So the first year I can really deal with is either
       1977 or 1981. So I choose 1977 (the year I was born in :)

	Then, to get the number of leap years I divide by 4.
	However, if year is a year which is a leap year, then this year
	will not be counted, so I have to check for this.

	If year % 4 == 3, then we are in a leap year, and if the month
	is after February, then I can add a leap year.
    */

#if 1
    /* stegerg: based on dos.library/strtodate */
    
    year = date->year;

    if (((year % 400) == 0) || (((year % 4) == 0) && !((year % 100) == 0)))
    {
    	/* data->year is a leap year. So dayspermonth table was wrong if
	   month >= March */
	   
	if (date->month >= 3) time += 86400;
    }
    
    year--;
    
    leaps = ((year / 4) - (year / 100) + (year / 400) - (494 - 19 + 4));

    time += leaps * 86400;
    
#else
    year++;
    leaps = year / 4;
    if( (year % 4 == 3) && (date->month > 2))
	leaps++;

    /* If the year is greater than the year 2100, or it is the
       year 2100 and after February, then we also have to subtract
       a leap year, as the year 2100 is NOT a leap year.
    */
    if( ( year > 123) || ((year == 123) && (date->month > 2)) )
	leaps--;

    time += leaps * 86400;
#endif

    return time;

    AROS_LIBFUNC_EXIT
} /* Date2Amiga */
