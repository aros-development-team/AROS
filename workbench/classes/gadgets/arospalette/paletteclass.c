/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    AROS Palette gadget for use in gadtools.
*/

#define USE_BOOPSI_STUBS
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <utility/tagitem.h>
#include <gadgets/arospalette.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* abs() */
#include "arospalette_intern.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define AROSPaletteBase ((struct PaletteBase_intern *)(cl->cl_UserData))

#include <clib/boopsistubs.h>


STATIC IPTR _OM_SET(Class *cl, Object *o, struct opSet *msg, BOOL render)
{
    const struct TagItem *tag, *tstate;
    IPTR retval = (IPTR)0;
    struct PaletteData *data = INST_DATA(cl, o);
    
    BOOL labelplace_set = FALSE, relayout = FALSE;
    BOOL colortag_found = FALSE, numcolorstag_found = FALSE;
        
    EnterFunc(bug("Palette::Set()\n"));
    
    for (tstate = msg->ops_AttrList; (tag = NextTagItem(&tstate)); )
    {
    	IPTR tidata = tag->ti_Data;
    	
    	
    	switch (tag->ti_Tag)
    	{
    	case AROSA_Palette_Depth:		/* [ISU] */
	    /* NumColors tag overrides Depth tag! */
	    if (!numcolorstag_found)
	    {	        
    		data->pd_NumColors = (1 << ((UBYTE)tidata));

    		D(bug("Depth initialized to %d\n", tidata));
    		relayout = TRUE;
    		retval++;
	    }
    	    break;
    	    
    	case AROSA_Palette_Color:		/* [IS] */
	    colortag_found = TRUE;
	    
    	    data->pd_OldColor = data->pd_Color;
    	    data->pd_Color = (UBYTE)tidata;
    	    D(bug("Color set to %d\n", tidata));    	    
    	    retval++;
    	    break;
    	    
    	case AROSA_Palette_ColorOffset:		/* [I] */
    	    data->pd_ColorOffset = (UBYTE)tidata;
    	    D(bug("ColorOffset initialized to %d\n", tidata));
    	    retval++;
    	    break;
    	    
    	case AROSA_Palette_IndicatorWidth:	/* [I] */
    	    data->pd_IndWidth = (UWORD)tidata;
    	    D(bug("Indicatorwidth set to %d\n", tidata));
    	    
    	    /* If palette has an indictor on left, GA_LabelPlace
    	    ** defaults to GV_LabelPlace_Left
    	    */
    	    if (!labelplace_set)
    	 	data->pd_LabelPlace = GV_LabelPlace_Left;
    	    break;

    	case AROSA_Palette_IndicatorHeight:	/* [I] */
    	    data->pd_IndHeight = (UWORD)tidata;
    	    D(bug("Indicatorheight set to %d\n", tidata));
    	    break;
    	    
    	case GA_LabelPlace:			/* [I] */
    	    data->pd_LabelPlace = (LONG)tidata;
    	    D(bug("Labelplace set to %d\n", tidata));
    	    
    	    labelplace_set = TRUE;
    	    break;
    	    
    	case GA_TextAttr:			/* [I] */
    	    data->pd_TAttr = (struct TextAttr *)tidata;
    	    D(bug("TextAttr set to %s %d\n",
    	    	data->pd_TAttr->ta_Name, data->pd_TAttr->ta_YSize));
    	    break;
    	    
    	case AROSA_Palette_ColorTable:
    	    data->pd_ColorTable = (UBYTE *)tidata;
    	    break;
    	    
    	case AROSA_Palette_NumColors:
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
    	DoMethod(o, GM_LAYOUT, (IPTR)msg->ops_GInfo, FALSE);
    }
    
    if ( render )
    {
	struct GadgetInfo *gi = ((struct opSet *)msg)->ops_GInfo;

	if (gi)
	{
	    struct RastPort *rp = ObtainGIRPort(gi);

	    if (rp)
	    {		        
		DoMethod(o, 
			 GM_RENDER,
			 (IPTR)gi,
			 (IPTR)rp,
			 FindTagItem(GA_Disabled, ((struct opSet *)msg)->ops_AttrList) ? GREDRAW_REDRAW : GREDRAW_UPDATE
		);
				 
		ReleaseGIRPort(rp);
	    }
	}
    }

    ReturnPtr ("Palette::Set", IPTR, retval);
}

/************************
**  Palette::Update()  **
************************/
IPTR AROSPalette__OM_UPDATE(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);
    /* Only rerender when we are not subclassed */
    return retval + _OM_SET(cl, o, msg, cl == OCLASS(o));
}

