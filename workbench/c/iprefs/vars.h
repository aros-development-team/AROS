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
VAR struct Library              *DataTypesBase;
VAR struct Library              *DiskfontBase;
VAR struct Library  	    	*IFFParseBase;

VAR struct MsgPort  	    	*notifyport;
VAR struct IPrefsSem	    	*iprefssem;

VAR WORD    	    	    	prog_exitcode;
VAR UBYTE   	    	    	s[256];

VAR UBYTE   	    	    	envname[256];

VAR UBYTE   	    	    	inputprefsname[256];
VAR UBYTE   	    	    	fontprefsname[256];
VAR UBYTE   	    	    	screenprefsname[256];
VAR UBYTE   	    	    	localeprefsname[256];
VAR UBYTE   	    	    	paletteprefsname[256];
VAR UBYTE   	    	    	patternprefsname[256];
VAR UBYTE   	    	    	icontrolprefsname[256];
VAR UBYTE   	    	    	screenmodeprefsname[256];
VAR UBYTE   	    	    	serialprefsname[256];
VAR UBYTE   	    	    	printerprefsname[256];
VAR UBYTE   	    	    	pointerprefsname[256];
VAR UBYTE   	    	    	overscanprefsname[256];

VAR BOOL    	    	    	patches_installed;
