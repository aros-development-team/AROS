/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into UTC, reentrant.
*/

/* At the moment no daylight saving time information
 * Implementation has to be changed when DST is implemented in AROS
 */
int __dstflag = -1;

static char monthtable[] =
{
 /* JanFebMarAprMayJunJulAugSepOktNov */
    31,29,31,30,31,30,31,31,30,31,30
};

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * gmtime_r (

/*  SYNOPSIS */
	const time_t * tt,
        struct tm * tm)

/*  FUNCTION
	The gmtime_r() function converts the calendar time tt to
	broken-down time representation, expressed in Coordinated Universal
	Time (UTC).


    INPUTS
	tt - The time to convert
        tm - A struct tm to store the result in

    RESULT
	The pointer passed in tm, containing the broken down time in
        Coordinated Universal Time (UTC).

    NOTES

    EXAMPLE
	time_t	  tt;
	struct tm tm;

	// Get the time
	time (&tt);

	// and convert it
	gmtime (&tt, &tm);

    BUGS

    SEE ALSO
	time(), ctime_r(), asctime_r(), localtime_r()

    INTERNALS
	Rules for leap-years:

	1. every 4th year is a leap year

	2. every 100th year is none

	3. every 400th is one

	4. 1900 was none, 2000 is one

******************************************************************************/
{
    signed long      tim;
    int 	     leapday  = 0,
		     leapyear = 0,
		     i;

    tim = *tt;

    tm->tm_sec = tim % 60;
    tim /= 60;

    tm->tm_min = tim % 60;
    tim /= 60;

    /*
	719162 number of days between 1.1.1 and 1.1.1970 if the calendar
	would go so far which it doesn't :-) this is true for all of the
	following.
    */
    tm->tm_hour = tim % 24;
    tim = tim / 24 + 719162;

    tm->tm_wday = (tim + 1) % 7;

    /* 146097 number of days from 1.1.1 to 1.1.401 */
    tm->tm_year = tim / 146097 * 400 - 1899;
    tim %= 146097;

    /* 145731 number of days from 1.1.1 to 1.1.400 */
    if (tim >= 145731)
    {
	leapyear ++; /* The day is in one of the 400th */

	/* Be careful: The last of the 4 centuries is 1 day longer */
	if (tim == 146096)
	{
	    tim --;
	    leapday ++;
	}
    }

    /* 36524 number of days from 1.1.1 to 1.1.101 */
    tm->tm_year += tim / 36524 * 100;
    tim %= 36524;

    /* 36159 number of days from 1.1.1 to 1.1.100 */
    if (tim >= 36159)
	leapyear --; /* The day is in one of the 100th */

    /* 1461 number of days from 1.1.1 to 1.1.5 */
    tm->tm_year += tim / 1461 * 4;
    tim %= 1461;

    /* 1095 number of days from 1.1.1 to 1.1.4 */
    if (tim >= 1095)
    {
	leapyear ++; /* The day is in one of the 4th */

	/* Be careful: The 4th year is 1 day longer */
	if (tim == 1460)
	{
	    tim --;
	    leapday ++;
	}
    }

    /* 365 days in a normal year */
    tm->tm_year += tim / 365;
    tim = tim % 365 + leapday;

    tm->tm_yday = tim;

    if (!leapyear && tim >= 31+28)
	tim ++; /* add 1 for 29-Feb if no leap year */

    /* Find the month */
    for (i=0; i<11; i++)
    {
	if (tim < monthtable[i])
	    break;

	tim-=monthtable[i];
    }

    tm->tm_mon = i;
    tm->tm_mday = tim + 1;

    tm->tm_isdst = __dstflag;

    return tm;
} /* gmtime */