/*********************
**  Palette::Set()  **
*********************/
IPTR AROSPalette__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);
    /* Always rerender for the OM_SET method */
    return retval + _OM_SET(cl, o, msg, TRUE);
}

/*********************
**  Palette::New()  **
*********************/
Object *AROSPalette__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct opSet ops;
    struct TagItem tags[] =
    {
	{GA_RelSpecial, TRUE},
	{TAG_MORE, 0}
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

    	
    	_OM_SET(cl, o, msg, FALSE);
    	
    
    }
    ReturnPtr ("Palette::New", Object *, o);
}

/*********************
**  Palette::Get()  **
*********************/
IPTR AROSPalette__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PaletteData *data = INST_DATA(cl, o);
    IPTR retval = 1UL;
   
    switch(msg->opg_AttrID)
    {
   	case AROSA_Palette_Color:
	    *msg->opg_Storage = (IPTR)GetPalettePen(data, data->pd_Color);
	    break;
	
	case AROSA_Palette_ColorOffset:
	    *msg->opg_Storage = (IPTR)data->pd_ColorOffset;
	    break;
	
	case AROSA_Palette_ColorTable:
	    *msg->opg_Storage = (IPTR)data->pd_ColorTable;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
   
    return retval;
}

/************************
**  Palette::Render()  **
************************/
VOID AROSPalette__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct PaletteData *data = INST_DATA(cl, o);
    
    struct DrawInfo *dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort *rp;
    struct IBox *gbox = &(data->pd_GadgetBox);
    
    EnterFunc(bug("Palette::Render()\n"));    

    rp = msg->gpr_RPort;

   
    switch (msg->gpr_Redraw)
    {
    	case GREDRAW_REDRAW:
    	    D(bug("Doing total redraw\n"));

	    /* Render gadget label in correct position */
	    RenderLabel((struct Gadget *)o, gbox, rp,
    			data->pd_LabelPlace, AROSPaletteBase);

	    RenderFrame(data, rp, gbox, dri, FALSE, FALSE, AROSPaletteBase);

    	    RenderPalette(data, rp, AROSPaletteBase);
    	    
    	    /* Render frame aroun ibox */
    	    if (data->pd_IndWidth || data->pd_IndHeight)
    	    {
    	    	RenderFrame(data, rp, &(data->pd_IndicatorBox), dri, TRUE, TRUE, AROSPaletteBase);
    	    }
    
    	case GREDRAW_UPDATE:
     	    D(bug("Doing redraw update\n"));
    	    
    	    UpdateActiveColor(data, dri, rp, AROSPaletteBase);
    	
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
    
    if (EG(o)->Flags & GFLG_DISABLED)
    {
    	DrawDisabledPattern(rp, gbox, dri->dri_Pens[SHADOWPEN], AROSPaletteBase);
    }
        	
    ReturnVoid("Palette::Render");
}

/*************************
**  Palette::Dispose()  **
*************************/
VOID AROSPalette__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct PaletteData *data = INST_DATA(cl, o);
    
    if (data->pd_Frame) DisposeObject(data->pd_Frame);
    
    DoSuperMethodA(cl, o, msg);
}

/*************************
**  Palette::HitTest()  **
*************************/
IPTR AROSPalette__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    WORD x, y;
    
    IPTR retval = 0UL;
    struct PaletteData *data = INST_DATA(cl, o);
    
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

