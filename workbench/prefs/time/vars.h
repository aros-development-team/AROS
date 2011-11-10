/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef VAR
#define VAR extern
#endif

VAR struct Device	    	*TimerBase;
VAR struct Library		    *BattClockBase;
VAR struct MUI_CustomClass  *calendarmcc;
VAR struct MUI_CustomClass  *clockmcc;
VAR struct MsgPort  	    *TimerMP;
VAR struct timerequest	    *TimerIO;
VAR struct Catalog          *catalog;
VAR struct ClockData	    clockdata;
VAR UBYTE   	    	    s[256];

VAR LONG    	    	    prog_exitcode;

VAR Object  	    	    *app, *wnd, *calobj, *clockobj;
VAR Object  	    	    *monthobj, *yearobj;
VAR Object  	    	    *hourobj, *minobj, *secobj;

