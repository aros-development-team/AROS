/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LocDateToStr - locale.library's private replacement
    	  of dos.library/DateToStr function. IPrefs will install
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

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
 #define YEAR_FORMAT "%y"
#else
 #define YEAR_FORMAT "%Y"
#endif

#ifndef FORMAT_DEF
#define FORMAT_DEF 4
#endif

extern struct LocaleBase *globallocalebase;

AROS_UFH3(void, LocDateToStrPutCharFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, c, A1))
{
    AROS_USERFUNC_INIT

    STRPTR *buf = (STRPTR *)hook->h_Data;

    *(*buf)++ = c;

    AROS_USERFUNC_EXIT
}

 /*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(LONG, LocDateToStr,

/*  SYNOPSIS */
	AROS_LHA(struct DateTime *, datetime, D1),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 36, Locale)

/*  FUNCTION
    	See dos.library/DateToStr
	
    INPUTS
    	See dos.library/DateToStr

    RESULT

    NOTES
    	This function is not called by apps directly. Instead dos.library/DateToStr
	is patched to use this function. This means, that the LocaleBase parameter
	above actually points to DOSBase!!! But I may not rename it, because then
	no entry for this function is generated in the Locale functable by the
	corresponding script!
	
    EXAMPLE

    BUGS

    SEE ALSO
	dos.library/DateToStr, locale.library/FormatDate.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
#define LocaleBase globallocalebase

    struct Locale *loc;
    struct Hook   hook;
    
    STRPTR buf, fstring;
    const UBYTE * name;
    
    LONG days, mins, tick;

    /* Read time. */
    days = datetime->dat_Stamp.ds_Days;
    mins = datetime->dat_Stamp.ds_Minute;
    tick = datetime->dat_Stamp.ds_Tick;

    /*
	Check if timestamp is correct. Correct timestamps lie
	between the 1.1.1978 0:00:00 and the 11.7.5881588 23:59:59.
    */
    if((days < 0) ||
       ((ULONG)mins >= 24 * 60) ||
       ((ULONG)tick >= TICKS_PER_SECOND * 60))
    {
	return DOSFALSE;
    }
    
    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(LocDateToStrPutCharFunc);
    hook.h_Data = &buf;
    
    REPLACEMENT_LOCK;
    
    loc = (struct Locale *)IntLB(LocaleBase)->lb_CurrentLocale;
    
    if (datetime->dat_StrDay)
    {
    	buf = datetime->dat_StrDay;
    	name = GetLocaleStr(loc, DAY_1 + (days % 7));

	while((*buf++ = *name++) != 0)
	    ;	
    }
    
    if (datetime->dat_StrDate)
    {
    	buf = datetime->dat_StrDate;
	
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
		
	    default:
	    	fstring = "%d-%b-" YEAR_FORMAT;
		break;
		
	}
	
	if (datetime->dat_Flags & DTF_SUBST)
	{
	    struct DateStamp curr;
	    
	    DateStamp(&curr);
	    
	    curr.ds_Days -= datetime->dat_Stamp.ds_Days;
	    
	    if ((curr.ds_Days >= -1) && (curr.ds_Days <= 7))
	    {
	    	LONG strid;
		
	    	fstring = "";
		
		switch(curr.ds_Days)
		{
		    case -1:
		    	strid = TOMORROWSTR;
			break;
			
		    case 0:
		    	strid = TODAYSTR;
			break;
			
		    case 1:
		    	strid = YESTERDAYSTR;
			break;
			
		    default:
		    	strid = DAY_1 + (days % 7);
			break;
		}
		
    		name = GetLocaleStr(loc, strid);

		while((*buf++ = *name++) != 0)
		    ;	
		
	    } /* if ((curr.ds_Days >= -1) && (cur.ds_Days <= 7)) */
	    
	} /* if (datetime->dat_Flags & DTF_SUBST) */
	
	if (*fstring)
	{
	    FormatDate(loc, fstring, &datetime->dat_Stamp, &hook);
	}
		
    } /* if (datetime->dat_StrDate) */
    
    if (datetime->dat_StrTime)
    {
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
	
	FormatDate(loc, fstring, &datetime->dat_Stamp, &hook);
    }
    
    REPLACEMENT_UNLOCK;
   
    return TRUE;
    
    AROS_LIBFUNC_EXIT
    
} /* LocDateToStr */
