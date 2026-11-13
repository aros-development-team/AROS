/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/drawlist.h
*/

#ifndef IMAGES_DRAWLIST_H
#define IMAGES_DRAWLIST_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define DRAWLIST_CLASSNAME  "drawlist.image"
#define DRAWLIST_VERSION    44

#define DRAWLIST_Dummy      (REACTION_Dummy + 0x17000)

#define DRAWLIST_Directives (DRAWLIST_Dummy+1) /* (struct DrawList *) Directive array */
#define DRAWLIST_RefHeight  (DRAWLIST_Dummy+2) /* (WORD) Reference height */
#define DRAWLIST_RefWidth   (DRAWLIST_Dummy+3) /* (WORD) Reference width */
#define DRAWLIST_DrawInfo   (DRAWLIST_Dummy+4) /* Obsolete - do not use */

/* DrawList primitive directives */
#define DLST_END        0   /* End of directive list */
#define DLST_LINE       1   /* Draw a line */
#define DLST_RECT       2   /* Draw a rectangle */
#define DLST_FILL       3   /* Draw a filled rectangle */
#define DLST_ELLIPSE    4   /* Draw an ellipse */
#define DLST_CIRCLE     5   /* Draw a circle */
#define DLST_LINEPAT    6   /* Set line pattern */
#define DLST_FILLPAT    7   /* Set fill pattern */
#define DLST_AMOVE      8   /* Absolute move */
#define DLST_ADRAW      9   /* Absolute draw */
#define DLST_AFILL      10  /* Absolute fill */
#define DLST_BEVELBOX   11  /* Draw a bevel box */
#define DLST_ARC        12  /* Draw an arc */
#define DLST_START      13  /* Start/bounds marker */
#define DLST_BOUNDS     13  /* Same as DLST_START */
#define DLST_LINESIZE   14  /* Set line size */

/* Directive array entry - last entry must be DLST_END */
struct DrawList
{
    WORD  dl_Directive;
    UWORD dl_X1, dl_Y1;
    UWORD dl_X2, dl_Y2;
    WORD  dl_Pen;
};

#ifndef DrawListObject
#define DrawListObject  NewObject(NULL, DRAWLIST_CLASSNAME
#endif
#ifndef DrawListEnd
#define DrawListEnd     TAG_END)
#endif

#endif /* IMAGES_DRAWLIST_H */
