/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "locale_intern.h"
#include <exec/types.h>
#include <utility/hooks.h>
#include <utility/date.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <stdio.h>

VOID PrintDigits(UWORD number, char fill, UWORD len, struct Hook *hook,
		 struct Locale *locale);
VOID _WriteChar(char token, struct Hook *hook, struct Locale *locale);
VOID _WriteString(STRPTR string, struct Hook *hook, struct Locale *locale);

static const ULONG dayspermonth[13] = {0 /* not used */,0,31,59,90,120,151,181,212,243,273,304,334};

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(VOID, FormatDate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale    *, locale, A0),
	AROS_LHA(STRPTR            , formatString, A1),
	AROS_LHA(struct DateStamp *, date, A2),
	AROS_LHA(struct Hook      *, hook, A3),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 10, Locale)

/*  FUNCTION

    Generate a date string based on a template. The bytes generated are sent
    to a user specified callback function.

    INPUTS

    locale        --  the locale to use when formatting the string
    formatString  --  the formatting template string; this is much like the
                      printf() formatting style, i.e. a % followed by a
		      formatting command. The following commands exist:

		      %a -- abbreviated weekday name
		      %A -- weekday name
		      %b -- abbreviated month name
		      %B -- month name
		      %c -- the same as "%a %b %d %H:%M:%S %Y"
		      %C -- the same as "%a %b %e %T %Z %Y"
		      %d -- day number with leading zeros
		      %D -- the same as "%m/%d/%y"
		      %e -- day number with leading spaces
		      %h -- abbreviated month name
		      %H -- hour using 24 hour style with leading zeros
		      %I -- hour using 12 hour style with leading zeros
		      %j -- julian date
		      %m -- month number with leading zeros
		      %M -- the number of minutes with leading zeros
		      %n -- linefeed
		      %p -- AM or PM string
		      %q -- hour using 24 hour style
		      %Q -- hour using 12 hour style
		      %r -- the same as "%I:%M:%S %p"
		      %R -- the same as "%H:%M"
		      %S -- the number of seconds with leading zeros
		      %t -- tab
		      %T -- the same as "%H:%M:%S"
		      %U -- the week number, taking Sunday as the first day
		            of the week
		      %w -- the weekday number
		      %W -- the week number, taking Monday as the first day
		            of the week
		      %x -- the same as "%m/%d/%y"
		      %X -- the same as "%H:%M:%S"
		      %y -- the year using two digits with leading zeros
		      %Y -- the year using four digits with leading zeros

		      If the template parameter is NULL, a single null byte
		      is sent to the callback function.

    date          --  the current date
    hook          --  callback function; this is called for every character
                      generated with the following arguments:

		      * pointer to hook structure
		      * character
		      * pointer to locale

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    ParseDate(), <libraries/locale.h>

    INTERNALS

    HISTORY

    17.01.2000  bergers implemented U & W, j is still missing
    07.07.1999  SDuvan  implemented (U, W, j not done yet)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClockData cData;
    ULONG week, days, tmp;

    if(/* locale == NULL || */ hook == NULL)
	return;

    if(formatString == NULL)
    {
	_WriteChar(0, hook, locale);
	return;
    }

