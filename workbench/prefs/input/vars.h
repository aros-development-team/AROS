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

VAR struct Library              *AslBase;
VAR struct Library              *GadToolsBase;
VAR struct Library  	    	*IFFParseBase;
VAR struct Library  	    	*CyberGfxBase;
#if USE_SHARED_COOLIMAGES
VAR struct Library  	    	*CoolImagesBase;
#endif

VAR struct MsgPort  	    	*InputMP;
VAR struct timerequest	    	*InputIO;
VAR struct Screen               *scr;
VAR struct Window               *win;
VAR struct DrawInfo             *dri;
VAR struct Menu                 *menus;
VAR APTR                        vi;
VAR struct Catalog              *catalog;
VAR struct List     	    	keymap_list;
VAR struct InputPrefs	    	inputprefs, restore_prefs;
VAR WORD    	    	    	pages_left, pages_top, pages_width, pages_height;
VAR WORD    	    	    	winwidth, winheight, buttonwidth, buttonheight;
VAR BOOL    	    	    	truecolor;
VAR UBYTE   	    	    	s[256];

VAR LONG    	    	    	prog_exitcode;
