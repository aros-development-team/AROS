/*
    (C) 1997-2000 AROS - The Amiga Research OS
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

VAR struct Screen               *scr;
VAR struct Window               *win;
VAR struct DrawInfo             *dri;
VAR struct Menu                 *menus;
VAR APTR                        vi;
VAR struct Catalog              *catalog;
VAR struct List     	    	country_list, language_list, pref_language_list;
VAR struct SerialPrefs	    	serialprefs;
VAR WORD    	    	    	winwidth, winheight, buttonwidth, buttonheight;
VAR BOOL    	    	    	truecolor;
VAR UBYTE   	    	    	s[256];

VAR LONG    	    	    	prog_exitcode;
