/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Support functions for AROSCheckboxClass.
    Lang: english
*/

#include <exec/types.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>

#include "aroscheckbox_intern.h"


UWORD disabledpattern[2] = {0x4444, 0x1111};

/* draws a disabled pattern */
void drawdisabledpattern(struct CBBase_intern *AROSCheckboxBase,
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



struct TextFont *preparefont(struct CBBase_intern *AROSCheckboxBase,
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

void closefont(struct CBBase_intern *AROSCheckboxBase,
	       struct RastPort *rport,
	       struct TextFont *font, struct TextFont *oldfont)
{
    if (oldfont)
    {
	SetFont(rport, oldfont);
	CloseFont(font);
    }
}

BOOL renderlabel(struct CBBase_intern *AROSCheckboxBase,
		 struct Gadget *gad, struct RastPort *rport, LONG redraw)
{
    struct TextFont *font, *oldfont;
    struct TextExtent te;
    int len, position, x, y;

    if (gad->GadgetText->IText)
    {
	if ((redraw != GREDRAW_TOGGLE) && (redraw != GREDRAW_UPDATE))
	{
	    font = preparefont(AROSCheckboxBase, rport, gad->GadgetText, &oldfont);
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
	    closefont(AROSCheckboxBase, rport, font, oldfont);
        }
    }
    return TRUE;
}
