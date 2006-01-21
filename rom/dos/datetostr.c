/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Convert a DateTime struct into strings.
    Lang: english
*/
#include "dos_intern.h"
#include "date.h"

#ifdef TEST
#	include <proto/dos.h>
#	include <stdio.h>
#	undef AROS_LH1
#	undef DateToStr
#	undef AROS_LHA
#	define AROS_LH1(ret,name,arg,type,base,offset,libname) \
    ret name (arg)
#	define AROS_LHA(type,name,reg) type name
#endif

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

/*****************************************************************************

    NAME */
#include <dos/datetime.h>
#include <proto/dos.h>

	AROS_LH1(BOOL, DateToStr,

/*  SYNOPSIS */
	AROS_LHA(struct DateTime *, datetime, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 124, Dos)

/*  FUNCTION
	DateToStr converts an AmigaDOS DateStamp to a human
	readable ASCII string as requested by your settings in the
	DateTime structure.

    INPUTS
	DateTime - a pointer to an initialized DateTime structure. The
		   DateTime structure should be initialized as follows:

		dat_Stamp: The datestamp to convert to ascii

		dat_Format: How to convert the datestamp into
			dat_StrDate. Can be any of the following:

		    FORMAT_DOS: AmigaDOS format (dd-mmm-yy). This
			is the default if you specify something other
			than any entry in this list.

		    FORMAT_INT: International format (yy-mmm-dd).

		    FORMAT_USA: American format (mm-dd-yy).

		    FORMAT_CDN: Canadian format (dd-mm-yy).

		    FORMAT_DEF default format for locale.


		dat_Flags: Modifies dat_Format. The only flag
			used by this function is DTF_SUBST. If set, then
			a string like "Today" or "Monday" is generated
			instead of the normal format if possible.

		dat_StrDay: Pointer to a buffer to receive the day of
			the week string. (Monday, Tuesday, etc.). If null,
			this string will not be generated.

		dat_StrDate: Pointer to a buffer to receive the date
			string, in the format requested by dat_Format,
			subject to possible modifications by DTF_SUBST. If
			null, this string will not be generated.

		dat_StrTime: Pointer to a buffer to receive the time
			of day string. If NULL, this will not be generated.


    RESULT
	A zero return indicates that the DateStamp was invalid, and could
	not be converted.  Non-zero indicates that the call succeeded.

    NOTES

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	DateStamp(), StrtoDate()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Starting days of the months in a leap year. */

    STRPTR buf, fstring;
    const UBYTE * name;

    LONG year, month, days, mins, tick, leap=1;

    /* Read time. */
    days=datetime->dat_Stamp.ds_Days;
    mins=datetime->dat_Stamp.ds_Minute;
    tick=datetime->dat_Stamp.ds_Tick;

    /*
	Check if timestamp is correct. Correct timestamps lie
	between the 1.1.1978 0:00:00 and the 11.7.5881588 23:59:59.
    */
    if(days<0||(ULONG)mins>=24*60||(ULONG)tick>=TICKS_PER_SECOND*60)
	return DOSFALSE;

    if(datetime->dat_StrDay!=NULL)
    {
	/* Get weekday name. The 1.1.1978 was a sunday. */
	buf=datetime->dat_StrDay;
	name=Dos_WeekTable[days%7];
	while((*buf++=*name++)!=0)
	    ;
    }

    if(datetime->dat_StrDate!=NULL)
    {
	/*
	    Calculate year and the days in the year. Use a short method
	    if the date is between the 1.1.1978 and the 1.1.2100:
	    Every year even divisible by 4 in this time span is a leap year.
	    There are 92 normal and 30 leap years there.
	*/
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
	    if(days>=Dos_DayTable[month])
	    {
		days-=Dos_DayTable[month];
		break;
	    }
	}

	/* Remember that 0 means 1.1.1978. */
	days++;
	month++;

	/* Build date string */
	buf=datetime->dat_StrDate;

	switch (datetime->dat_Format)
	{
	case FORMAT_INT: fstring="y-m-d"; break;
	case FORMAT_USA: fstring="M-d-y"; break;
	case FORMAT_CDN: fstring="d-M-y"; break;
	case FORMAT_DEF: fstring="d.M.y"; break;
	default:	 fstring="d-m-y"; break;
	}

	if (datetime->dat_Flags & DTF_SUBST)
	{
	    struct DateStamp curr;

	    DateStamp (&curr);

	    curr.ds_Days -= datetime->dat_Stamp.ds_Days;

	    if (curr.ds_Days >= -1 && curr.ds_Days <= 7)
	    {
		fstring = "";

		if (curr.ds_Days <= 1)
		    name = Dos_SubstDateTable[curr.ds_Days+1];
		else
		    name = Dos_WeekTable[datetime->dat_Stamp.ds_Days%7];

		while ((*buf++ = *name++) != 0);
	    }
	}

	if (*fstring)
	{
	    while (*fstring)
	    {
		switch(*fstring)
		{
		case 'y':
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
		    *buf++ = year/10%10+'0';
		    *buf++ = year%10+'0';
#else
		    *buf++ = year/1000%10+'0';
		    *buf++ = year/100%10+'0';
		    *buf++ = year/10%10+'0';
		    *buf++ = year%10+'0';
#endif
		    break;
		case 'm':
		    name=Dos_MonthTable[month-1];
		    while(*name)
			*buf++=*name++;
		    break;
		case 'M':
		    *buf++=month/10%10+'0';
		    *buf++=month%10+'0';
		    break;
		case 'd':
		    *buf++=days/10%10+'0';
		    *buf++=days%10+'0';
		    break;
		default:
		    *buf++=*fstring;
		    break;
		}

		fstring ++;
	    }

	    *buf = 0;
	}
    }

    if(datetime->dat_StrTime!=NULL)
    {
	/* Build time string */
	datetime->dat_StrTime[0] = mins/(10*60)+'0';
	datetime->dat_StrTime[1] = mins/60%10+'0';
	datetime->dat_StrTime[2] = ':';
	datetime->dat_StrTime[3] = mins/10%6+'0';
	datetime->dat_StrTime[4] = mins%10+'0';
	datetime->dat_StrTime[5] = ':';
	datetime->dat_StrTime[6] = tick/(10*TICKS_PER_SECOND)+'0';
	datetime->dat_StrTime[7] = tick/TICKS_PER_SECOND%10+'0';
	datetime->dat_StrTime[8] = 0;
    }

    /* All done. Return OK. */
    return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* DateToStr */

