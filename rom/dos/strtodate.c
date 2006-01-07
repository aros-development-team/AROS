/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Converts a string into a date
    Lang: english
*/
#include <string.h>
#include "dos_intern.h"

#ifdef TEST
#    include <proto/dos.h>
#    include <stdio.h>
#    undef AROS_LH1
#    undef StrToDate
#    undef AROS_LHA
#    define AROS_LH1(ret,name,arg,type,base,offset,libname) \
        ret name (arg)
#    define AROS_LHA(type,name,reg) type name

const ULONG Dos_DayTable[]=
{
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};

const char *const Dos_MonthTable[]=
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *const Dos_WeekTable[]=
{
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

const char *const Dos_SubstDateTable[]=
{
    "Tomorrow", "Today", "Yesterday"
};
#else
#    include "date.h"
#endif

/*****************************************************************************

    NAME */
#include <dos/datetime.h>
#include <proto/dos.h>

	AROS_LH1(BOOL, StrToDate,

/*  SYNOPSIS */
	AROS_LHA(struct DateTime *, datetime, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 125, Dos)

/*  FUNCTION
	Converts a human readable ASCII string into an AmigaDOS
	DateStamp.

    INPUTS
	DateTime - a pointer to an initialized DateTime structure.
		The structure should be initialized as follows:

		dat_Stamp: The converted date will be written here

		dat_Format: How to convert the datestamp into
		    dat_StrDate. Can be any of the following:

		    FORMAT_DOS: AmigaDOS format (dd-mmm-yy). This
			    is the default if you specify something other
			    than any entry in this list.

		    FORMAT_INT: International format (yy-mmm-dd).

		    FORMAT_USA: American format (mm-dd-yy).

		    FORMAT_CDN: Canadian format (dd-mm-yy).

		    FORMAT_DEF: default format for locale.


		dat_Flags: Modifies dat_Format. The only flag
			used by this function is DTF_FUTURE. If set, then
			a string like "Monday" refers to the next monday.
			Otherwise it refers to the last monday.

		dat_StrDay: Ignored.

		dat_StrDate: Pointer to valid string representing the
			date. This can be a "DTF_SUBST" style string such
			as "Today" "Tomorrow" "Monday", or it may be a
			string as specified by the dat_Format byte. This
			will be converted to the ds_Days portion of the
			DateStamp. If this pointer is NULL,
			DateStamp->ds_Days will not be affected.

		dat_StrTime: Pointer to a buffer which contains the
			time in the ASCII format hh:mm:ss. This will be
			converted to the ds_Minutes and ds_Ticks portions
			of the DateStamp.  If this pointer is NULL,
			ds_Minutes and ds_Ticks will be unchanged.


    RESULT
	A zero return indicates that a conversion could not be performed. A
	non-zero return indicates that the DateTime.dat_Stamp variable
	contains the converted values.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	DateStamp(), DateToStr()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    struct DateStamp curr;
    LONG days, min, tick, len, t, year, month;
    BOOL leap;
    UBYTE * ptr, * format;

    if ((ptr = datetime->dat_StrDate))
    {
	DateStamp (&curr);

	for (t=0; t<3; t++)
	{
	    if (!strncasecmp (Dos_SubstDateTable[t], ptr, strlen (Dos_SubstDateTable[t])))
		break;
	}

	if (t != 3)
	    days = curr.ds_Days + 1 - t;
	else
	{
	    for (t=0; t<7; t++)
	    {
		if (!strncasecmp (Dos_WeekTable[t], ptr, strlen (Dos_WeekTable[t])))
		    break;
	    }

	    if (t != 7)
	    {
	    #if 1
	    	LONG diffdays;
		
		days = curr.ds_Days;
		
		diffdays = t - (days % 7);
		
		if (datetime->dat_Flags & DTF_FUTURE)
		{
		    if (diffdays > 0)
		    {
		    	days += diffdays;
		    }
		    else
		    {
		    	days += 7 + diffdays;
		    }
		}
		else
		{
		    if (diffdays < 0)
		    {
		    	days += diffdays;
		    }
		    else
		    {
		    	days += diffdays - 7;
		    }
		}		
	    #else
		days = curr.ds_Days;

		if ((days % 7) == 0)
		    days -= 7;
		else
		    days -= (days % 7);

		days += t;

		if (datetime->dat_Flags & DTF_FUTURE)
		    days += 7;
	    #endif
	    }
	    else
	    {
		switch (datetime->dat_Format)
		{
		case FORMAT_INT: format = "y-m-d"; break;
		case FORMAT_USA: format = "M-d-y"; break;
		case FORMAT_CDN: format = "d-M-y"; break;
		case FORMAT_DEF: format = "d.M.y"; break;
		default:	 format = "d-m-y"; break;
		}

		while (*format)
		{
		    switch (*format)
		    {
		    case 'y':
			t = StrToLong (ptr, &year);

			if (t == -1)
			    return DOSFALSE;

			if (year < 100)
			    year += 1900;

			ptr += t;

			break;

		    case 'M':
			t = StrToLong (ptr, &month);

			if (t == -1)
			    return DOSFALSE;

			ptr += t;

			break;

		    case 'd':
			t = StrToLong (ptr, &days);

			if (t == -1)
			    return DOSFALSE;

			ptr += t;

			break;

		    case 'm':
			for (t=0; t<12; t++)
			{
			    if (!strncasecmp (Dos_MonthTable[t], ptr,
				    strlen (Dos_MonthTable[t])))
				break;
			}

			if (t == 12)
			    return DOSFALSE;

			month = t+1;

			ptr += strlen (Dos_MonthTable[t]);

			break;

		    default:
			if (*ptr != *format)
			    return DOSFALSE;

			ptr ++;

			break;

		    } /* switch */

		    format ++;
		} /* while */

		/* kprintf ("Year=%ld, Month=%ld, Days=%ld\n",
			year, month, days); */

		/* Days go from 1..x */
		days --;

		/* First year must be 1978 */
		if (year < 1978)
		    return DOSFALSE;

		/* Is this year a leap year ? */
		leap = (((year % 400) == 0) ||
		    (((year % 4) == 0) && !((year % 100) == 0)));

		/* Add the days for all years (without leap years) */
		days += (year - 1978) * 365;

#if 1
    	    	/* stegerg: we do *not* want a day to be added for *this*
		   year, if it is a leap year. Only the previous years
		   are the ones we want to be taken into account. */
		   
    	    	year--;
#endif

		/* Add leap years */
		days += ((year / 4) - (year / 100) + (year / 400)
		    - (494 - 19 + 4));

    	    	//kprintf("strtodate: days1 = %d\n", days);
		/* Add days of months */
		days += Dos_DayTable[month-1];
    	    	//kprintf("strtodate: days2 = %d\n", days);

		/*
		    In Dos_DayTable, February has 29 days. Correct this in
		    non-leap years and if the day has not yet been reached.
		*/

#if 1
    	    	/* stegerg: if this year is *no* leap year, then Dos_DayTable
		            is wrong by one day when accessing
			    Dos_DayTable[March..Dec] */
			    
    	    	if (!leap && (month >= 3)) days--;
#else
		if (month >= 2 || (leap && month < 2))
		    days --;
#endif

    	    	//kprintf("strtodate: days3 = %d\n", days);

	    } /* Normal date */

	} /* Not "Tomorrow", "Today" or "Yesterday" */

        datetime->dat_Stamp.ds_Days   = days;

    } /* Convert date ? */

    if ((ptr = datetime->dat_StrTime))
    {
	len = StrToLong (ptr, &t);

	if ((len == -1) || (t < 0) || (t > 23))
	    return DOSFALSE;
	    
	min = t * 60;

	ptr += len;

	if (*ptr++ != ':')
	    return DOSFALSE;

	len = StrToLong (ptr, &t);

	if ((len == -1) || (t < 0) || (t > 59))
	    return DOSFALSE;

	min += t;

	ptr += len;

	if (*ptr++ != ':')
	    return DOSFALSE;

	len = StrToLong (ptr, &t);

	if ((len == -1) || (t < 0) || (t > 59))
	    return DOSFALSE;

	tick = t * TICKS_PER_SECOND;

	datetime->dat_Stamp.ds_Minute = min;
	datetime->dat_Stamp.ds_Tick   = tick;

    }

    return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* StrToDate */

