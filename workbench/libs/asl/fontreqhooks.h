#ifndef FONTREQHOOKS_H
#define FONTREQHOOKS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Font requester specific defs.
    Lang: english
*/
#ifndef LAYOUT_H
#    include "layout.h"
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef DISKFONT_DISKFONT_H
#   include <diskfont/diskfont.h>
#endif

/* Options */

#define FOREQ_MIN_VISIBLELINES 	 	3
#define FOREQ_VISIBILE_SIZE_CHARS   	4

#ifdef __MORPHOS
#define FOREQ_COOL_BUTTONS 		0
#else
#define FOREQ_COOL_BUTTONS 		1
#endif

#define FOREQ_FIRST_OBJECT(x) 	    	((x)->NameListview)
#define FOREQ_LAST_OBJECT(x) 	    	((x)->EraserGadget)

struct FOUserData
{
    Object			*NameListview;
    Object  	    	    	*SizeListview;
    Object  	    	    	*NameString;
    Object  	    	    	*SizeString;
    Object			*OKBut;
    Object			*CancelBut;
    Object  	    	    	*Preview;
    Object  	    	    	*DrawModeLabel;
    Object  	    	    	*DrawModeGadget;
    Object  	    	    	*StyleLabel;
    Object  	    	    	*StyleGadget;
    Object  	    	    	*ColorLabel;
    Object  	    	    	*FGColorGadget;
    Object  	    	    	*BGColorGadget;
    Object			*EraserGadget;
    
    struct ScrollerGadget 	NameScrollGad;
    struct ScrollerGadget   	SizeScrollGad;
    
    struct List			NameListviewList;
    struct Hook     	    	SizeListviewRenderHook;
    struct Hook     	    	StringEditHook;
    struct ASLLVFontReqNode 	*ActiveFont;
    struct AvailFontsHeader 	*AFH;
    struct TextFont 	    	*PreviewFont;
    
    UWORD 			ButWidth;
    UWORD 			ButHeight;    
  
};

struct ASLLVFontReqNode
{
    struct Node 	node;
    struct List     	SizeList;
    struct TextAttr 	TAttr;
    UWORD   	    	NumSizes;
    UBYTE   	    	Name[MAXFONTNAME + 2];
    struct Node	    	SizeNode[0];
    /* growing */
};

#define FOFLG_LAYOUTED (1 << 0)

/* Menu IDs */

#define FOMEN_LASTFONT		1
#define FOMEN_NEXTFONT		2
#define FOMEN_RESTORE		3
#define FOMEN_RESCAN	    	4
#define FOMEN_OK		5 
#define FOMEN_CANCEL		6 

#endif /* FONTREQHOOKS_H */
