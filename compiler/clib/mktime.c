/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Convert a broken-down time into calendar time.
*/


static char monthtable[] =
{
 /* JanFebMarAprMayJunJulAugSepOktNov */
    31,28,31,30,31,30,31,31,30,31,30
};

/*****************************************************************************

    NAME */
#include <time.h>

	time_t mktime (

/*  SYNOPSIS */
	struct tm * utim)

/*  FUNCTION
	The mktime() function converts the broken-down time utim to
	calendar time representation.

    INPUTS
	utim - The broken-down time to convert

    RESULT
	The converted calendar time

    NOTES

    EXAMPLE
	time_t	    tt;
	struct tm * tm;

        //Computation which results in a tm
        tm = ...

	// and convert it
	tt = mktime (tm);

    BUGS
        At the moment sanity check is not performed nor a normalization on the
        structure is done

    SEE ALSO
	time(), ctime(), asctime(), localtime(), gmtime()

    INTERNALS
	Rules for leap-years:

	1. every 4th year is a leap year

	2. every 100th year is none

	3. every 400th is one

	4. 1900 was none, 2000 is one

******************************************************************************/
{
    time_t           tt;
    int 	     leapyear,
		     days,
                     year,
                     i;

#warning TODO: Add struct tm normalization code here

    /* Compute number of days in the years before this year and after 1970.
     * 1972 is the first leapyear
     */
    year = utim->tm_year-1;
    days = 365*(year-69) + (year-68)/4 - year/100 + (year+300)/400;

    /* Add the day of the months before this month */
    for (i=0; i<utim->tm_mon; i++)
    {
        days += monthtable[i];
    }
    
    /* Is this a leapyear ? */
    year = utim->tm_year;
    leapyear = year%4==0 && (year%100!=0 || (year+300)%400==0);
    if (leapyear && utim->tm_mon>1) days++;

    /* Add day in the current month */
    days += utim->tm_mday - 1;

    tt = ( (days*24+utim->tm_hour)*60 + utim->tm_min )*60 + utim->tm_sec;

    return tt;
} /* mktime */
