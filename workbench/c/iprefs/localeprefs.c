/*
    (C) 2001 AROS - The Amiga Research OS
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
    struct Locale *new, *old;
    
    D(bug("In IPrefs:LocalePrefs_Handler\n"));
    
    if ((new = OpenLocale(filename)))
    {
    	D(bug("In IPrefs:LocalePrefs_Handler. OpenLocale(\"%s\") okay\n", filename));
    	old = LocalePrefsUpdate(new);
    	D(bug("In IPrefs:LocalePrefs_Handler. New Locale installed\n", filename));
	if (old) CloseLocale(old);
    }
    
    D(bug("In IPrefs:LocalePrefs_Handler. Done.\n", filename));
}

/*********************************************************************************************/
