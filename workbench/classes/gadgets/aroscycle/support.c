/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$

    Desc: Support functions for AROSCycleClass.
    Lang: english
*/
#include <strings.h>
#include <exec/types.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>

#include "aroscycle_intern.h"


UWORD disabledpattern[2] = {0x4444, 0x1111};

/* draws a disabled pattern */
void drawdisabledpattern(struct CycleBase_intern *AROSCycleBase,
			 struct RastPort *rport, UWORD pen,
			 WORD left, WORD top, UWORD width, UWORD height)
{
    SetABPenDrMd(rport, pen, 0, JAM1);
    rport->AreaPtrn = disabledpattern;
    rport->AreaPtSz = 1;
    RectFill(rport, left, top, left+width-1, top+height-1);
}


void renderlabel(struct CycleBase_intern *AROSCycleBase,
                 struct Gadget *gad,
                 STRPTR string,
                 struct RastPort *rport,
                 struct GadgetInfo *ginfo)
{
    UWORD *pens = ginfo->gi_DrInfo->dri_Pens;
    int len = strlen(string);

    SetABPenDrMd(rport, pens[TEXTPEN], pens[BACKGROUNDPEN], JAM1);
    Move(rport,
         gad->LeftEdge + (gad->Width - TextLength(rport, string, len)) / 2,
         gad->TopEdge + rport->Font->tf_YSize);
    Text(rport, string, len);
}
