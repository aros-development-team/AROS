/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Support functions fro the palette class
    Lang: english
*/
#include <proto/graphics.h>

#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/screens.h>

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


/********************
**  InsidePalette  **
********************/

BOOL InsidePalette(struct IBox *pbox, WORD x, WORD y)
{
    return ((BOOL)(    (x > pbox->Left)
	      	    && (x < pbox->Left + pbox->Width - 1)
	      	    && (y > pbox->Top)
    	      	    && (y < pbox->Top + pbox->Height - 1)
    	      	    ));
    	 
}
    	


/*********************
**  ClickedColor()  **
*********************/

UBYTE ComputeColor(struct PaletteData *data, WORD x, WORD y, struct PaletteBase_intern *AROSPaletteBase)
{
     UWORD row, col;
     
     UBYTE color;
     
     EnterFunc(bug("ComputeColor(data=%p, x=%d, y=%d)\n", data, x, y));
     
     col = (x - data->pd_PaletteBox.Left)	/ data->pd_ColWidth;
     row = (y - data->pd_PaletteBox.Top)	/ data->pd_RowHeight;
     
     color = data->pd_NumCols * row + col;
     
     ReturnInt("ComputeColor", UBYTE, color);
}

/********************
**  RenderPalette  **
********************/

VOID RenderPalette(struct PaletteData *data,  UWORD *pens,
		struct RastPort *rp, struct PaletteBase_intern *AROSPaletteBase)
{
    UBYTE currentcolor = 0;
    WORD left, top;
    register UWORD col, row;
    struct IBox *pbox = &(data->pd_PaletteBox);
    
    EnterFunc(bug("RenderPalette(data=%p, pens=%p, rp=%p)\n", data, pens, rp));

    top  = pbox->Top + HBORDER;
    
    SetDrMd(rp, JAM1);
        	
    for (row = data->pd_NumRows; row; row --)
    {
    	left = pbox->Left + VBORDER;
    	for (col = data->pd_NumCols; col; col --)
    	{
    	    SetAPen(rp, pens[currentcolor]);
    	    
    	    RectFill(rp, left, top,
    	    	left + data->pd_ColWidth - VBORDER - 1, 
    	    	top + data->pd_RowHeight - HBORDER - 1 );
    	    	

	    currentcolor ++;
	    
    	    D(bug("Rectfilling area (%d, %d, %d, %d)\n ", left, top,
    	    left + data->pd_ColWidth - VBORDER - 1, top + data->pd_RowHeight - HBORDER));

    	    left += data->pd_ColWidth;
    	
    	} /* for (each row) */
    	top += data->pd_RowHeight;
    } /* for (each column) */
    
    ReturnVoid("RenderPalette");
}

/************************
**  UpdateActiveColor  **
************************/

VOID UpdateActiveColor( struct PaletteData	*data, 
			UWORD 			*pens, 
			struct RastPort 	*rp,
			struct PaletteBase_intern *AROSPaletteBase)
{

    WORD left, top, right, bottom;
    
    EnterFunc(bug("UpdateActiveColor(data=%p, pens=%p, rp=%p)\n",
    			data, pens, rp));
    
    SetAPen(rp, pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);

    if (data->pd_OldColor != data->pd_Color)
    {
    	left = data->pd_PaletteBox.Left + (data->pd_OldColor % data->pd_NumCols) * data->pd_ColWidth;
    	top  = data->pd_PaletteBox.Top  + (data->pd_OldColor / data->pd_NumCols) * data->pd_RowHeight;
    
    	D(bug("clearing old selected: left=%d, top=%d, oldcolor=%d\n",
    		left, top, data->pd_OldColor));

    	/* Clear area with BACKGROUNDPEN */
    	RectFill(rp, left, top,
    	    left + data->pd_ColWidth + VSELBORDER, top + data->pd_RowHeight + HSELBORDER);
    
    	/* Rerender in original color */
    	SetAPen(rp, pens[data->pd_OldColor + data->pd_ColorOffset]);
    
    	RectFill(rp, left, top,
    	    left + data->pd_ColWidth - VBORDER- 1, top + data->pd_RowHeight - HBORDER - 1);
    }

    /* Render new active entry */

    D(bug("rendering new selected: left=%d, top=%d, color=%d\n",
    	left, top, data->pd_Color));
 
    left  = data->pd_PaletteBox.Left + (data->pd_Color % data->pd_NumCols) * data->pd_ColWidth  + VBORDER;
    top   = data->pd_PaletteBox.Top  + (data->pd_Color / data->pd_NumCols) * data->pd_RowHeight + HBORDER;
    
    right  = left + data->pd_ColWidth   - 1;
    bottom = top  + data->pd_RowHeight  - 1;
    
    
    /* Draw some borders */
    SetAPen(rp, pens[SHADOWPEN]);
    
    /* left */
    RectFill(rp, left, top,
    	left + VSELBORDER - 1, bottom + HSELBORDER - 1);
    	
    /* top */
    RectFill(rp, left + VSELBORDER, top,
    	right + VSELBORDER - 1, top + HSELBORDER - 1);
    
    SetAPen(rp, pens[SHINEPEN]);
    
    /* right */	
    RectFill(rp, right, top + HSELBORDER,
    	right + VSELBORDER - 1, bottom + HSELBORDER - 1);
    	
    /* bottom */
    RectFill(rp, left + VSELBORDER, bottom,
    	right - 1, bottom + HSELBORDER - 1);
    	
    /* The newly update color becomes the new OldColor */
    data->pd_OldColor = data->pd_Color;
    
    ReturnVoid("UpdateActiveColor");
}


