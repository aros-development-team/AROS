/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef VAR
#define VAR extern
#endif

VAR struct IntuitionBase        *IntuitionBase;
VAR struct GfxBase              *GfxBase;

#ifdef __AROS__
VAR struct UtilityBase          *UtilityBase;
VAR struct LocaleBase           *LocaleBase;
#else
VAR struct Library              *UtilityBase;
VAR struct Library              *LocaleBase;
#endif

VAR struct Library  	    	*MUIMasterBase;
VAR struct Library  	    	*AslBase;
VAR struct Catalog              *catalog;
VAR struct ClockData	    	clockdata;
VAR UBYTE   	    	    	s[256];
VAR struct IControlPrefs    	icontrolprefs, restore_prefs;
VAR LONG    	    	    	prog_exitcode;

VAR Object  	    	    	*app, *wnd;

