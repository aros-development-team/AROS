/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Support functions fro the palette class
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>

#include "arospalette_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/**********************
**  GetGadgetIBox()  **
**********************/

VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left	 = EG(o)->LeftEdge;
    ibox->Top	 = EG(o)->TopEdge;
    ibox->Width  = EG(o)->Width;
    ibox->Height = EG(o)->Height;

    if (gi)
    {
	if (EG(o)->Flags & GFLG_RELRIGHT)
	    ibox->Left	 += gi->gi_Domain.Width - 1;

	if (EG(o)->Flags & GFLG_RELBOTTOM)
	    ibox->Top	 += gi->gi_Domain.Height - 1;

	if (EG(o)->Flags & GFLG_RELWIDTH)
	    ibox->Width  += gi->gi_Domain.Width;

	if (EG(o)->Flags & GFLG_RELHEIGHT)
	    ibox->Height += gi->gi_Domain.Height;
    }
}

/**********************
**  GetPalettePen()  **
**********************/

UWORD GetPalettePen(struct PaletteData *data, struct DrawInfo *dri, UWORD idx)
{
    UWORD pen;

    if (data->pd_ColorTable)
	pen = data->pd_ColorTable[idx];
    else
//	pen = dri->dri_Pens[idx];
	pen = idx + data->pd_ColorOffset;

    return (pen);
}

/*********************
**  Colors2Depth()  **
*********************/

UBYTE Colors2Depth(UWORD numcolors)
{
    UBYTE depth = 0;

    while ((1 << depth) < numcolors)
    	depth ++;

    return (depth);
}

/********************
**  InsidePalette  **
********************/

BOOL InsidePalette(struct PaletteData *data, WORD x, WORD y)
{
    /* Georg Steger */

    return TRUE;

#if 0
    BOOL inside = FALSE;
    struct IBox *pbox = &(data->pd_PaletteBox);

    /* Inside palette bounding box ? */

    if (    (x > pbox->Left)
	 && (x < pbox->Left + pbox->Width - 1)
	 && (y > pbox->Top)
    	 && (y < pbox->Top + pbox->Height - 1)
    	 )
    {
   	/* Must do additional testing, since we might
   	** not have 2^n number of colors
   	*/

   	if (data->pd_NumColors > ComputeColor(data, x, y))
   	{
   	    inside = TRUE;
   	}
    }
    return (inside);
#endif

}



/*********************
**  ComputeColor()  **
*********************/

UWORD ComputeColor(struct PaletteData *data, WORD x, WORD y)
{
     UWORD row, col;

     WORD color;

     col = (x - data->pd_PaletteBox.Left)	/ data->pd_ColWidth;
     row = (y - data->pd_PaletteBox.Top)	/ data->pd_RowHeight;

     color = data->pd_NumCols * row + col;

     /* Georg Steger */
     if (color < 0)
     {
     	 color = 0;
     }
     else if (color >= data->pd_NumColors)
     {
    	 color = data->pd_NumColors - 1;
     }
     
     return (UWORD)color;
}

/********************
**  RenderPalette  **
********************/

#define MIN(a, b) (( a < b) ? a : b)
VOID RenderPalette(struct PaletteData *data, struct DrawInfo *dri,
	struct RastPort *rp, struct PaletteBase_intern *AROSPaletteBase)
{
    UWORD currentcolor = data->pd_ColorOffset, colors_left;
    WORD left, top;
    register UWORD col, row;
    struct IBox *pbox = &(data->pd_PaletteBox);

    EnterFunc(bug("RenderPalette(data=%p, dri=%p, rp=%p)\n", data, dri, rp));

    top  = pbox->Top;

    colors_left = data->pd_NumColors;    
    SetDrMd(rp, JAM1);

    for (row = data->pd_NumRows; row; row --)
    {
    	left = pbox->Left;
    	for (col = MIN(data->pd_NumCols, colors_left); col; col --)
    	{

    	    SetAPen(rp, GetPalettePen(data, dri, currentcolor));

    	    RectFill(rp, left, top,
    	    	left + data->pd_ColWidth - VSPACING - 1, 
    	    	top + data->pd_RowHeight - HSPACING - 1 );

    	    D(bug("Rectfilling area (%d, %d, %d, %d)\n with color %d", left, top,
    	    left + data->pd_ColWidth - VSPACING - 1, top + data->pd_RowHeight - HSPACING,
    	    currentcolor));

	    currentcolor ++;

    	    left += data->pd_ColWidth;

    	} /* for (each row) */
    	top += data->pd_RowHeight;

    	colors_left -= data->pd_NumCols;

    } /* for (each column) */

    ReturnVoid("RenderPalette");
}

/************************
**  UpdateActiveColor  **
************************/

VOID UpdateActiveColor( struct PaletteData	*data, 
			struct DrawInfo		*dri,
			struct RastPort 	*rp,
			struct PaletteBase_intern *AROSPaletteBase)
{

