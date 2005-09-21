/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>

#include <string.h>
#include <math.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL CYCLE CLASS **************************************************/

#define CYCLEIMAGEWIDTH 19
#define ARROWWIDTH  	7
#define ARROWHEIGHT 	4

#define ARROW_DOWN  	1
#define ARROW_UP    	2

/***********************************************************************************/

IPTR AslColorPicker__OM_SET(Class * cl, struct Gadget * g, struct opSet *msg);

/***********************************************************************************/

static void RenderObject_Update(Class *cl, struct Gadget *g, struct GadgetInfo *gi)
{
    struct RastPort *rp;
    
    if ((rp = ObtainGIRPort(gi)))
    {
        DoMethod((Object *)g, GM_RENDER,
		              (IPTR) gi,
		              (IPTR) rp,
		              GREDRAW_UPDATE
	);

        ReleaseGIRPort(rp);
    }
}

/***********************************************************************************/

static void DrawArrow(Class *cl, struct RastPort *rp, WORD x1, WORD y1, WORD type)
{
    WORD dy = 1;
    
    if (type == ARROW_UP)
    {
        y1 += 3; dy = -1;
    }
    
    RectFill(rp, x1, y1, x1 + 6, y1); y1 += dy;
    RectFill(rp, x1 + 1, y1, x1 + 5, y1); y1 += dy;
    RectFill(rp, x1 + 2, y1, x1 + 4, y1); y1 += dy;
    
    WritePixel(rp, x1 + 3, y1);
}

/***********************************************************************************/

static void DrawCell(Class *cl, struct AslColorPickerData  *data, WORD index)
{
    WORD    	    	    	x1, y1, x2, y2, col, row;
    
    row = index % data->columns;
    col = index / data->columns;
    
    x1 = data->menux1 + row * (data->cellwidth + data->cellspacex);
    y1 = data->menuy1 + col * (data->cellheight + data->cellspacey);
    
    x2 = x1 + data->cellwidth - 1;
    y2 = y1 + data->cellheight - 1;
    
    SetDrMd(data->rp, JAM1);
    SetAPen(data->rp, data->colortable ? data->colortable[index] : index);
    RectFill(data->rp, x1, y1, x2, y2);
}

/***********************************************************************************/

static void DrawCellMark(Class *cl, struct AslColorPickerData  *data, struct DrawInfo *dri, WORD index, BOOL selected)
{
    WORD    	    	    	x1, y1, x2, y2, col, row;
    
    row = index % data->columns;
    col = index / data->columns;
    
    x1 = data->menux1 + row * (data->cellwidth + data->cellspacex) - 2;
    y1 = data->menuy1 + col * (data->cellheight + data->cellspacey) - 2;
    
    x2 = x1 + data->cellwidth - 1 + 4;
    y2 = y1 + data->cellheight - 1 + 4;
    
    SetDrMd(data->rp, JAM1);
    SetAPen(data->rp, dri->dri_Pens[selected ? SHADOWPEN : SHINEPEN]);
    
    Move(data->rp, x1, y1);
    Draw(data->rp, x2, y1);
    Draw(data->rp, x2, y2);
    Draw(data->rp, x1, y2);
    Draw(data->rp, x1, y1);
}

/***********************************************************************************/

static void DrawAllCells(Class *cl, struct AslColorPickerData  *data)
{
    WORD    	    	    	i;
    
    for(i = 0; i < data->numcolors; i++)
    {
    	DrawCell(cl, data, i);
    }
}

/***********************************************************************************/

