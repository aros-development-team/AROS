/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Internal GadTools palette class.
    Lang: English
*/
 

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <utility/tagitem.h>
#include <devices/inputevent.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h> /* memset() */
#include <stdlib.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**********************************************************************************************/

#define HSPACING	2
#define VSPACING	2

#define HBORDER	    	HSPACING
#define VBORDER     	VSPACING


#define HSELBORDER	1
#define VSELBORDER	1

#define GadToolsBase 	((struct GadToolsBase_intern *)cl->cl_UserData)

/**********************************************************************************************/

STATIC VOID RenderFrame(struct PaletteData *data, struct RastPort *rp, struct IBox *gadbox,
            	    	struct DrawInfo *dri, BOOL recessed, BOOL edgesonly, Class *cl);

/**********************************************************************************************/

STATIC VOID GetGadgetIBox(struct Gadget *g, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left	 = g->LeftEdge;
    ibox->Top	 = g->TopEdge;
    ibox->Width  = g->Width;
    ibox->Height = g->Height;

    if (gi)
    {
	if (g->Flags & GFLG_RELRIGHT)
	    ibox->Left	 += gi->gi_Domain.Width - 1;

	if (g->Flags & GFLG_RELBOTTOM)
	    ibox->Top	 += gi->gi_Domain.Height - 1;

	if (g->Flags & GFLG_RELWIDTH)
	    ibox->Width  += gi->gi_Domain.Width;

	if (g->Flags & GFLG_RELHEIGHT)
	    ibox->Height += gi->gi_Domain.Height;
    }
}

/**********************************************************************************************/

STATIC UWORD GetPalettePen(struct PaletteData *data, UWORD idx)
{
    UWORD pen;

    if (data->pd_ColorTable)
	pen = data->pd_ColorTable[idx];
    else
	pen = idx + data->pd_ColorOffset;

    return (pen);
}

/**********************************************************************************************/

STATIC UBYTE Colors2Depth(UWORD numcolors)
{
    UBYTE depth = 0;

    while ((1 << depth) < numcolors)
    	depth ++;

    return (depth);
}

/**********************************************************************************************/

STATIC BOOL InsidePalette(struct PaletteData *data, WORD x, WORD y)
{
    (void)data;
    (void)x;
    (void)y;

    return TRUE;
}

/**********************************************************************************************/