    WORD left, top, right, bottom;

    struct IBox framebox;

    EnterFunc(bug("UpdateActiveColor(data=%p, dri=%p, rp=%p)\n",
    			data, dri, rp));

    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);

    if (data->pd_OldColor != data->pd_Color)
    {

    	left = data->pd_PaletteBox.Left + (data->pd_OldColor % data->pd_NumCols) * data->pd_ColWidth;
    	top  = data->pd_PaletteBox.Top  + (data->pd_OldColor / data->pd_NumCols) * data->pd_RowHeight;

    	D(bug("clearing old selected: (%d, %d, %d, %d) oldcolor=%d\n",
    		left, top, left + data->pd_ColWidth, top + data->pd_RowHeight, data->pd_OldColor));

    	/* Clear area with BACKGROUNDPEN */
    	RectFill(rp, 
    	    left - VBORDER,
    	    top - HBORDER,
    	    left + data->pd_ColWidth,
    	    top  + data->pd_RowHeight);

    	/* Rerender in original color */
    	SetAPen(rp, GetPalettePen(data, dri, data->pd_OldColor + data->pd_ColorOffset));

    	RectFill(rp, left, top,
    	    left + data->pd_ColWidth  - VSPACING - 1,
    	    top  + data->pd_RowHeight - HSPACING - 1);

    }

    left  = data->pd_PaletteBox.Left + (data->pd_Color % data->pd_NumCols) * data->pd_ColWidth;
    top   = data->pd_PaletteBox.Top  + (data->pd_Color / data->pd_NumCols) * data->pd_RowHeight;

    /* Right & bottom of *colored* area */
    right  = left + data->pd_ColWidth  - VSPACING - 1;
    bottom = top  + data->pd_RowHeight - HSPACING - 1;

    /* Render new active entry */
    D(bug("rendering new selected: (%d, %d, %d, %d), color=%d\n",
    	left, top, right, bottom, data->pd_Color));


    /* Draw some borders */

    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);

    /* We draw left & right from top to bottom, but draw top & bottom
    ** so they don't overlap with what's allready drawn
    */

    /* left */
    RectFill(rp, left, top,
    	left + 1, bottom);

    /* right */	
    RectFill(rp, right - 1, top,
    	right, bottom);

    /* top */
    RectFill(rp, left + 2, top,
    	right - 2, top + 1);

    /* bottom */
    RectFill(rp, left + 2, bottom - 1,
    	right - 2, bottom);

    /* Draw recessed frame around selected color */
    framebox.Left   = left - VBORDER;
    framebox.Top    = top  - HBORDER;
    framebox.Width  = data->pd_ColWidth  + VBORDER;
    framebox.Height = data->pd_RowHeight + HBORDER;

    RenderFrame(rp, &framebox, dri->dri_Pens, TRUE, AROSPaletteBase);

    /* The newly update color becomes the new OldColor */
    data->pd_OldColor = data->pd_Color;

    ReturnVoid("UpdateActiveColor");
}


/****************************
**  DrawDisabledPattern()  **
****************************/

UWORD disabledpattern[2] = {0x4444, 0x1111};

/* draws a disabled pattern */
void DrawDisabledPattern(struct RastPort *rport, struct IBox *gadbox, UWORD pen,
			 struct PaletteBase_intern *AROSPaletteBase)
{

    WORD x, y;

    EnterFunc(bug("DrawDisabledPattern(rp=%p, gadbox=%p, pen=%d)\n",
    		rport, gadbox, pen));


    for (y=0; y<(gadbox->Height-1); y++)
    {
        for (x=0; x<(gadbox->Width-1); x++)
	{
	    if (y%2)
	    {
		if (!((x+1)%4))
		{
		    Move(rport, gadbox->Left + x, gadbox->Top + y);
		    Draw(rport, gadbox->Left + x, gadbox->Top + y);
		}
	    } else
	    {
		if (!((x+3)%4))
		{
		    Move(rport, gadbox->Left + x, gadbox->Top + y);
		    Draw(rport, gadbox->Left + x, gadbox->Top + y);
		}
            }
	}
    }

    ReturnVoid("DrawDisabledPattern");
}

/******************
**  PrepareFont  **
******************/
struct TextFont *PrepareFont(struct RastPort *rport, struct IntuiText *itext,
			     struct TextFont **oldfont,
			     struct PaletteBase_intern *AROSPaletteBase)
{
    struct TextFont *font;

    EnterFunc(bug("PrepareFont(rp=%p, itext=%p)\n", rport, itext));

    if (itext->ITextFont)
    {
	*oldfont = rport->Font;
	font = OpenFont(itext->ITextFont);
	if (font)
	{
	    SetFont(rport, font);
#warning FIXME:
	    /* SetSoftStyle(rport, itext->ITextFont->ta_Style, 0xffffffff) FIXME */
	} else
	    font = rport->Font;
    } else
    {
	*oldfont = NULL;
	font = rport->Font;
    }
    SetABPenDrMd(rport, itext->FrontPen, itext->BackPen, itext->DrawMode);

