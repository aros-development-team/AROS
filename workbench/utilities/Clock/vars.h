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

VAR struct Library              *KeymapBase;
VAR struct Library              *LayersBase;
VAR struct Library              *GadToolsBase;
VAR struct Library              *DiskfontBase;

VAR struct MsgPort  	    	*TimerMP;
VAR struct timerequest	    	*TimerIO;
VAR struct Screen               *scr;
VAR struct Window               *win;
VAR struct DrawInfo             *dri;
VAR struct Menu                 *menus;
VAR APTR                        vi;
VAR struct Catalog              *catalog;
VAR WORD                        prog_exitcode;
VAR UBYTE                       s[300];

VAR BYTE    	    	    	opt_analogmode;
VAR BYTE    	    	    	opt_showdate;
VAR BYTE    	    	    	opt_showsecs;
VAR BYTE    	    	    	opt_alarm;
VAR BYTE    	    	    	opt_format;
VAR BYTE    	    	    	opt_24hour;