#warning Amiga2Date will fail around year 2114, because then the numer of seconds since 1978 dont fit in a 32 bit variable anymore!
   
    Amiga2Date(date->ds_Days*86400 + date->ds_Minute*60 + date->ds_Tick / 50,
	       &cData);
    
    while(*formatString != 0)
    {
    	if(*formatString == '%')
	{
	    switch(*(++formatString))
	    {
	    case 'a':
		_WriteString(GetLocaleStr(locale, ABDAY_1 + cData.wday), hook,
			     locale);
		break;
		
	    case 'A':
		_WriteString(GetLocaleStr(locale, DAY_1 + cData.wday), hook,
			     locale);
		break;
		
	    case 'b':
		_WriteString(GetLocaleStr(locale, ABMON_1 + cData.month - 1),
			    hook, locale);
		break;
		
	    case 'B':
		_WriteString(GetLocaleStr(locale, MON_1 + cData.month - 1), hook,
			     locale);
		break;
		
	    case 'c':
		FormatDate(locale, "%a %b %d %H:%M:%S %Y", date, hook);
		break;
		
	    case 'C':
		FormatDate(locale, "%a %b %e %T %Z %Y", date, hook);
		break;
		
	    case 'd':
		PrintDigits(cData.mday, '0', 2, hook, locale);
		break;
		
	    case 'x':
	    case 'D':
		FormatDate(locale, "%m/%d/%y", date, hook);
		break;
		
	    case 'e':
		PrintDigits(cData.mday, ' ', 2, hook, locale);
		break;
		
	    case 'h':
		_WriteString(GetLocaleStr(locale, ABMON_1 + cData.month - 1),
			     hook, locale);
		break;
		
	    case 'H':
		PrintDigits(cData.hour, '0', 2, hook, locale);
		break;
		
	    case 'I':
		PrintDigits(cData.hour % 12, '0', 2, hook, locale);
		break;
		
	    case 'j':
	        /* TODO */
#warning Julian date not tested.
		/* Julian date is DDD (1 - 366)*/
		PrintDigits(
		    cData.mday + dayspermonth[cData.month],
		    '0',
		    3,
		    hook,
		    locale
		);
		break;
		
	    case 'm':
		PrintDigits(cData.month, '0', 2, hook, locale);
		break;
		
	    case 'M':
		PrintDigits(cData.min, '0', 2, hook, locale);
		break;
		
	    case 'n':
		_WriteChar('\n', hook, locale);
		break;
		
	    case 'p':
		_WriteString(GetLocaleStr(locale,
					  cData.hour < 12 ? AM_STR : PM_STR),
			     hook, locale);
		break;
		
	    case 'q':
		PrintDigits(cData.hour, -1, 2, hook, locale);
		break;
		
	    case 'Q':
		PrintDigits(cData.hour % 12, -1, 2, hook, locale);
		break;
		
	    case 'r':
		FormatDate(locale, "%I:%M:%S %p", date, hook);
		break;
		
	    case 'R':
		FormatDate(locale, "%H:%M", date, hook);
		break;
		
	    case 'S':
		PrintDigits(cData.sec, '0', 2, hook, locale);
		break;
		
	    case 't':
		_WriteChar('\t', hook, locale);
		break;
		
	    case 'X':
	    case 'T':
		FormatDate(locale, "%H:%M:%S", date, hook);
		break;
		
	    case 'W': /* week number, Monday first day of week */
	    case 'U': /* week number, Sunday first day of week */
	        days = cData.mday + dayspermonth[cData.month]; 
		
		/* leap year ? */
		if (0 == (cData.year % 4) && cData.month > 2)
		{
		  /*
		  ** 1700, 1800, 1900, 2100, 2200 re not leap years.
		  ** 2000 is a leap year.
		  ** -> if a year is divisible by 100 but not by 400 then
		  ** it is not a leap year!
		  */
		  if (0 == (cData.year % 100) && 0 != (cData.year % 400))
		    ;
		  else
		    days++;
		}
		
		/* 
		** If January 1st is a Monday then the first week
		** will start with a Sunday January 7th if Sunday is the first day of the week
		** but if Monday is the first day of the week then Jan 1st will also be the
		** first day of the first week.
		*/
		/* 
		** Go to Saturday = last day of week if Sunday is first day of week
                ** Go to Sunday   = last day of week if Monday is first day of week
		*/
		if ('U' == *formatString)
		{
		  /* Sunday is first day of the week */
  		  tmp = days + (6 - cData.wday);
  		}
  		else
  		{
  		  /* Monday is first day of week */
  		  if (0 != cData.wday)
  		    tmp = days + (7 - cData.wday);
  		  else
  		    tmp = days;
		}

		if (tmp < 7)
		  week = 0;
		else
		{
		  /* cut off the few days that belong to week 0 */
		  tmp -= (tmp % 7);
		  /* Calculate the full amount of weeks */
		  week = tmp / 7;
		}
		
                PrintDigits(week, '0', 2, hook, locale);
	    break;
		
	    case 'w':
		PrintDigits(cData.wday, -1, 1, hook, locale);
		break;
		
	    case 'y':
		PrintDigits(cData.year % 100, '0', 2, hook, locale);
		break;
		
	    case 'Y':
		PrintDigits(cData.year, '0', 4, hook, locale);
		break;

            case 'Z':
                /* cuurent time zone Unimplemented in 3.1 */
                break;
		
	    case 0:
		break;
		
	    default:
		_WriteChar(*formatString, hook, locale);
		break;
	    }
	}
	else
	{
	    _WriteChar(*formatString, hook, locale);
	}
	
	formatString++;
    }

    _WriteChar(0, hook, locale);	/* Write null terminator */

    AROS_LIBFUNC_EXIT
} /* FormatDate */


VOID _WriteString(STRPTR string, struct Hook *hook, struct Locale *locale)
{
    while(*string != 0)
    {
	_WriteChar(*string++, hook, locale);
    }
}


VOID _WriteChar(char token, struct Hook *hook, struct Locale *locale)
{
     AROS_UFC3(VOID, hook->h_Entry,
       AROS_UFCA(struct Hook *,   hook,   A0),
       AROS_UFCA(struct Locale *, locale, A2),
       AROS_UFCA(char,            token,  A1)
     );
}


VOID PrintDigits(UWORD number, char fill, UWORD len, struct Hook *hook,
		 struct Locale *locale)
{
    char  buf[7];
    char *ptr = &buf[6];
    int   i   = 0;
    
    buf[6] = 0;
    
    while((number || !i) && i < len)
    {
	*--ptr = number % 10 + '0';
	number /= 10;
	i++;
    }

    while(len - i > 0  && (char)-1 != fill)
    {
      len--;
      _WriteChar(fill, hook, locale);
    }

    _WriteString((char *)ptr, hook, locale);
}