#ifdef TEST
#    include <stdio.h>

int main (int argc, char ** argv)
{
    struct DateTime dt;
    char * date;
    char * time;
    char daybuf[LEN_DATSTRING];
    char datebuf[LEN_DATSTRING];
    char timebuf[LEN_DATSTRING];

    if (argc >= 2)
	date = argv[1];
    else
	date = NULL;

    if (argc >= 3)
	time = argv[2];
    else
	time = NULL;

    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    dt.dat_Flags   = 0;
    dt.dat_Format  = FORMAT_CDN;

    if (!StrToDate (&dt))
    {
	printf ("Cannot convert date/time\n");
	return 10;
    }
    else
    {
	printf ("Result: Days=%ld, Minute=%ld, Ticks=%ld\n"
	    , dt.dat_Stamp.ds_Days
	    , dt.dat_Stamp.ds_Minute
	    , dt.dat_Stamp.ds_Tick
	);

	dt.dat_StrDay  = daybuf;
	dt.dat_StrDate = datebuf;
	dt.dat_StrTime = timebuf;

	DateToStr (&dt);

	printf ("Gives: %s, %s %s\n", daybuf, datebuf, timebuf);

	dt.dat_Flags   = DTF_SUBST;

	DateToStr (&dt);

	printf ("(With DTF_SUBST): %s, %s %s\n", daybuf, datebuf, timebuf);

    }

    return 0;
} /* main */

#endif /* TEST */