STATIC UWORD ComputeColor(struct PaletteData *data, WORD x, WORD y)
{
     UWORD row, col;

     WORD color;

     col = (x - data->pd_PaletteBox.Left)	/ data->pd_ColWidth;
     row = (y - data->pd_PaletteBox.Top)	/ data->pd_RowHeight;

     color = data->pd_NumCols *row + col;

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

/**********************************************************************************************/

#define MIN(a, b) (( a < b) ? a : b)

STATIC VOID RenderPalette(struct PaletteData *data, struct RastPort *rp, Class *cl)
{
    UWORD   	    currentcolor = data->pd_ColorOffset, colors_left;
    WORD    	    left, top;
    register UWORD  col, row;
    struct IBox     *pbox = &(data->pd_PaletteBox);

    EnterFunc(bug("RenderPalette(data=%p, rp=%p)\n", data, rp));

    top  = pbox->Top;

    colors_left = data->pd_NumColors;    
    SetDrMd(rp, JAM1);

    for (row = data->pd_NumRows; row; row --)
    {
    	left = pbox->Left;
    	for (col = MIN(data->pd_NumCols, colors_left); col; col --)
    	{

    	    SetAPen(rp, GetPalettePen(data, currentcolor));

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

/**********************************************************************************************/

VOID UpdateActiveColor( struct PaletteData	*data, 
			struct DrawInfo		*dri,
			struct RastPort 	*rp,
			Class	    	    	*cl)
{
    struct IBox framebox;
    WORD    	left, top, right, bottom;

    EnterFunc(bug("UpdateActiveColor(data=%p, dri=%p, rp=%p)\n",
    			data, dri, rp));

    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);

    if (data->pd_OldColor != data->pd_Color)
    {

    	left = data->pd_PaletteBox.Left + (data->pd_OldColor % data->pd_NumCols) *data->pd_ColWidth;
    	top  = data->pd_PaletteBox.Top  + (data->pd_OldColor / data->pd_NumCols) *data->pd_RowHeight;

    	D(bug("clearing old selected: (%d, %d, %d, %d) oldcolor=%d\n",
    		left, top, left + data->pd_ColWidth, top + data->pd_RowHeight, data->pd_OldColor));

    	/* Clear area with BACKGROUNDPEN */
    	RectFill(rp, 
    	    left - VBORDER,
    	    top - HBORDER,
    	    left + data->pd_ColWidth - 1,
    	    top  + data->pd_RowHeight - 1);

    	/* Rerender in original color */
    	SetAPen(rp, GetPalettePen(data, data->pd_OldColor + data->pd_ColorOffset));

    	RectFill(rp, left, top,
    	    left + data->pd_ColWidth  - VSPACING - 1,
    	    top  + data->pd_RowHeight - HSPACING - 1);

    }

    left  = data->pd_PaletteBox.Left + (data->pd_Color % data->pd_NumCols) *data->pd_ColWidth;
    top   = data->pd_PaletteBox.Top  + (data->pd_Color / data->pd_NumCols) *data->pd_RowHeight;

    /* Right & bottom of *colored* area */
    right  = left + data->pd_ColWidth  - VSPACING - 1;
    bottom = top  + data->pd_RowHeight - HSPACING - 1;

    /* Render new active entry */
    D(bug("rendering new selected: (%d, %d, %d, %d), color=%d\n",
    	left, top, right, bottom, data->pd_Color));


    if ((right - left >= 6) && (bottom - top >= 6))
    {
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
    }
    
    /* Draw recessed frame around selected color */
    framebox.Left   = left - VBORDER;
    framebox.Top    = top  - HBORDER;
    framebox.Width  = data->pd_ColWidth  + VBORDER;
    framebox.Height = data->pd_RowHeight + HBORDER;

    RenderFrame(data, rp, &framebox, dri, TRUE, TRUE, cl);

    /* The newly update color becomes the new OldColor */
    data->pd_OldColor = data->pd_Color;

    ReturnVoid("UpdateActiveColor");
}

/**********************************************************************************************/

STATIC VOID RenderFrame(struct PaletteData *data, struct RastPort *rp, struct IBox *gadbox,
            	    	struct DrawInfo *dri, BOOL recessed, BOOL edgesonly, Class *cl)
{
    WORD left, top, right, bottom;

    EnterFunc(bug("RenderFrame(rp=%p, gadbox=%p, dri=%p)\n",
    		rp, gadbox, dri));

    left = gadbox->Left; top = gadbox->Top;
    right = left + gadbox->Width - 1; bottom = top + gadbox->Height - 1;

    if (!data->pd_Frame)
    {
	struct TagItem frame_tags[] =
	{
	    {IA_Resolution	, (dri->dri_Resolution.X << 16) +  dri->dri_Resolution.Y},
	    {IA_FrameType	, FRAME_BUTTON						},
	    {IA_EdgesOnly	, edgesonly						},
	    {TAG_DONE									}
	};

	data->pd_Frame = NewObjectA(NULL, FRAMEICLASS, frame_tags);
    }
     
    if (data->pd_Frame)
    {
        struct TagItem frameset_tags[] =
	{
	    {IA_Width		, gadbox->Width		},
	    {IA_Height		, gadbox->Height	},
	    {IA_Recessed	, recessed		},
	    {IA_EdgesOnly	, edgesonly		},
	    {TAG_DONE					}
	};
	
	SetAttrsA(data->pd_Frame, frameset_tags);

	DrawImageState(rp,
		       (struct Image *)data->pd_Frame,
		       left,
		       top,
		       IDS_NORMAL,
		       dri);
	
    }
    
    ReturnVoid("RenderFrame");
}

/**********************************************************************************************/

STATIC IPTR palette_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem  	*tag;
    const struct TagItem *tstate = msg->ops_AttrList;
    struct PaletteData  *data = INST_DATA(cl, o);
    BOOL    	    	labelplace_set = FALSE, relayout = FALSE;
    BOOL    	    	colortag_found = FALSE, numcolorstag_found = FALSE;
    IPTR    	    	retval = 0UL;
    
        
    EnterFunc(bug("Palette::Set()\n"));
    
    while ((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTPA_Depth:		/* [ISU] */
		/* NumColors tag overrides Depth tag! */
		if (!numcolorstag_found)
		{	        
    		    data->pd_NumColors = (1 << ((UBYTE)tidata));

    		    D(bug("Depth initialized to %d\n", tidata));
    		    relayout = TRUE;
    		    retval = 1UL;
		}
    		break;

    	    case GTPA_Color:		/* [IS] */
		colortag_found = TRUE;

    		data->pd_OldColor = data->pd_Color;
    		data->pd_Color = (UBYTE)tidata;
    		D(bug("Color set to %d\n", tidata));    	    
    		retval = 1UL;
    		break;

    	    case GTPA_ColorOffset:	/* [I] */
    		data->pd_ColorOffset = (UBYTE)tidata;
    		D(bug("ColorOffset initialized to %d\n", tidata));
    		retval = 1UL;
    		break;

    	    case GTPA_IndicatorWidth:	/* [I] */
    		data->pd_IndWidth = (UWORD)tidata;
    		D(bug("Indicatorwidth set to %d\n", tidata));

    		/* If palette has an indictor on left, GA_LabelPlace
    		** defaults to GV_LabelPlace_Left
    		*/
    		if (!labelplace_set)
    	 	    data->pd_LabelPlace = GV_LabelPlace_Left;
    		break;

    	    case GTPA_IndicatorHeight:	/* [I] */
    		data->pd_IndHeight = (UWORD)tidata;
    		D(bug("Indicatorheight set to %d\n", tidata));
    		break;

    	    case GA_LabelPlace:		/* [I] */
    		data->pd_LabelPlace = (LONG)tidata;
    		D(bug("Labelplace set to %d\n", tidata));

    		labelplace_set = TRUE;
    		break;

    	    case GA_TextAttr:			/* [I] */
    		data->pd_TAttr = (struct TextAttr *)tidata;
    		D(bug("TextAttr set to %s %d\n",
    	    	    data->pd_TAttr->ta_Name, data->pd_TAttr->ta_YSize));
    		break;

    	    case GTPA_ColorTable:
    		data->pd_ColorTable = (UBYTE *)tidata;
    		break;

    	    case GTPA_NumColors:
		numcolorstag_found = TRUE;

    		data->pd_NumColors = (UWORD)tidata;
    		relayout = TRUE;
    		break;

    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */

    if (colortag_found)
    {
    	/* convert pen number to index */
	
    	if (data->pd_ColorTable)
	{	
            WORD i;

	    /* convert pen number to index number */
	    for(i = 0; i < data->pd_NumColors; i++)
	    {
		if (data->pd_ColorTable[i] == data->pd_Color)
		{
	            data->pd_Color = i;
		    break;
		}
	    }
	    
	} else {
	    data->pd_Color -= data->pd_ColorOffset;
	}
	
    } /* if (colortag_found) */
        
    if (relayout)
    {
    	/* Check if the old selected fits into the new depth */
    	if (data->pd_Color > data->pd_NumColors - 1)
    	{
    	    data->pd_Color = 0;
    	    data->pd_OldColor = 0; /* So that UpdateActiveColor() don't get confused */
    	}

    	/* Relayout the gadget */
    	DoMethod(o, GM_LAYOUT, (IPTR) msg->ops_GInfo, FALSE);
    }
    
    ReturnPtr ("Palette::Set", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTPalette__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PaletteData *data   = INST_DATA(cl, o);
    IPTR    	        retval = 0;
    
    switch (msg->opg_AttrID)
    {
	case GTA_GadgetKind:
	case GTA_ChildGadgetKind:
	    *(msg->opg_Storage) = PALETTE_KIND;
	    retval = 1UL;
	    break;

  	case GTPA_Color:
	    *msg->opg_Storage = (IPTR)GetPalettePen(data, data->pd_Color);
	    break;
	
	case GTPA_ColorOffset:
	    *msg->opg_Storage = (IPTR)data->pd_ColorOffset;
	    break;
	
	case GTPA_ColorTable:
	    *msg->opg_Storage = (IPTR)data->pd_ColorTable;
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/**********************************************************************************************/

IPTR GTPalette__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct opSet    ops;
    struct TagItem  tags[] =
    {
	{GA_RelSpecial	, TRUE},
	{TAG_MORE   	, (IPTR)NULL}
    };

    EnterFunc(bug("Palette::New()\n"));
    
    tags[1].ti_Data = (IPTR)msg->ops_AttrList;
    
    ops.MethodID	= OM_NEW;
    ops.ops_GInfo	= NULL;
    ops.ops_AttrList	= &tags[0];
 
    o = (Object *)DoSuperMethodA(cl, o, (Msg)&ops);
    if (o)
    {
    	struct PaletteData *data = INST_DATA(cl, o);
    	
    	/* Set some defaults */
    	data->pd_NumColors	= 2;
    	data->pd_ColorTable	= NULL;
    	data->pd_Color		= 0;
    	data->pd_OldColor	= 0 ; /* = data->pd_OldColor */
    	data->pd_ColorOffset	= 0;
    	data->pd_IndWidth 	= 0;
    	data->pd_IndHeight	= 0;
    	data->pd_LabelPlace	= GV_LabelPlace_Above;
  	
    	palette_set(cl, o, msg);
      
    }
    ReturnPtr ("Palette::New", IPTR, (IPTR)o);
}

/**********************************************************************************************/

IPTR GTPalette__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct PaletteData *data = INST_DATA(cl, o);
    
    if (data->pd_Frame) DisposeObject(data->pd_Frame);
    
    return DoSuperMethodA(cl, o, msg);
}

/**********************************************************************************************/

IPTR GTPalette__GM_LAYOUT(Class *cl, struct Gadget *g, struct gpLayout *msg)
{
    
    /* The palette gadget has been resized and we need to update our layout */
    
    struct PaletteData  *data = INST_DATA(cl, g);
    struct IBox     	*gbox   = &(data->pd_GadgetBox),
    		    	*pbox   = &(data->pd_PaletteBox),
    		    	*indbox = &(data->pd_IndicatorBox);

    
    UWORD   	    	cols, rows;

    WORD    	    	factor1, factor2, ratio;
    UWORD   	    	fault, smallest_so_far;
    
    UWORD   	    	*largest, *smallest;
    
    WORD    	    	leftover_width, leftover_height;
    
    EnterFunc(bug("Palette::Layout()\n"));
    
    if (!msg->gpl_GInfo)
    	ReturnInt("Palette::Layout", IPTR, 0); /* We MUST have a GInfo to get screen aspect ratio */
    
    /* Delete the old gadget box */
    if (!msg->gpl_Initial)
    {

    	struct RastPort *rp;    
    
    	if ((rp = ObtainGIRPort(msg->gpl_GInfo)))
    	{
    	    SetAPen(rp, msg->gpl_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN]);
    	    D(bug("Clearing area (%d, %d, %d, %d)\n",
    	    	gbox->Left, gbox->Top, gbox->Left + gbox->Width, gbox->Top + gbox->Height));
    	    RectFill(rp, gbox->Left, gbox->Top, 
    	    	gbox->Left + gbox->Width - 1, gbox->Top + gbox->Height - 1);

	    ReleaseGIRPort(rp);
    	}
    }

    /* get the IBox surrounding the whole palette */
    GetGadgetIBox(g, msg->gpl_GInfo, gbox);
    
    D(bug("Got palette ibox: (%d, %d, %d, %d)\n",
    	gbox->Left, gbox->Top, gbox->Width, gbox->Height));
    	
    /* Get the palette box */
    pbox->Left   = gbox->Left + VBORDER + VSPACING;
    pbox->Top    = gbox->Top  + HBORDER + HSPACING;
    pbox->Width  = gbox->Width  - VBORDER *2 - VSPACING *2;
    pbox->Height = gbox->Height - HBORDER *2 - HSPACING *2;
    
    	
    /* If we have an indicator box then account for this */
    if (data->pd_IndHeight)
    {
    	indbox->Top 	= pbox->Top;
    	indbox->Left	= pbox->Left;
    	indbox->Width	= pbox->Width;
    	indbox->Height	= data->pd_IndHeight;
    	
    	pbox->Height -= (indbox->Height + HSPACING *2);
	pbox->Top    += (data->pd_IndHeight + HSPACING *2);
    }
    else if (data->pd_IndWidth)
    {
    	indbox->Left	= pbox->Left;
    	indbox->Top	= pbox->Top;
    	indbox->Width	= data->pd_IndWidth;
    	indbox->Height	= pbox->Height;
    	
    	pbox->Width -= (indbox->Width + VSPACING *2);
    	pbox->Left  += (data->pd_IndWidth + VSPACING *2);
    }

    
    /* Compute initial aspect ratio */
    if (pbox->Width > pbox->Height)
    {
    	cols = pbox->Width / pbox->Height;
    	rows = 1;
    	largest  = &cols;
    	smallest = &rows;
    }
    else
    {
    	rows = pbox->Height / pbox->Width;
    	cols = 1;
    	largest  = &rows;
    	smallest = &cols;
    }

    D(bug("Biggest aspect: %d\n", *largest));
    
    ratio = *largest;
    
    smallest_so_far = 0xFFFF;
    
    factor1 = 1 << Colors2Depth(data->pd_NumColors);
    factor2 = 1;
    
    while (factor1 >= factor2)
    {

    	D(bug("trying aspect %dx%d\n", factor1, factor2));

    	fault = abs(ratio - (factor1 / factor2));
    	D(bug("Fault: %d, smallest fault so far: %d\n", fault, smallest_so_far));

    	if (fault < smallest_so_far)
    	{
    	     *largest  = factor1;
    	     *smallest = factor2;
    	     
    	     smallest_so_far = fault;
    	}

	factor1 >>= 1;
	factor2 <<= 1;
    		
    }
    
    data->pd_NumCols = (UBYTE)cols;
    data->pd_NumRows = (UBYTE)rows;
    
    data->pd_ColWidth  = pbox->Width  / data->pd_NumCols;
    data->pd_RowHeight = pbox->Height / data->pd_NumRows;
    
    D(bug("cols=%d, rows=%d\n", data->pd_NumCols, data->pd_NumRows));
    D(bug("colwidth=%d, rowheight=%d\n", data->pd_ColWidth, data->pd_RowHeight));
    
    /* Adjust the pbox's and indbox's height according to leftovers */
    
    leftover_width  = pbox->Width  % data->pd_NumCols;
    leftover_height = pbox->Height % data->pd_NumRows;

    pbox->Width  -= leftover_width;
    pbox->Height -= leftover_height;

    if (data->pd_IndHeight)
    	indbox->Width -= leftover_width;
    else if (data->pd_IndWidth)
    	indbox->Height -= leftover_height;
    
    ReturnInt ("Palette::Layout", IPTR, 0);

}

/**********************************************************************************************/

IPTR GTPalette__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct PaletteData  *data = INST_DATA(cl, g);
    
    struct DrawInfo 	*dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort 	*rp;
    struct IBox     	*gbox = &(data->pd_GadgetBox);
    
    EnterFunc(bug("Palette::Render()\n"));    

    rp = msg->gpr_RPort;
 
    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	    D(bug("Doing total redraw\n"));

	    /* Render gadget label in correct position */
	    renderlabel(GadToolsBase, g, rp, data->pd_LabelPlace);

	    RenderFrame(data, rp, gbox, dri, FALSE, FALSE, cl);

    	    RenderPalette(data, rp, cl);
    	    
    	    /* Render frame aroun ibox */
    	    if (data->pd_IndWidth || data->pd_IndHeight)
    	    {
    	    	RenderFrame(data, rp, &(data->pd_IndicatorBox), dri, TRUE, TRUE, cl);
    	    }
    
    	case GREDRAW_UPDATE:
     	    D(bug("Doing redraw update\n"));
    	    
    	    UpdateActiveColor(data, dri, rp, cl);
    	
    	    if (data->pd_IndWidth || data->pd_IndHeight)
    	    {
    	    	struct IBox *ibox = &(data->pd_IndicatorBox);
		
    	    	SetAPen(rp, GetPalettePen(data, data->pd_Color));
    	    	
    	    	D(bug("Drawing indocator at: (%d, %d, %d, %d)\n",
    	    		ibox->Left, ibox->Top,
    	    		ibox->Left + ibox->Width, ibox->Top + ibox->Height));
    	    	
    	    	RectFill(msg->gpr_RPort,
    	    		ibox->Left + VBORDER + VSPACING,
    	    		ibox->Top  + HBORDER + HSPACING,
    	    	        ibox->Left + ibox->Width  - 1 - VBORDER - VSPACING,
    	    	        ibox->Top +  ibox->Height - 1 - HBORDER - HSPACING);

    	    }
    	    break;
    	    
    	    
    } /* switch (redraw method) */
    
    if (g->Flags & GFLG_DISABLED)
    {
    	DoDisabledPattern(rp,
	    	    	  gbox->Left,
			  gbox->Top,
			  gbox->Left + gbox->Width - 1,
			  gbox->Top + gbox->Height - 1,
			  dri->dri_Pens[SHADOWPEN],
			  GadToolsBase);
    }

    ReturnInt ("Palette::Render", IPTR, 0);
}

/**********************************************************************************************/

IPTR GTPalette__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    struct PaletteData  *data = INST_DATA(cl, o);
    WORD    	    	x, y;
    
    IPTR    	    	retval = 0UL;
    
    EnterFunc(bug("Palette::HitTest()\n"));

    /* One might think that this method is not necessary to implement,
    ** but here is an example to show the opposite:
    ** Consider a 16 color palette with 8 rows and 2 cols.
    ** Gadget is 87 pix. heigh. Each row will then be 10 pix hight + 7 pix
    ** of "nowhere". To prevent anything from happening when this area is
    ** clicked, we rule it out here.
    */
    
    x = msg->gpht_Mouse.X + data->pd_GadgetBox.Left;
    y = msg->gpht_Mouse.Y + data->pd_GadgetBox.Top;

    if (    (x > data->pd_PaletteBox.Left)
	 && (x < data->pd_PaletteBox.Left + data->pd_PaletteBox.Width - 1)
	 && (y > data->pd_PaletteBox.Top)
    	 && (y < data->pd_PaletteBox.Top + data->pd_PaletteBox.Height - 1)
    	 )
    {
    	retval = GMR_GADGETHIT;
    }
    
    ReturnInt ("Palette::HitTest", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTPalette__GM_GOACTIVE(Class *cl, struct Gadget *g, struct gpInput *msg)
{
    struct PaletteData  *data = INST_DATA(cl, g);
    IPTR    	    	retval = 0UL;

    EnterFunc(bug("Palette::GoActive()\n"));
    if (g->Flags & GFLG_DISABLED)
    {
    	retval = GMR_NOREUSE;
    }
    else
    {
    	if (msg->gpi_IEvent)
    	{
    	    UBYTE clicked_color;
    	
    	    /* Set temporary active to the old active */
    	    data->pd_ColorBackup = data->pd_Color;
    	
    	    clicked_color = ComputeColor(data,
	                                 msg->gpi_Mouse.X + data->pd_GadgetBox.Left,
					 msg->gpi_Mouse.Y + data->pd_GadgetBox.Top);
    	
    	    if (clicked_color != data->pd_Color)
    	    {
    	    	struct RastPort *rp;
    	    
    	    	data->pd_Color = clicked_color;
    	
    	    	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    	    	{
		    DoMethod((Object *)g, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    
	    	    ReleaseGIRPort(rp);
	    	}
	    }
	
	    retval = GMR_MEACTIVE;
	    
        } /* if (gadget activated is a result of user input) */        
        else
	{
    	    retval = GMR_NOREUSE;
	}
	    
    } /* if (gadget isn't disabled) */
    
    ReturnInt("Palette::GoActive", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTPalette__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct PaletteData  *data = INST_DATA(cl, o);
    struct InputEvent 	*ie = msg->gpi_IEvent;
    IPTR    	    	retval = 0UL;
    
    EnterFunc(bug("Palette::HandleInput\n"));
    
    retval = GMR_MEACTIVE;
    
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {	   
    	WORD x = msg->gpi_Mouse.X + data->pd_GadgetBox.Left;
	WORD y = msg->gpi_Mouse.Y + data->pd_GadgetBox.Top;
	
	if (x <= data->pd_PaletteBox.Left) x = data->pd_PaletteBox.Left + 1;
	if (y <= data->pd_PaletteBox.Top)  y = data->pd_PaletteBox.Top + 1;
	if (x >= data->pd_PaletteBox.Left + data->pd_PaletteBox.Width - 1)
		x = data->pd_PaletteBox.Left + data->pd_PaletteBox.Width - 2;
	if (y >= data->pd_PaletteBox.Top + data->pd_PaletteBox.Height - 1)
		y = data->pd_PaletteBox.Top + data->pd_PaletteBox.Height - 2;
		 
    	switch (ie->ie_Code)
    	{
    	    case SELECTUP:
	    {
		/* If the button was released outside the gadget, then
		** go back to old state --> no longer: stegerg
		*/

		D(bug("IECLASS_RAWMOUSE: SELECTUP\n"));

		#if 0		     
		if (!InsidePalette(data, x, y))
    	    	{
    	    	    struct RastPort *rp;
    	    	     	
    	    	    /* Left released outside of gadget area, go back
    	    	    ** to old state
    	    	    */
    	    	    data->pd_Color = data->pd_ColorBackup;
    	    	    D(bug("Left released outside gadget\n"));

    	    	    if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    	    	    {
    	    	     	DoMethod(o, GM_RENDER, msg->gpi_GInfo, rp, GREDRAW_UPDATE);
    	    	     	    
    	    	     	ReleaseGIRPort(rp);
    	    	    }
    	    	}
    	    	else
    	    	{
		#endif
    	    	
		    D(bug("Left released inside gadget, color=%d\n", GetPalettePen(data, data->pd_Color)));
    	    	    *(msg->gpi_Termination) = GetPalettePen(data, data->pd_Color);
    	    	    retval = GMR_VERIFY;
    	    	
		#if 0
		}
    	    	#endif
		
    	    	retval |= GMR_NOREUSE;
    	    	break;
    	    }
	    
    	    case IECODE_NOBUTTON:
	    {

    	    	UBYTE over_color;
    	    
    	    	D(bug("IECLASS_POINTERPOS\n"));
    	    	
    	    	if (InsidePalette(data, x, y))
    	    	{
    	    	
    	    	    over_color = ComputeColor(data, x, y);
    	    
    	   	    if (over_color != data->pd_Color)
    	    	    {
    	    	    	struct RastPort *rp;

    	    	    	data->pd_Color = over_color;
    	    	    	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    	    	    	{
	    	    	    DoMethod(o, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    
	    	    	    ReleaseGIRPort(rp);
	    	    	}
			
    	    	    } /* if (mouse is over a different color) */
    	    	    
    	    	} /* if (mouse is over gadget) */
    	    
    	    	retval = GMR_MEACTIVE;
    	    
    	    } break;
    	

    	    case MENUUP:
	    {
    	    	/* Right released on gadget, go back to old state */
    	    	
    	    	struct RastPort *rp;
    	    	
    	    	data->pd_Color = data->pd_ColorBackup;
    	    	D(bug("Right mouse pushed \n"));

    	    	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    	    	{
    	    	    DoMethod(o, GM_RENDER, (IPTR) msg->gpi_GInfo, (IPTR) rp, GREDRAW_UPDATE);
    	    	     	    
    	    	    ReleaseGIRPort(rp);
    	    	}

    	    	retval = GMR_NOREUSE;
		break;
		
    	    }
    	    	
    	} /* switch (ie->ie_Code) */
    	
    } /* if (mouse event) */
    
    ReturnInt("Palette::HandleInput", IPTR, retval);
}

/**********************************************************************************************/

IPTR GTPalette__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    retval += (IPTR)palette_set(cl, o, msg);
    /* If we have been subclassed, OM_UPDATE should not cause a GM_RENDER
     *because it would circumvent the subclass from fully overriding it.
     *The check of cl == OCLASS(o) should fail if we have been
     *subclassed, and we have gotten here via DoSuperMethodA().
     */
    if ( retval && ((msg->MethodID != OM_UPDATE) || (cl == OCLASS(o))) )
    {
	struct GadgetInfo *gi = msg->ops_GInfo;

	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);

	    if (rp)
	    {		        
		DoMethod(o, 
			 GM_RENDER,
			 (IPTR) gi,
			 (IPTR) rp,
			 FindTagItem(GA_Disabled, msg->ops_AttrList) ? GREDRAW_REDRAW : GREDRAW_UPDATE
		);

		ReleaseGIRPort(rp);
		
	    } /* if */
	    
	} /* if */
		
    } /* if */
    
    return retval;
}

/**********************************************************************************************/
