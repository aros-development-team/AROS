/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert the date from machine to human form.
    Lang: english
*/
#include "intern.h"

#if 1
const ULONG Utility_DayTable[]=
{
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};
#endif

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
    LONG days;
    LONG leap, temp, year, month;

    days = seconds / 86400;
    result->wday = days % 7;

    result->sec = seconds % 60;
    seconds /= 60;
    result->min = seconds % 60;
    seconds /= 60;
    result->hour = seconds % 24;

#if 1
    /* stegerg: based on dos.library/datetostr */
    
    leap = 1;
    
    if(days<92*365+30*366)
    {
	/*
	    1976 was a leap year so use it as a base to divide the days
	    into 4-year blocks (each beginning with a leap year).
	*/
	days+=366+365;
	year=4*(days/(366+3*365))+1976;
	days%=(366+3*365);

	/* Now divide the 4-year blocks into single years. */
	if (days>=366)
	{
	    leap=0;
	    days--;
	    year+=days/365;
	    days%=365;
	}
    }
    else
    {
	/*
	    The rule for calendar calculations are:
	    1. Every year even divisible by 4 is a leap year.
	    2. As an exception from rule 1 every year even divisible by
	       100 is not.
	    3. Every year even divisible by 400 is a leap year as an
	       exception from rule 2.
	    So 1996, 2000 and 2004 are leap years - 1900 and 1999 are not.

	    Use 2000 as a base to devide the days into 400 year blocks,
	    those into 100 year blocks and so on...
	*/
	days-=17*365+5*366;
	year=400*(days/(97*366+303*365))+2000;
	days%=(97*366+303*365);

	if(days>=366)
	{
	    leap=0;
	    days--;
	    year+=100*(days/(24*366+76*365));
	    days%=(24*366+76*365);

	    if(days>=365)
	    {
		leap=1;
		days++;
		year+=4*(days/(366+3*365));
		days%=(366+3*365);

		if(days>=366)
		{
		    leap=0;
		    days--;
		    year+=days/365;
		    days%=365;
		}
	    }
	}
    }
    /*
	 The starting-day table assumes a leap year - so add one day if
	 the date is after february 28th and the year is no leap year.
    */
    if(!leap&&days>=31+28)
	days++;

    for(month=11;;month--)
    {
	if(days>=Utility_DayTable[month])
	{
	    days-=Utility_DayTable[month];
	    break;
	}
    }

    /* Remember that 0 means 1.1.1978. */
    days++;
    month++;
    
    result->month = month;
    result->mday = days;
    result->year = year;
    
#else

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
#endif

    AROS_LIBFUNC_EXIT

} /* Amiga2Date */
