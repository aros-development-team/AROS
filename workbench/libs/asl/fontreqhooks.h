#ifndef FONTREQHOOKS_H
#define FONTREQHOOKS_H

/*
    (C) 1997 AROS - The Amiga Research OS
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
#define FOREQ_COOL_BUTTONS 		1

#define FOREQ_FIRST_OBJECT(x) 	    	((x)->NameListview)
#define FOREQ_LAST_OBJECT(x) 	    	((x)->CancelBut)

struct FOUserData
{
    Object			*NameListview;
    Object  	    	    	*SizeListview;
    Object			*OKBut;
    Object			*CancelBut;
 
    struct ScrollerGadget 	NameScrollGad;
    struct ScrollerGadget   	SizeScrollGad;
    
    struct List			NameListviewList;
    struct List     	    	SizeListviewList;

    UWORD 			ButWidth;
    UWORD 			ButHeight;    
  
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
