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

#ifdef _AROS
VAR struct UtilityBase          *UtilityBase;
VAR struct LocaleBase           *LocaleBase;
#else
VAR struct Library              *UtilityBase;
VAR struct Library              *LocaleBase;
#endif

VAR struct Library  	    	*MUIMasterBase;
VAR struct Device	    	*TimerBase;
VAR struct MUI_CustomClass  	*calendarmcc;
VAR struct MUI_CustomClass  	*clockmcc;
VAR struct MsgPort  	    	*TimerMP;
VAR struct timerequest	    	*TimerIO;
VAR struct Catalog              *catalog;
VAR struct ClockData	    	clockdata;
VAR LONG    	    	    	firstweekday;
VAR UBYTE   	    	    	s[256];

VAR LONG    	    	    	prog_exitcode;

VAR Object  	    	    	*app, *wnd, *calobj, *clockobj;
VAR Object  	    	    	*monthobj, *yearobj;
VAR Object  	    	    	*hourobj, *minobj, *secobj;