IPTR AslColorPicker__OM_NEW(Class * cl, Object * o, struct opSet *msg)
{
    struct AslColorPickerData  *data;
    struct TagItem 	    	fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, FALSE	   },
	{TAG_DONE    , 0UL	   }
    };

    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);
    if (g)
    {
	data = INST_DATA(cl, g);

	/* We want to get a GM_LAYOUT message, no matter if gadget is GFLG_RELRIGHT/RELBOTTOM/
	   RELWIDTH/RELHEIGHT or not */	   
	g->Flags |= GFLG_RELSPECIAL;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);	
	if (data->frame)
	{
	    AslColorPicker__OM_SET(cl, g, msg);

	} else {

	    CoerceMethod(cl, (Object *)g, OM_DISPOSE);
	    g = NULL;

	}
    };
    
    return (IPTR)g;
}

/***********************************************************************************/

IPTR AslColorPicker__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct AslColorPickerData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/

IPTR AslColorPicker__OM_SET(Class * cl, struct Gadget * g, struct opSet *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, g);
    struct TagItem 	    	*tag;
    const struct TagItem *tstate = msg->ops_AttrList;
    IPTR		    	 retval, tidata;
    
    retval = DoSuperMethod(cl, (Object *)g, OM_SET, (IPTR) msg->ops_AttrList, (IPTR) msg->ops_GInfo);
    
    while((tag = NextTagItem(&tstate)))
    {
        tidata = tag->ti_Data;
	
        switch(tag->ti_Tag)
	{
            case ASLCP_Color:
	        data->color = tidata;
	        retval += 1;
		break;
	
	    case ASLCP_ColorTable:
	        data->colortable = (UBYTE *)tidata;
		retval += 1;
		break;
    	    
	    case ASLCP_NumColors:
	    	data->numcolors = tidata;
	    	retval += 1;
		break;
		
	} /* switch(tag->ti_Tag) */
	 
    } /* while((tag = NextTagItem(&tstate))) */ 
    
    if (retval)
    {
        RenderObject_Update(cl, g, msg->ops_GInfo);
    }
    
    return retval;    
}


/***********************************************************************************/

IPTR AslColorPicker__OM_GET(Class * cl, Object * o, struct opGet *msg)
{
    struct AslColorPickerData *data = INST_DATA(cl, o);
    IPTR    	    	       retval = 1;
    
    switch(msg->opg_AttrID)
    {
        case ASLCP_Color:
	    *msg->opg_Storage = data->color;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
    }
    
    return retval;
}

/***********************************************************************************/

IPTR AslColorPicker__GM_RENDER(Class * cl, struct Gadget * g, struct gpRender *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, g);
    struct RastPort	    	*rp   = msg->gpr_RPort;
    struct DrawInfo 	    	*dri  = msg->gpr_GInfo->gi_DrInfo;
    WORD 		    	 gadx, gady, gadw, gadh, x, y, y2, a1, a2; 
    
    if (rp)
    {
	struct TagItem im_tags[] =
	{
	    {IA_Width	, 0	},
	    {IA_Height	, 0	},
	    {TAG_DONE		}
	};
 
        getgadgetcoords(g, msg->gpr_GInfo, &gadx, &gady, &gadw, &gadh);

    	if (msg->gpr_Redraw == GREDRAW_REDRAW)
	{
	    im_tags[0].ti_Data = gadw;
	    im_tags[1].ti_Data = gadh;

	    SetAttrsA(data->frame, im_tags);

	    DrawImageState(msg->gpr_RPort,
			   (struct Image *)data->frame,
			   gadx,
			   gady,
			   (g->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
			   msg->gpr_GInfo->gi_DrInfo);

	    x = gadx + gadw - CYCLEIMAGEWIDTH;
	    y = gady + 2;
	    y2 = gady + gadh - 1 - 2;

	    /* separator bar */

	    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
	    RectFill(rp, x + 1, y , x + 1, y2);
	    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	    RectFill(rp, x, y, x, y2);

	    y = gady + (gadh - 4) / 2;
	    x += 6;

	    DrawArrow(cl, rp, x, y, ARROW_DOWN);
    	}
	
    	/* color */
	
	a1 = gadx + 3;
	a2 = gadx + gadw - CYCLEIMAGEWIDTH - 3;
    	
	SetAPen(rp, data->color);
	RectFill(rp, a1, gady + 3, a2, gady + gadh - 1 - 3);
	
    } /* if (rp) */
    
    return (IPTR)0;
}

