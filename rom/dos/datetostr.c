/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/11/21 10:49:47  aros
    Created macros AROS_SLIB_ENTRY() for assembler files, too, to solve naming
    problems.

    The #includes in the header *must* begin in the first column. Otherwise
    makedepend will ignore them (GCC works, though).

    Removed a couple of Logs

    Revision 1.4  1996/10/24 15:50:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:53  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:48  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/datetime.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH1(BOOL, DateToStr,

/*  SYNOPSIS */
	AROS_LHA(struct DateTime *, datetime, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 124, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Starting days of the months in a leap year. */
    const ULONG daytabl[]=
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

    char *const monthtable[]=
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    char *const weektable[]=
    { "Sunday", "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday" };

    STRPTR buf, name, fstring;

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
	return 0;

    if(datetime->dat_StrDay!=NULL)
    {
	/* Get weekday name. The 1.1.1978 was a sunday. */
	buf=datetime->dat_StrDay;
	name=weektable[days%7];
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
	if(days<92*365+30*365)
	{
	    /*
		1976 was a leap year so use it as a base to divide the days
		into 4-year blocks (each beginning with a leap year).
	    */
	    days+=366+365;
	    year=4*(days/(366+3*365))+1976;
	    days%=(366+3*365);
	    /* Now divide the 4-year blocks into single years. */
	    if(days>=366)
	    {
		leap=0;
		days--;
		year+=days/365;
		days%=365;
	    }
	}else
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
	    if(days>=daytabl[month])
	    {
		days-=daytabl[month];
		break;
	    }
	/* Remember that 0 means 1.1.1978. */
	days++;

	/* Build date string */
	buf=datetime->dat_StrDate;
	fstring="d-m-y";
	do
	    switch(*fstring)
	    {
		case 'y':
		    *buf++=year/10%10+'0';
		    *buf++=year%10+'0';
		    break;
		case 'm':
		    name=monthtable[month];
		    while(*name)
			*buf++=*name++;
		    break;
		case 'd':
		    *buf++=days/10+'0';
		    *buf++=days%10+'0';
		    break;
		default:
		    *buf++=*fstring;
		    break;
	    }
	while(*fstring++);
    }

    if(datetime->dat_StrTime!=NULL)
    {
	/* Build time string */
	datetime->dat_StrTime[0]=mins/(10*60)+'0';
	datetime->dat_StrTime[1]=mins/60%10+'0';
	datetime->dat_StrTime[2]=':';
	datetime->dat_StrTime[3]=mins/10%6+'0';
	datetime->dat_StrTime[4]=mins%10+'0';
	datetime->dat_StrTime[5]=':';
	datetime->dat_StrTime[6]=tick/(10*TICKS_PER_SECOND)+'0';
	datetime->dat_StrTime[7]=tick/TICKS_PER_SECOND%10+'0';
	datetime->dat_StrTime[8]=0;
    }

    /* All done. Return OK. */
    return 1;

    AROS_LIBFUNC_EXIT
} /* DateToStr */
