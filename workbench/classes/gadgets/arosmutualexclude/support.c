/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for AROSMutualExcludeClass.
    Lang: english
*/

/***********************************************************************************/

#include <strings.h>
#include <exec/types.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>

#include "arosmutualexclude_intern.h"

/***********************************************************************************/

UWORD disabledpattern[2] = {0x4444, 0x1111};

/***********************************************************************************/

/* draws a disabled pattern */
void drawdisabledpattern(struct RastPort *rport, UWORD pen,
			 WORD left, WORD top, UWORD width, UWORD height
)
{
    SetABPenDrMd(rport, pen, 0, JAM1);
    rport->AreaPtrn = disabledpattern;
    rport->AreaPtSz = 1;
    RectFill(rport, left, top, left+width-1, top+height-1);
}

/***********************************************************************************/

struct TextFont *preparefont(struct RastPort *rport, struct IntuiText *itext,
			     struct TextFont **oldfont
)
{
    struct TextFont *font;

    if (itext->ITextFont)
    {
	*oldfont = rport->Font;
	font = OpenFont(itext->ITextFont);
	if (font)
	{
	    SetFont(rport, font);
	    SetSoftStyle(rport, itext->ITextFont->ta_Style, 0xffffffff);
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

/***********************************************************************************/

void closefont(struct RastPort *rport,
	       struct TextFont *font, struct TextFont *oldfont
)
{
    if (oldfont)
    {
	SetFont(rport, oldfont);
	CloseFont(font);
    }
}

/***********************************************************************************/

BOOL renderlabel(struct Gadget *gad, struct RastPort *rport, struct MXData *data)
{
    struct TextFont *font = NULL, *oldfont;
    struct TextExtent te;
    STRPTR text;
    int len = 0, x, y;
    UWORD width, height;

    if (gad->GadgetText)
    {
        /* Calculate offsets. */
        if ((gad->Flags & GFLG_LABELSTRING))
            text = (STRPTR)gad->GadgetText;
        else if ((gad->Flags & GFLG_LABELIMAGE))
            text = NULL;
        else
        {
            /* GFLG_LABELITEXT */
            text = gad->GadgetText->IText;
            font = preparefont(rport, gad->GadgetText, &oldfont);
            if (!font)
                return FALSE;
        }

        if (text)
        {
            len = strlen(text);
            TextExtent(rport, text, len, &te);
            width  = te.te_Width;
            height = te.te_Height;
        } else
        {
            width  = ((struct Image *)gad->GadgetText)->Width;
            height = ((struct Image *)gad->GadgetText)->Height;
        }

        if (data->labelplace == GV_LabelPlace_Right)
        {
            x = data->bbox.MaxX + 5;
            y = (data->bbox.MinY + data->bbox.MaxY - height) / 2;
        } else if (data->labelplace == GV_LabelPlace_Above)
        {
	    x = (data->bbox.MinX + data->bbox.MaxX - width) / 2;
            y = data->bbox.MinY - height - 2;
        } else if (data->labelplace == GV_LabelPlace_Below)
        {
	    x = (data->bbox.MinX + data->bbox.MaxX - width) / 2;
            y = gad->TopEdge + gad->Height + 3;
        } else if (data->labelplace == GV_LabelPlace_In)
        {
	    x = (data->bbox.MinX + data->bbox.MaxX - width) / 2;
            y = (data->bbox.MinY + data->bbox.MaxY - height) / 2;
        } else /* GV_LabelPlace_Left */
        {
            x = data->bbox.MinX - width - 4;
            y = (data->bbox.MinY + data->bbox.MaxY - height) / 2;
        }

        if (gad->Flags & GFLG_LABELSTRING)
        {
            SetABPenDrMd(rport, 1, 0, JAM1);
            Move(rport, x, y + rport->TxBaseline);
            Text(rport, text, len);
        } else if (gad->Flags & GFLG_LABELIMAGE)
            DrawImage(rport, (struct Image *)gad->GadgetText, x, y);
        else
        {
            PrintIText(rport, gad->GadgetText, x, y);
            closefont(rport, font, oldfont);
        }
    }

    return TRUE;
}

/***********************************************************************************/
