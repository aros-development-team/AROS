/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include "locale_intern.h"

UWORD monthdays[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(BOOL, ParseDate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale    *, locale, A0),
	AROS_LHA(struct DateStamp *, date, A1),
	AROS_LHA(STRPTR            , fmtTemplate, A2),
	AROS_LHA(struct Hook      *, getCharFunc, A3),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 27, Locale)

/*  FUNCTION
	This function will convert a stream of characters into an AmigaDOS
	DateStamp structure. It will obtain its characters from the
	getCharFunc callback hook, and the given formatting template will
	be used to direct the parse.

    INPUTS
	locale      -   the locale to use for the formatting
	date        -   where to put the converted date. If this is NULL,
			then this function can be used to verify a date
			string.
	fmtTemplate -   the date template used to direct the parse of the
			data. The following FormatDate() formatting
			controls can be used:
			  %a %A %b %B %d %e %h %H %I %m %M %p %S %y %Y

			See FormatDate() autodoc for more information.
	getCharFunc -   A callback Hook which is used to read the data
			from a stream. The hook is called with:

			A0 - address of the Hook structure
			A2 - locale pointer
			A1 - NULL

    	    	    	BTW: The AmigaOS autodocs which state that A1
			gets locale pointer and A2 NULL are wrong!!
			
			The read character should be returned in D0. Note
			that this is a 32 bit character not an 8 bit
			character. Return a NULL character if you reach the
			end of the stream.

    RESULT
	TRUE    -   If the parse could be performed.
	FALSE   -   If the format of the data did not match the formatting
		    string.

    NOTES
	This has a few differences from the implementation in locale.library
	v38. In particular:
	    - %p does not have to be at the end of the line.
	    - %d and %e are not effectively the same, ie %d requires a leading
	      zero, but %e can not handle leading 0's.

    EXAMPLE

    BUGS
	%d, %e probably needs some work.

    SEE ALSO
	FormatDate()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LocaleBase *,LocaleBase)

    ULONG c;
    LONG day = 0, month = 0, hour = 0, min = 0, sec = 0;
    LONG year = 1978;
    BOOL ampm = FALSE, checkEOF = TRUE;

    if(    (fmtTemplate == NULL)
	|| (getCharFunc == NULL)
	|| (locale == NULL)
	|| (*fmtTemplate == NULL)
      )
	return FALSE;

#define GetChar()\
	c = AROS_UFC3(ULONG, getCharFunc->h_Entry, \
		AROS_UFCA(struct Hook *, getCharFunc, A0), \
		AROS_UFCA(struct Locale *, locale, A2), \
		AROS_UFCA(ULONG, NULL, A1)) 

    while(*fmtTemplate)
    {
	/* Check for EOF if we leave the loop */
	checkEOF = TRUE;

	if(*fmtTemplate == '%')
	{
	    UBYTE strOffs = 0;
	    fmtTemplate++;

	    switch(*fmtTemplate++)
	    {
		/* Days of the week. */
		case 'a':
		    strOffs = 7;
		case 'A':
		{
		    STRPTR dayStr[7];
		    BOOL dayOk[7];
		    ULONG i, a;

		    for(i= 0; i < 7; i++)
		    {
			dayOk[i] = TRUE;
			dayStr[i] = GetLocaleStr(locale, i + strOffs + 1);
		    }

		    c = GetChar();
		    while((c != NULL) && (c != *fmtTemplate))
		    {
			for(i=0; i < 7; i++)
			{
			    a = ConvToUpper(locale, *(dayStr[i])++);
			    c = ConvToUpper(locale, c);

			    if(dayOk[i] && a)
				if(a != c)  dayOk[i] = FALSE;
			}
			c = GetChar();
		    }

		    /* End of stream in wrong place, or invalid */
		    if(((c == NULL) && *fmtTemplate) || (c != *fmtTemplate))
			return FALSE;

		    /* If we didn't get a valid day, fail */
		    i = 0;
		    while((i < 7) && (dayOk[i++] == FALSE))
			;
		    if((i == 7) && (dayOk[6] == FALSE))
			return FALSE;

		    if(*fmtTemplate)    fmtTemplate++;
		    checkEOF = FALSE;
		} break;  /* case 'A': */

		case 'b':
		case 'h':
		    strOffs = 12;
		case 'B':
		{
		    STRPTR monthStr[12];
		    BOOL monthOk[12];
		    ULONG i, a;

		    for(i= 0; i < 12; i++)
		    {
			monthOk[i] = TRUE;
			monthStr[i] = GetLocaleStr(locale, i + strOffs + MON_1);
		    }

		    c = GetChar();
		    while((c != NULL) && (c != *fmtTemplate))
		    {
			for(i=0; i < 12; i++)
			{
			    a = ConvToUpper(locale, *(monthStr[i])++);
			    c = ConvToUpper(locale, c);

			    if(monthOk[i] && a)
				if(a != c)  monthOk[i] = FALSE;
			}
			c = GetChar();
		    }

		    /* End of stream in wrong place, or invalid */
		    if(((c == NULL) && *fmtTemplate) || (c != *fmtTemplate))
			return FALSE;

		    /* If we didn't get a valid month, fail */
		    i = 0;
		    while((i < 12) && (monthOk[i++] == FALSE))
			;
		    if((i == 12) && (monthOk[11] == FALSE))
			return FALSE;
		    month = i;

		    if(*fmtTemplate)    fmtTemplate++;
		    checkEOF = FALSE;

		    break;
		} /* case 'B': */

		/* Day no, leading 0's */
		case 'd':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    day = (c - '0') * 10;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    day += (c - '0');

		    /* Day 0 is undefined. */
		    if(day == 0)    return FALSE;
		    day--;

		    /* day is unsigned, so day < 0 is not possible */
		    if(day > 31)
			return FALSE;

		    break;

		/* Day no., leading spaces. */
		case 'e':
		    day = 0;
		    c = GetChar();
		    while(IsSpace(locale, c) == TRUE)
			c = GetChar();

		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    day = (c - '0');

		    c = GetChar();
		    if(IsDigit(locale, c) == TRUE)
		    {
			day *= 10;
			day += (c - '0');
		    }
		    else
		    {
			if(c != *fmtTemplate++)
			    return FALSE;
			if(c == NULL)
			    checkEOF = FALSE;
		    }
		    if(day == 0)    return FALSE;
		    day--;
		    break;

		/* hour 24-hr style with 0's */
		case 'H':
		    ampm = FALSE;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    c = c - '0';
		    hour = c * 10;

		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    hour += (c - '0');

		    if(hour > 23)    return FALSE;
		    break;

		/* hour 12-hr style with 0's */
		case 'I':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    c = c - '0';
		    hour = c * 10;

		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    hour += (c - '0');

		    if(hour > 11)    return FALSE;
		    break;

		/* month num, with 0's */
		case 'm':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    month = (c - '0') * 10;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    month += (c - '0');

		    if((month > 12) || (month == 0))
			return FALSE;
		    break;

		/* minutes with 0's */
		case 'M':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    min = (c - '0') * 10;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    min += (c - '0');

		    if(min > 59)  return FALSE;
		    break;

		/* AM or PM string */
		case 'p':
		{
		    STRPTR amStr, pmStr;
		    BOOL amOk = TRUE, pmOk = TRUE;
		    ULONG a, b;
		    amStr = GetLocaleStr(locale, AM_STR);
		    pmStr = GetLocaleStr(locale, PM_STR);

		    c = GetChar();
		    while((c != NULL) && (c != *fmtTemplate))
		    {
			a = ConvToUpper(locale, *amStr++);
			b = ConvToUpper(locale, *pmStr++);
			c = ConvToUpper(locale, c);

			if(amOk && a)
			    if(a != c)   amOk = FALSE;

			if(pmOk && b)
			    if(b != c)   pmOk = FALSE;

			c = GetChar();
		    }

		    /* End of stream in wrong place, or invalid */
		    if(((c == NULL) && *fmtTemplate) || (c != *fmtTemplate))
			return FALSE;

		    /* Check whether we got AM or PM */
		    if(pmOk == TRUE)        ampm = TRUE;
		    else if(amOk == TRUE)   ampm = FALSE;

		    if(*fmtTemplate)    fmtTemplate++;
		    checkEOF = FALSE;
		    break;
		}

		case 'S':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    sec = (c - '0') * 10;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    sec += (c - '0');

		    if(sec > 59)  return FALSE;
		    break;

		case 'y':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;

		    year = (c - '0') * 10;
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    year += (c - '0');
		    year += (year < 78) ? 2000 : 1900;
		    break;

		case 'Y':
		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    year = (c - '0') * 1000;

		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    year += (c - '0') * 100;

		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    year += (c - '0') * 10;

		    c = GetChar();
		    if(IsDigit(locale, c) == FALSE)
			return FALSE;
		    year += (c - '0');

		    if(year < 1978)
			return FALSE;
		    break;

		default:
		    return FALSE;
		    break;
	    } /* switch() */
	} /* if(char == '%') */
	else
	{
	    c = GetChar();
	    if(c != *fmtTemplate++)
		return FALSE;
	}
    } /* while(*fmtTemplate) */

    /* Reached end of fmtTemplate, end of input stream? */
    if(checkEOF)
	if((GetChar() != 0)) return FALSE;

    if(date)
    {
    	BOOL leap;
	
#if 1
    	/* stegerg: based on dos.library/strtodate */

	/* First year must be 1978 */
	if (year < 1978)
	    return FALSE;

	/* Is this year a leap year ? */
	leap = (((year % 400) == 0) ||
	    (((year % 4) == 0) && !((year % 100) == 0)));

	/* Add the days for all years (without leap years) */
	day += (year - 1978) * 365;

   	year--;

	/* Add leap years */
	day += ((year / 4) - (year / 100) + (year / 400)
	    - (494 - 19 + 4));

	/* Add days of months */
	day += monthdays[month - 1];

	/*
	    in monthdays, February has 28 days. Correct this in
	    leap years if month is >= March.
	*/

    	if (leap && (month >= 3)) day++;
	
	date->ds_Days = day;

#else
	year -= 1978;

	if(year > 2)
	    day += (year-3) / 4 + 1;
	else if((year == 2) && (month > 2))
	    day += 1;

	date->ds_Days = year * 365 + day + monthdays[month - 1];
#endif

	date->ds_Minute = hour * 60 + min;
	if((hour < 12) && ampm)
	    date->ds_Minute += 720;
	date->ds_Tick = sec * TICKS_PER_SECOND;
    }
    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ParseDate */
