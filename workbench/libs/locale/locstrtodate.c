/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocStrToDate - locale.library's private replacement
    	  of dos.library/StrToDate function. IPrefs will install
	  the patch.
	  
    Lang: english
*/

#include <exec/types.h>
#include <dos/datetime.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#include <string.h>

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
 #define YEAR_FORMAT "%y"
#else
 #define YEAR_FORMAT "%Y"
#endif

#ifndef FORMAT_DEF
#define FORMAT_DEF 4
#endif

AROS_UFH3(ULONG, LocStrToDateGetCharFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(ULONG, null, A1))
{
    AROS_USERFUNC_INIT

    STRPTR *buf = (STRPTR *)hook->h_Data;

    return *(*buf)++;

    AROS_USERFUNC_EXIT
}

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_PLH1(LONG, LocStrToDate,

/*  SYNOPSIS */
	AROS_LHA(struct DateTime *, datetime, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 37, Locale)

/*  FUNCTION
    	See dos.library/StrToDate
	
    INPUTS
    	See dos.library/StrToDate

    RESULT

    NOTES
    	This function is not called by apps directly. Instead dos.library/DosGet-
	LocalizedString is patched to use this function. This means, that the
	LocaleBase parameter above actually points to DOSBase, so we make use of 
	the global LocaleBase variable. This function is marked as private,
	thus the headers generator won't mind the different basename in the header.
	
    EXAMPLE

    BUGS

    SEE ALSO
	dos.library/StrToDate, locale.library/ParseDate.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Locale   *loc;
    struct Hook     hook;
    STRPTR  	    buf, fstring;
    LONG    	    days;
    LONG    	    retval = TRUE;

    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(LocStrToDateGetCharFunc);
    hook.h_Data = &buf;

    REPLACEMENT_LOCK;

    loc = (struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale;

    if (datetime->dat_StrDate)
    {
    	struct DateStamp curr;
	
	buf = datetime->dat_StrDate;

	DateStamp(&curr);
    	
	if (!strnicmp(buf, GetLocaleStr(loc, YESTERDAYSTR), strlen(GetLocaleStr(loc, YESTERDAYSTR))))
	{
	    datetime->dat_Stamp.ds_Days = curr.ds_Days - 1;
	}
	else if (!strnicmp(buf, GetLocaleStr(loc, TODAYSTR), strlen(GetLocaleStr(loc, TODAYSTR))))
	{
	    datetime->dat_Stamp.ds_Days = curr.ds_Days;
	}
	else if (!strnicmp(buf, GetLocaleStr(loc, TOMORROWSTR), strlen(GetLocaleStr(loc, TOMORROWSTR))))
	{
	    datetime->dat_Stamp.ds_Days = curr.ds_Days + 1;
	}
	else
	{
	    WORD i;
	
	    for(i = 0; i < 7; i++)
	    {
	    	if (!strnicmp(buf, GetLocaleStr(loc, DAY_1 + i), strlen(GetLocaleStr(loc, DAY_1 + i))))
		    break;
	    }
	
	    if (i != 7)
	    {
#if 1
	    	LONG diffdays;
		
		days = curr.ds_Days;
		
		diffdays = i - (days % 7);
		
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
		datetime->dat_Stamp.ds_Days = days;
#else
	    	days = curr.ds_Days;
		
		if ((days %7) == 0)
		    days -= 7;
		else
		    days -= (days % 7);
		
		days += i;
		
		if (datetime->dat_Flags & DTF_FUTURE)
		    days += 7;
		
		datetime->dat_Stamp.ds_Days = days;
#endif	
	    }
	    else
	    {

   		switch(datetime->dat_Format)
		{
		    case FORMAT_INT:
	    		fstring = YEAR_FORMAT "-%b-%d";
			break;

		    case FORMAT_USA:
	    		fstring = "%m-%d-" YEAR_FORMAT;
			break;

		    case FORMAT_CDN:
	    		fstring = "%d-%m-" YEAR_FORMAT;
			break;

		    case FORMAT_DEF:
	    		fstring = loc->loc_ShortDateFormat;
			break;

		    default: /* FORMAT_DOS */
	    		fstring = "%d-%b-" YEAR_FORMAT;
			break;

		}
		
		if (ParseDate(loc, &curr, fstring, &hook))
		{
		    datetime->dat_Stamp.ds_Days = curr.ds_Days;
    	    	}
		else
		{
		    retval = FALSE;
		}
		
	    }
	
	}

    } /* if (datetime->dat_StrDate) */
	

    if (retval && datetime->dat_StrTime)
    {
    	struct DateStamp ds;
	
    	buf = datetime->dat_StrTime;
	
 	switch(datetime->dat_Format)
	{
	    case FORMAT_DEF:
	    	fstring = loc->loc_ShortTimeFormat;
		break;
		
	    default:
	    	fstring = "%H:%M:%S";
		break;
	}
   	
    	if (ParseDate(loc, &ds, fstring, &hook))
	{
	    datetime->dat_Stamp.ds_Minute = ds.ds_Minute;
	    datetime->dat_Stamp.ds_Tick   = ds.ds_Tick;
	}
	else
	{
	    retval = FALSE;
	}
    }

    REPLACEMENT_UNLOCK;

    return retval;

    AROS_LIBFUNC_EXIT

} /* LocStrToDate */