/***********************************************************************************/

IPTR AslColorPicker__GM_GOACTIVE(Class * cl, struct Gadget * g, struct gpInput *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, g);
    struct DrawInfo 	    	*dri = msg->gpi_GInfo->gi_DrInfo;
    WORD 		    	 x, y, x2, y2, gadx, gady, gadw, gadh;   
    IPTR 		    	 rc = GMR_MEACTIVE;

    if (!msg->gpi_IEvent || !data->numcolors) return GMR_NOREUSE;

    data->sentgadgetup = FALSE;
    
    getgadgetcoords(g, msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);

    data->borderleft 	= 4;
    data->bordertop  	= 4;
    data->borderright 	= 4;
    data->borderbottom  = 4;
    
    if (!data->colortable && data->numcolors > (1L << dri->dri_Depth))
    {
    	data->numcolors = 1L << dri->dri_Depth;
    }
#ifdef AROS_NOFPU
    {
        ULONG n = 0;
        /*
         * Do a sqrt 'by hand' 
         */
        while ((n*n) <= data->numcolors) {
            n++;
        }
        if ((n * n) > data->numcolors) {
            n--;
        }
        data->rows = n;
    }
#else
    data->rows = (WORD)sqrt((double)data->numcolors);
#endif
    data->columns = (data->numcolors + data->rows - 1) / data->rows;
    
    data->cellspacex = 3;
    data->cellspacey = 3;
    
    data->cellwidth = dri->dri_Font->tf_YSize;
    data->cellheight = data->cellwidth;
    
    data->menuwidth = data->columns * data->cellwidth +
    	    	      (data->columns - 1) * data->cellspacex +
		      data->borderleft + data->borderright;
		      
    data->menuheight = data->rows * data->cellheight +
    	    	       (data->rows - 1) * data->cellspacey +
		       data->bordertop + data->borderbottom;
    
    data->menux1 = data->borderleft;
    data->menuy1 = data->bordertop;
    data->menux2 = data->menuwidth - data->borderright - 1;
    data->menuy2 = data->menuheight - data->borderbottom - 1;
    
    
    x = msg->gpi_GInfo->gi_Window->MouseX;

    x = msg->gpi_GInfo->gi_Window->LeftEdge + gadx;
    y = msg->gpi_GInfo->gi_Window->TopEdge + gady + gadh;

    if (x < 0) x = 0;
    if ((x + data->menuwidth) > msg->gpi_GInfo->gi_Screen->Width)
    {
	x = msg->gpi_GInfo->gi_Screen->Width - data->menuwidth;
    }

    if (y < 0) y = 0;
    if ((y + data->menuheight) > msg->gpi_GInfo->gi_Screen->Height)
    {
	y = msg->gpi_GInfo->gi_Screen->Height - data->menuheight;
    }

    x2 = x + data->menuwidth - 1;
    y2 = y + data->menuheight - 1;

    if ((data->popupwindow = OpenWindowTags(0,WA_CustomScreen	, (IPTR) msg->gpi_GInfo->gi_Screen , 
					      WA_Left	    	, x 	    	    	    ,
					      WA_Top	    	, y 	    	    	    ,
					      WA_Width	    	, data->menuwidth   	    ,
					      WA_Height     	, data->menuheight  	    ,
					      WA_Flags	    	, WFLG_BORDERLESS   	    ,
					      WA_BackFill   	, (IPTR) LAYERS_NOBACKFILL 	    ,
					      TAG_DONE)))
    {
	data->rp = data->popupwindow->RPort;
 
 	data->layerx1 = x;  data->layery1 = y;
	data->layerx2 = x2; data->layery2 = y2;

	data->selected = -1;
	SetDrMd(data->rp, JAM1);

	x = 0;
	y = 0;
	x2 = x + data->menuwidth - 1;
	y2 = y + data->menuheight - 1;

	SetAPen(data->rp, dri->dri_Pens[SHADOWPEN]);	
	RectFill(data->rp, x, y, x2, y);
	RectFill(data->rp, x2, y + 1, x2, y2);
	RectFill(data->rp, x, y2, x2 - 1, y2);
	RectFill(data->rp, x, y + 1, x, y2 - 1);

	SetAPen (data->rp, dri->dri_Pens[SHINEPEN]);
	RectFill(data->rp, x + 1, y  + 1, x2  - 1, y2 - 1);
	
	DrawAllCells(cl, data);
    }
    else
    {
    	rc = GMR_NOREUSE;
    }
	
    return rc;
}