/************************
**  Palette::GoActive  **
************************/
IPTR AROSPalette__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    IPTR retval = 0UL;
    struct PaletteData *data = INST_DATA(cl, o);

    EnterFunc(bug("Palette::GoActive()\n"));
    if (EG(o)->Flags & GFLG_DISABLED)
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
	    	    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
	    
	    	    ReleaseGIRPort(rp);
	    	}
	    }
	
	    retval = GMR_MEACTIVE;
        } /* if (gadget activated is a result of user input) */
        
        else
    	    retval = GMR_NOREUSE;
    } /* if (gadget isn't disabled) */
    
    ReturnInt("Palette::GoActive", IPTR, retval);
}



/*****************************
**  Palette::HandleInput()  **
*****************************/
IPTR AROSPalette__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct PaletteData *data = INST_DATA(cl, o);
    IPTR retval = 0UL;
    struct InputEvent *ie = msg->gpi_IEvent;
    
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
    	    case SELECTUP: {
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
    	    } break;
    	    	
    	    case IECODE_NOBUTTON: {

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
	    	    	    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
	    
	    	    	    ReleaseGIRPort(rp);
	    	    	}
    	    	    } /* if (mouse is over a different color) */
    	    	    
    	    	} /* if (mouse is over gadget) */
    	    
    	    	retval = GMR_MEACTIVE;
    	    
    	    } break;
    	

    	    case MENUUP: {

    	    	/* Right released on gadget, go back to old state */
    	    	
    	    	struct RastPort *rp;
    	    	
    	    	data->pd_Color = data->pd_ColorBackup;
    	    	D(bug("Right mouse pushed \n"));

    	    	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
    	    	{
    	    	    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
    	    	     	    
    	    	    ReleaseGIRPort(rp);
    	    	}

    	    	retval = GMR_NOREUSE;
    	    } break;
    	    	
    	} /* switch (ie->ie_Code) */
    	
    } /* if (mouse event) */
    
    ReturnInt("Palette::HandleInput", IPTR, retval);
}

/************************
**  Palette::Layout()  **
************************/
VOID AROSPalette__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    
    /* The palette gadget has been resized and we need to update our layout */
    
    struct PaletteData *data = INST_DATA(cl, o);
    struct IBox *gbox   = &(data->pd_GadgetBox),
    		*pbox   = &(data->pd_PaletteBox),
    		*indbox = &(data->pd_IndicatorBox);

    
    UWORD cols, rows;

    WORD factor1, factor2, ratio;
    UWORD fault, smallest_so_far;
    
    UWORD *largest, *smallest;
    
    WORD leftover_width, leftover_height;
    
    EnterFunc(bug("Palette::Layout()\n"));
    
    if (!msg->gpl_GInfo)
    	ReturnVoid("Palette::Layout"); /* We MUST have a GInfo to get screen aspect ratio */
    
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
    GetGadgetIBox(o, msg->gpl_GInfo, gbox);
    D(bug("Got palette ibox: (%d, %d, %d, %d)\n",
    	gbox->Left, gbox->Top, gbox->Width, gbox->Height));
    	
    /* Get the palette box */
    pbox->Left   = gbox->Left + VBORDER + VSPACING;
    pbox->Top    = gbox->Top  + HBORDER + HSPACING;
    pbox->Width  = gbox->Width  - VBORDER * 2 - VSPACING * 2;
    pbox->Height = gbox->Height - HBORDER * 2 - HSPACING * 2;
    
    	
    /* If we have an indicator box then account for this */
    if (data->pd_IndHeight)
    {
    	indbox->Top 	= pbox->Top;
    	indbox->Left	= pbox->Left;
    	indbox->Width	= pbox->Width;
    	indbox->Height	= data->pd_IndHeight;
    	
    	pbox->Height -= (indbox->Height + HSPACING * 2);
	pbox->Top    += (data->pd_IndHeight + HSPACING * 2);
    }
    else if (data->pd_IndWidth)
    {
    	indbox->Left	= pbox->Left;
    	indbox->Top	= pbox->Top;
    	indbox->Width	= data->pd_IndWidth;
    	indbox->Height	= pbox->Height;
    	
    	pbox->Width -= (indbox->Width + VSPACING * 2);
    	pbox->Left  += (data->pd_IndWidth + VSPACING * 2);
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
    
    ReturnVoid("Palette::Layout");
}
