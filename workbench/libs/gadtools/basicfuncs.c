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

struct TextFont *preparefont(struct GadToolsBase_intern *GadToolsBase,
			     struct RastPort *rport, struct IntuiText *itext,
			     struct TextFont **oldfont)
{
    struct TextFont *font;

    if (itext->ITextFont)
    {
	*oldfont = rport->Font;
	font = OpenFont(itext->ITextFont);
	if (font)
	{
	    SetFont(rport, font);
	    /* SetSoftStyle(rport, itext->ITextFont->ta_Style, 0xffffffff) !!! */
	} else
	    font = rport->Font;
    } else
    {
	*oldfont = NULL;
	font = rport->Font;
    }
    SetABPenDrMd(rport, itext->FrontPen, itext->BackPen, itext->DrawMode);

    return font;
}

void closefont(struct GadToolsBase_intern *GadToolsBase,
	       struct RastPort *rport,
	       struct TextFont *font, struct TextFont *oldfont)
{
    if (oldfont)
    {
	SetFont(rport, oldfont);
	CloseFont(font);
    }
}

BOOL renderlabel(struct GadToolsBase_intern *GadToolsBase,
		 struct Gadget *gad, struct RastPort *rport, LONG redraw)
{
    struct TextFont *font, *oldfont;
    struct TextExtent te;
    int len, position, x, y;

    if (gad->GadgetText->IText)
    {
	if ((redraw != GREDRAW_TOGGLE) && (redraw != GREDRAW_UPDATE))
	{
	    font = preparefont(GadToolsBase, rport, gad->GadgetText, &oldfont);
	    if (!font)
		return FALSE;
	    len = strlen(gad->GadgetText->IText);
	    TextExtent(rport, gad->GadgetText->IText, len, &te);
	    position = gad->GadgetText->LeftEdge;
	    if (position & PLACETEXT_RIGHT)
	    {
		x = gad->LeftEdge + gad->Width + 7;
		y = gad->TopEdge + ((gad->Height - te.te_Height) / 2) + 1;
	    } else if (position & PLACETEXT_ABOVE)
	    {
		x = gad->LeftEdge - ((te.te_Width - gad->Width) / 2);
		y = gad->TopEdge - te.te_Height - 4;
	    } else if (position & PLACETEXT_BELOW)
	    {
		x = gad->LeftEdge - ((te.te_Width - gad->Width) / 2);
		y = gad->TopEdge + gad->Height + 3;
	    } else if (position & PLACETEXT_IN)
	    {
		x = gad->LeftEdge - ((te.te_Width - gad->Width) / 2);
		y = gad->TopEdge + ((gad->Height - te.te_Height) / 2) + 1;
	    } else /* PLACETEXT_LEFT */
	    {
		x = gad->LeftEdge - te.te_Width - 8;
		y = gad->TopEdge + ((gad->Height - te.te_Height) / 2) + 1;
	    }
	    gad->GadgetText->LeftEdge = 0;
	    PrintIText(rport, gad->GadgetText, x, y);
	    gad->GadgetText->LeftEdge = position;
	    closefont(GadToolsBase, rport, font, oldfont);
        }
    }
    return TRUE;
}


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
    /*    for (y=0; y<(height-1); y++)
    {
        for (x=0; x<(width-1); x++)
	{
	    if (y%2)
	    {
		if (!((x+1)%4))
		{
		    Move(rport, left + x, top + y);
		    Draw(rport, left + x, top + y);
		}
	    } else
	    {
		if (!((x+3)%4))
		{
		    Move(rport, left + x, top + y);
		    Draw(rport, left + x, top + y);
		}
            }
	}
    } */
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
    it->BackPen = dri->dri_Pens[BACKGROUNDPEN];
    it->DrawMode = JAM1;
    it->LeftEdge = ng->ng_Flags & (PLACETEXT_LEFT | PLACETEXT_RIGHT |
				   PLACETEXT_ABOVE | PLACETEXT_BELOW |
				   PLACETEXT_IN);
    it->TopEdge = 0;
    it->ITextFont = ng->ng_TextAttr;
    it->IText = ng->ng_GadgetText;
    it->NextText = NULL;

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