/***********************************************************************************/

IPTR AslColorPicker__GM_HANDLEINPUT(Class * cl, struct Gadget * g, struct gpInput *msg)
{
    struct AslColorPickerData  *data = INST_DATA(cl, g);
    WORD 		    	gadx, gady, gadw, gadh, x, y, sel;
    IPTR 		    	rc = GMR_MEACTIVE;

    getgadgetcoords(g, msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);
    
    switch(msg->gpi_IEvent->ie_Class)
    {
    	case IECLASS_RAWMOUSE:
	    if (msg->gpi_IEvent->ie_Code == SELECTUP)
	    {
	    	rc = GMR_NOREUSE;
		if (data->selected != -1)
		{
		    data->color = data->colortable ? data->colortable[data->selected] : data->selected;
		    data->sentgadgetup = TRUE;
		    
		    rc |= GMR_VERIFY;
		    *msg->gpi_Termination = data->color;
		}
		break;
	    }
	    else if (msg->gpi_IEvent->ie_Code == IECODE_NOBUTTON)
	    {
	    	/* fall through to IECLASS_TIMER */
	    }
	    else
	    {
	    	break;
	    }
	    /* fall through */
	    
	case IECLASS_TIMER:
	    x = msg->gpi_GInfo->gi_Screen->MouseX - data->layerx1;
	    y = msg->gpi_GInfo->gi_Screen->MouseY - data->layery1;
	    
	    if ((x < data->menux1) || (x > data->menux2) ||
	        (y < data->menuy1) || (y > data->menuy2))
	    {
	    	if (data->selected != -1)
		{
		    DrawCellMark(cl, data, msg->gpi_GInfo->gi_DrInfo, data->selected, FALSE);
		    data->selected = -1;
    		}		
	    }
	    else
	    {
	    	x -= data->menux1;
		y -= data->menuy1;
		
		sel = data->columns * (y / (data->cellheight + data->cellspacey)) +
		      x / (data->cellwidth + data->cellspacex);
		      
		if (sel < 0) sel = 0; /* paranoia */
		
		if (sel != data->selected)
		{
		    if (data->selected != -1)
		    {
		    	DrawCellMark(cl, data, msg->gpi_GInfo->gi_DrInfo, data->selected, FALSE);
			data->selected = -1;
		    }
		    
		    if (sel < data->numcolors)
		    {
		    	data->selected = sel;
		    	DrawCellMark(cl, data, msg->gpi_GInfo->gi_DrInfo, data->selected, TRUE);		    
		    }
		}
		
	    }    
	    break;
	    
    } /* switch(msg->gpi_IEvent->ie_Class) */
    
    return rc;
    
}

/***********************************************************************************/

IPTR AslColorPicker__GM_GOINACTIVE(Class * cl, struct Gadget * g, struct gpGoInactive *msg)
{
    struct AslColorPickerData *data = INST_DATA(cl, g);
    
    if (data->popupwindow)
    {
        CloseWindow(data->popupwindow);
        data->popupwindow = 0;
    }

    if (data->sentgadgetup)
    {
        RenderObject_Update(cl, g, msg->gpgi_GInfo);
    }
         
    return 0;
}

/***********************************************************************************/
