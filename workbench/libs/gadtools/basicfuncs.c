/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic help functions needed by gadtools.library.
    Lang: English.
*/
#include <string.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <graphics/text.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <aros/debug.h>
#include "gadtools_intern.h"



UWORD disabledpattern[2] = {0x4444, 0x1111};

/* draws a disabled pattern */
void drawdisabledpattern(struct GadToolsBase_intern *GadToolsBase,
			 struct RastPort *rport, UWORD pen,
			 WORD left, WORD top, UWORD width, UWORD height)
{
    SetABPenDrMd(rport, pen, 0, JAM1);
    rport->AreaPtrn = disabledpattern;
    rport->AreaPtSz = 1;
    RectFill(rport, left, top, left+width-1, top+height-1);
}



struct IntuiText *makeitext(struct GadToolsBase_intern *GadToolsBase,
			    struct NewGadget *ng)
{
    struct IntuiText *it;
    struct DrawInfo *dri = ((struct VisualInfo *)ng->ng_VisualInfo)->vi_dri;

    it = (struct IntuiText *)AllocVec(sizeof(struct IntuiText), MEMF_ANY);
    if (!it)
	return NULL;

    if (!(ng->ng_Flags & NG_HIGHLABEL))
	it->FrontPen = dri->dri_Pens[TEXTPEN];
    else
	it->FrontPen = dri->dri_Pens[HIGHLIGHTTEXTPEN];
    it->BackPen   = dri->dri_Pens[BACKGROUNDPEN];
    it->DrawMode  = JAM1;
    it->LeftEdge  = 0;
    it->TopEdge   = 0;
    it->ITextFont = ng->ng_TextAttr;
    it->IText     = ng->ng_GadgetText;
    it->NextText  = NULL;

    return it;
}


void drawbevelsbyhand(struct GadToolsBase_intern *GadToolsBase,
                      struct RastPort *rport,
                      WORD left, WORD top, WORD width, WORD height,
                      struct TagItem *taglist)
{
    struct VisualInfo *vi;
    UWORD pen1, pen2;

    vi = (struct VisualInfo *)GetTagData(GT_VisualInfo, NULL, taglist);
    if (((BOOL)GetTagData(GTBB_Recessed, FALSE, taglist)) == FALSE)
    {
	pen1 = vi->vi_dri->dri_Pens[SHINEPEN];
	pen2 = vi->vi_dri->dri_Pens[SHADOWPEN];
    } else
    {
	pen1 = vi->vi_dri->dri_Pens[SHADOWPEN];
	pen2 = vi->vi_dri->dri_Pens[SHINEPEN];
    }

    SetDrMd(rport, JAM1);
    switch (GetTagData(GTBB_FrameType, BBFT_BUTTON, taglist))
    {
    case BBFT_BUTTON:
	SetAPen(rport, pen2);
	Move(rport, left + width, top);
	Draw(rport, left + width, top + height);
	Draw(rport, left, top + height);
	SetAPen(rport, pen1);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	break;
    case BBFT_RIDGE:
	SetAPen(rport, pen2);
	Move(rport, left + 1, top + height);
	Draw(rport, left + 1, top + 1);
	Draw(rport, left + width, top + 1);
	Move(rport, left + 2, top + height);
	Draw(rport, left + width, top + height);
	Draw(rport, left + width, top + 2);
	SetAPen(rport, pen1);
	Move(rport, left, top + height);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	Move(rport, left + 2, top + height - 1);
	Draw(rport, left + width - 1, top + height - 1);
	Draw(rport, left + width - 1, top + 2);
	break;
    case BBFT_ICONDROPBOX:
	SetAPen(rport, pen2);
	Move(rport, left + width, top);
	Draw(rport, left + width, top + height);
	Draw(rport, left, top + height);
	SetAPen(rport, pen1);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	Move(rport, left + width - 2, top + 2);
	Draw(rport, left + width - 2, top + height - 2);
	Draw(rport, left + 2, top + height - 2);
	SetAPen(rport, pen2);
	Draw(rport, left + 2, top + 2);
	Draw(rport, left + width - 2, top + 2);
	break;
    }
}
