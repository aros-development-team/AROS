#ifndef FILEREQHOOKS_H
#define FILEREQHOOKS_H

/*
    (C) 1997 AROS - The Amiga Research OS
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
#define FREQ_COOL_BUTTONS 		1

#define DEF_PROPWIDTH 20
#define MAX_PATTERN_LEN 64

struct FRUserData
{
    Object			*Prop;
    Object			*Listview;
	
    Object			*OKBut;
    Object			*VolumesBut;
    Object			*ParentBut;
    Object			*CancelBut;
    Object			*PatternLabel;
    Object			*DrawerLabel;
    Object			*FileLabel;
    Object			*DirectoryScanSymbol;
    struct Gadget		*FileGad;
    struct Gadget		*PatternGad;
    struct Gadget		*PathGad;
    struct ScrollerGadget 	ScrollGad;
    struct List			ListviewList;
    struct Hook			ListviewHook;
    UWORD ButWidth;
    UWORD ButHeight;
    WORD			LVColumnWidth[ASLLV_MAXCOLUMNS];
    UBYTE			LVColumnAlign[ASLLV_MAXCOLUMNS];

    UBYTE Flags;
	
};

/* Has the gadgetry been layouted before ? */
#define FRFLG_LAYOUTED 		(1 << 0)
#define FRFLG_SHOWING_VOLUMES	(1 << 1)

#endif /* FILEREQHOOKS_H */