#ifdef TEST

#	include <stdio.h>
#	include <dos/datetime.h>
#	include <proto/dos.h>

int main (int argc, char ** argv)
{
    struct DateTime curr;
    char day[LEN_DATSTRING];
    char time[LEN_DATSTRING];
    char date[LEN_DATSTRING];

    DateStamp (&curr.dat_Stamp);

    curr.dat_Format  = FORMAT_DOS;
    curr.dat_Flags   = 0;
    curr.dat_StrDay  = day;
    curr.dat_StrDate = date;
    curr.dat_StrTime = time;

    DateToStr (&curr);

    printf ("Today is %s, %s. It's %s\n"
	, day
	, date
	, time
    );

    curr.dat_Format = FORMAT_DEF;

    DateToStr (&curr);

    printf ("Local date: %s\n", date);

    curr.dat_Flags = DTF_SUBST;

    DateToStr (&curr);

    printf ("Date with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days ++;

    DateToStr (&curr);

    printf ("Date +1 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days ++;

    DateToStr (&curr);

    printf ("Date +2 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days ++;

    DateToStr (&curr);

    printf ("Date +3 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days -= 4;

    DateToStr (&curr);

    printf ("Date -1 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days --;

    DateToStr (&curr);

    printf ("Date -2 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days --;

    DateToStr (&curr);

    printf ("Date -3 with DTF_SUBST: %s\n", date);

    curr.dat_Stamp.ds_Days   = 0;
    curr.dat_Stamp.ds_Minute = 0;
    curr.dat_Stamp.ds_Tick   = 0;

    DateToStr (&curr);

    printf ("First Date: %s, %s. Time: %s\n", day, date, time);

    curr.dat_Stamp.ds_Days   = 0;
    curr.dat_Stamp.ds_Minute = 1;
    curr.dat_Stamp.ds_Tick   = 0;

    DateToStr (&curr);

    printf ("First Date + 1 Minute: %s, %s. Time: %s\n", day, date, time);

    curr.dat_Stamp.ds_Days   = 0;
    curr.dat_Stamp.ds_Minute = 0;
    curr.dat_Stamp.ds_Tick   = 153;

    DateToStr (&curr);

    printf ("First Date + 153 Ticks: %s, %s. Time: %s\n", day, date, time);

    curr.dat_Stamp.ds_Days   = 1;
    curr.dat_Stamp.ds_Minute = 0;
    curr.dat_Stamp.ds_Tick   = 0;

    DateToStr (&curr);

    printf ("First Date: %s, %s. Time: %s\n", day, date, time);

    return 0;
} /* main */

#endif /* TEST */
