#ifndef FILEREQHOOKS_H
#define FILEREQHOOKS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: File requester specific defs.
    Lang: english
*/

#ifndef LAYOUT_H
#    include "layout.h"
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

/* Options */

#define FREQ_MIN_VISIBLELINES 	 	5
#define FREQ_MIN_FILECOLUMNWIDTH 	150

#ifdef __MORPHOS
#define FREQ_COOL_BUTTONS 		0
#else
#define FREQ_COOL_BUTTONS 		1
#endif

#define DEF_PROPWIDTH 20
#define MAX_PATTERN_LEN 257
#define MAX_PATH_LEN 1025
#define MAX_FILE_LEN 257

#define FREQ_FIRST_OBJECT(x) ((x)->Listview)
#define FREQ_LAST_OBJECT(x) ((x)->EraserGad)

struct FRUserData
{
    Object			*Listview;	
    Object			*OKBut;
    Object			*VolumesBut;
    Object			*ParentBut;
    Object			*CancelBut;
    Object			*PatternLabel;
    Object			*DrawerLabel;
    Object			*FileLabel;
    Object			*DirectoryScanSymbol;
    Object			*FileGad;
    Object			*PatternGad;
    Object			*PathGad;
    Object  	    	    	*EraserGad;
    struct ScrollerGadget 	ScrollGad;
    struct List			ListviewList;
    struct Hook			ListviewHook;
    struct Hook			StringEditHook;
    STRPTR			SelectPattern;
    UWORD 			ButWidth;
    UWORD 			ButHeight;
    WORD			LVColumnWidth[ASLLV_MAXCOLUMNS];
    UBYTE			LVColumnAlign[ASLLV_MAXCOLUMNS];
    UBYTE 			Flags;
	
};

/* Has the gadgetry been layouted before ? */

#define FRFLG_LAYOUTED 		(1 << 0)
#define FRFLG_SHOWING_VOLUMES	(1 << 1)

/* Menu IDs */

#define FRMEN_LASTNAME		1
#define FRMEN_NEXTNAME		2 
#define FRMEN_RESTORE		3 
#define FRMEN_PARENT		4 
#define FRMEN_VOLUMES		5 
#define FRMEN_UPDATE		6 
#define FRMEN_DELETE		7 
#define FRMEN_NEWDRAWER		8 
#define FRMEN_RENAME		9 
#define FRMEN_SELECT		10 
#define FRMEN_OK		11 
#define FRMEN_CANCEL		12 
#define FRMEN_BYNAME		13 
#define FRMEN_BYDATE		14 
#define FRMEN_BYSIZE		15 
#define FRMEN_ASCENDING		16 
#define FRMEN_DESCENDING	17 
#define FRMEN_DRAWERSFIRST	18 
#define FRMEN_DRAWERSMIX	19 
#define FRMEN_DRAWERSLAST	20 

#endif /* FILEREQHOOKS_H */
