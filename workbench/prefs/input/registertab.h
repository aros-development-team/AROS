/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REGISTERTAB_H
#define REGISTERTAB_H

/****************************************************************************************/

#ifndef GRAPHICS_RASTPORT_H
#include <graphics/rastport.h>
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUITION_SCREENS_H
#include <intuition/screens.h>
#endif

/****************************************************************************************/

#define REGISTERTAB_IDCMP   	    IDCMP_MOUSEBUTTONS

#define REGISTERTAB_EXTRA_HEIGHT     4
#define REGISTERTAB_IMEXTRA_HEIGHT   2
#define REGISTERTABITEM_EXTRA_WIDTH  8
#define REGISTERTAB_SPACE_LEFT 	     8
#define REGISTERTAB_SPACE_RIGHT      8
#define REGISTERTAB_IMAGE_TEXT_SPACE 4

/****************************************************************************************/

struct RegisterTabItem
{
    CONST_STRPTR  	        text;
    Object  	    	   *image;
    WORD    	    	    textlen;
    WORD    	    	    x1, y1, x2, y2, w, h;
    WORD    	    	    tx, ty, ix, iy;
};

struct RegisterTab
{
    struct RegisterTabItem  *items;
    struct DrawInfo 	    *dri;
    WORD    	    	    numitems;
    WORD    	    	    active;
    WORD    	    	    left;
    WORD    	    	    top;
    WORD    	    	    width;
    WORD    	    	    height;
    WORD    	    	    framewidth;
    WORD    	    	    frameheight;
    WORD    	    	    fontw;
    WORD    	    	    fonth;
    WORD    	    	    fontb;
    WORD    	    	    slopew;
};

/****************************************************************************************/

void InitRegisterTab(struct RegisterTab *reg, struct RegisterTabItem *items);
void LayoutRegisterTab(struct RegisterTab *reg, struct Screen *scr, struct DrawInfo *dri, BOOL samewidth);
void SetRegisterTabPos(struct RegisterTab *reg, WORD left, WORD top);
void SetRegisterTabFrameSize(struct RegisterTab *reg, WORD width, WORD height);
void RenderRegisterTabItem(struct RastPort *rp, struct RegisterTab *reg, WORD item);
void RenderRegisterTab(struct RastPort *rp, struct RegisterTab *reg, BOOL alsoframe);
BOOL HandleRegisterTabInput(struct RegisterTab *reg, struct IntuiMessage *msg);

/****************************************************************************************/

#endif /* REGISTERTAB_H */

/****************************************************************************************/