    ReturnPtr("PrepareFont", struct TextFont *, font);
}

/********************
**  DisposeFont()  **
********************/
void DisposeFont(struct RastPort *rport,
	       struct TextFont *font, struct TextFont *oldfont,
	       struct PaletteBase_intern *AROSPaletteBase)
{
    EnterFunc(bug("DisposeFont(rp=%p, font=%p, oldofnt=%p)\n",
    		rport, font, oldfont));
    if (oldfont)
    {
	SetFont(rport, oldfont);
	CloseFont(font);
    }
 
    ReturnVoid("DisposeFont");
}

/********************
**  RenderLabel()  **
********************/
BOOL RenderLabel( struct Gadget *gad, struct IBox *gadbox, 
		struct RastPort *rport, LONG labelplace,
		struct PaletteBase_intern *AROSPaletteBase)
{
    struct TextFont *font = NULL, *oldfont;
    struct TextExtent te;
    STRPTR text;
    int len = 0, x, y;
    UWORD width, height;

    EnterFunc(bug("RenderLabel(gad=%p, gadbox=%p, rp=%p, labelplace=%ld)\n",
    	gad, gadbox, rport, labelplace));

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
            font = PrepareFont(rport, gad->GadgetText, &oldfont, AROSPaletteBase);
            if (!font)
                return FALSE;
        }

        if (text)
        {
            len = strlen(text);
            TextExtent(rport, text, len, &te);
            width  = te.te_Width;
            height = te.te_Height;
        }
        else
        {
            width  = ((struct Image *)gad->GadgetText)->Width;
            height = ((struct Image *)gad->GadgetText)->Height;
        }

        if (labelplace == GV_LabelPlace_Right)
        {
            x = gadbox->Left + gadbox->Width + 5;
            y = gadbox->Top + (gadbox->Height - height) / 2 + 1;
        }
        else if (labelplace == GV_LabelPlace_Above)
        {
            x = gadbox->Left - (width - gadbox->Width) / 2;
            y = gadbox->Top - height - 2;
        } 
        else if (labelplace == GV_LabelPlace_Below)
        {
            x = gadbox->Left - (width - gadbox->Width) / 2;
            y = gadbox->Top + gadbox->Height + 3;
        }
        else if (labelplace == GV_LabelPlace_In)
        {
            x = gadbox->Left - (width - gadbox->Width) / 2;
            y = gadbox->Top + (gadbox->Height - height) / 2 + 1;
        }
        else /* GV_LabelPlace_Left */
        {
            x = gadbox->Left - width - 4;
            y = gadbox->Top + (gadbox->Height - height) / 2 + 1;
        }

        if (gad->Flags & GFLG_LABELSTRING)
        {
            SetABPenDrMd(rport, 1, 0, JAM1);
            Move(rport, x, y);
            Text(rport, text, len);
        }
        else if (gad->Flags & GFLG_LABELIMAGE)
            DrawImage(rport, (struct Image *)gad->GadgetText, x, y);
        else
        {
            PrintIText(rport, gad->GadgetText, x, y);
            DisposeFont(rport, font, oldfont, AROSPaletteBase);
        }
    }
    ReturnBool("RenderLabel", TRUE);
}

/********************
**  RenderFrame()  **
********************/
VOID RenderFrame(struct RastPort *rp, struct IBox *gadbox, UWORD *pens,
	BOOL recessed, struct PaletteBase_intern *AROSPaletteBase)
{
    WORD left, top, right, bottom;
    UBYTE left_top_pen, right_bottom_pen;

    EnterFunc(bug("RenderFrame(rp=%p, gadbox=%p, pens=%p)\n",
    		rp, gadbox, pens));

    left = gadbox->Left; top = gadbox->Top;
    right = left + gadbox->Width - 1; bottom = top + gadbox->Height - 1;
 
    if (recessed)
    {
    	left_top_pen	 = SHADOWPEN;
    	right_bottom_pen = SHINEPEN;
    }
    else
    {
    	left_top_pen	 = SHINEPEN;
    	right_bottom_pen = SHADOWPEN;
    }

    /* Left */
    SetAPen(rp, pens[left_top_pen]);
    RectFill(rp, left, top, left + HBORDER - 1, bottom);

    /* Right */
    SetAPen(rp, pens[right_bottom_pen]);
    RectFill(rp, right - HBORDER + 1, top, right, bottom);

    /* Top */
    SetAPen(rp, pens[left_top_pen]);    		
    RectFill(rp, left + HBORDER, top, right - 1, top + HBORDER - 1);
 
    /* Bottom */
    SetAPen(rp, pens[right_bottom_pen]);
    RectFill(rp, left + 1, bottom - HBORDER + 1, right, bottom);

    ReturnVoid("RenderFrame");
}

