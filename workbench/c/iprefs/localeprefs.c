/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

void LocalePrefs_Handler(STRPTR filename)
{
    struct Locale  *newloc, *oldloc;
    struct TagItem  tags[] =
    {
    	{OC_BuiltInLanguage, (IPTR)"english"},
	{OC_Version 	   , 0	    	    },
	{TAG_DONE   	    	    	    }
    };
    
    D(bug("In IPrefs:LocalePrefs_Handler\n"));
    
    if ((newloc = OpenLocale(filename)))
    {
    	D(bug("In IPrefs:LocalePrefs_Handler. OpenLocale(\"%s\") okay\n", filename));
    	oldloc = LocalePrefsUpdate(newloc);
    	D(bug("In IPrefs:LocalePrefs_Handler. New Locale installed\n", filename));
	
	/* Never close old locale */
	
	/* if (oldloc) CloseLocale(oldloc); */
    }
    
    D(bug("In IPrefs:LocalePrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
