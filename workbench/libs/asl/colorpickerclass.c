/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

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

struct AslColorPickerData
{
    Object	       *frame;
    struct Window      *popupwindow;
    struct RastPort    *rp;
    UBYTE   	       *colortable;
    WORD   	    	numcolors;
    WORD   	    	color;
    WORD    	    	selected;
    WORD    	    	menuwidth;
    WORD    	    	menuheight;
    WORD    	    	menux1;
    WORD    	    	menuy1;
    WORD    	    	menux2;
    WORD    	    	menuy2;
    WORD    	    	layerx1;
    WORD    	    	layery1;
    WORD    	    	layerx2;
    WORD    	    	layery2;
    WORD    	    	columns;
    WORD    	    	rows;
    WORD    	    	cellwidth;
    WORD    	    	cellheight;
    WORD    	    	cellspacex;
    WORD    	    	cellspacey;
    WORD    	    	borderleft;
    WORD    	    	bordertop;
    WORD    	    	borderright;
    WORD    	    	borderbottom;
    BYTE    	    	sentgadgetup;
};


/***********************************************************************************/

static IPTR aslcolorpicker_set(Class * cl, Object * o, struct opSet *msg);

/***********************************************************************************/

static void RenderObject_Update(Class *cl, Object *o, struct GadgetInfo *gi)
{
    struct RastPort *rp;
    
    if ((rp = ObtainGIRPort(gi)))
    {
        DoMethod(o, GM_RENDER,
		    (IPTR) gi,
		    (IPTR) rp,
		    GREDRAW_UPDATE);
		    
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

static void DrawCell(Class *cl, Object *o, WORD index)
{
    struct AslColorPickerData  *data = INST_DATA(cl, o);
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

static void DrawCellMark(Class *cl, Object *o, struct DrawInfo *dri, WORD index, BOOL selected)
{
    struct AslColorPickerData  *data = INST_DATA(cl, o);
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

static void DrawAllCells(Class *cl, Object *o)
{
    struct AslColorPickerData  *data = INST_DATA(cl, o);
    WORD    	    	    	i;
    
    for(i = 0; i < data->numcolors; i++)
    {
    	DrawCell(cl, o, i);
    }
}

/***********************************************************************************/

static IPTR aslcolorpicker_new(Class * cl, Object * o, struct opSet *msg)
{
    struct AslColorPickerData  *data;
    struct TagItem 	    	fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, FALSE	   },
	{TAG_DONE    , 0UL	   }
    };
    IPTR 		        rc = 0;
    
    if ((o = (Object *)DoSuperMethodA(cl, o, (Msg)msg)))
    {
	data = INST_DATA(cl, o);

	/* We want to get a GM_LAYOUT message, no matter if gadget is GFLG_RELRIGHT/RELBOTTOM/
	   RELWIDTH/RELHEIGHT or not */	   
	G(o)->Flags |= GFLG_RELSPECIAL;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);	
	if (data->frame)
	{
	    aslcolorpicker_set(cl, o, msg);
	    
	    rc = (ULONG)o;
	} else {

	    CoerceMethod(cl, o, OM_DISPOSE);

	}
    };
    
    return rc;
}

/***********************************************************************************/

static IPTR aslcolorpicker_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslColorPickerData *data = INST_DATA(cl, o);

    if (data->frame) DisposeObject(data->frame);
    
    return DoSuperMethodA(cl, o, msg);
}

/***********************************************************************************/

static IPTR aslcolorpicker_set(Class * cl, Object * o, struct opSet *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, o);
    struct TagItem 	    	*tag, *tstate = msg->ops_AttrList;
    IPTR		    	 retval, tidata;
    
    retval = DoSuperMethod(cl, o, OM_SET, (IPTR) msg->ops_AttrList, (IPTR) msg->ops_GInfo);
    
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
        RenderObject_Update(cl, o, msg->ops_GInfo);
    }
    
    return retval;    
}


/***********************************************************************************/

static IPTR aslcolorpicker_get(Class * cl, Object * o, struct opGet *msg)
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

static IPTR aslcolorpicker_render(Class * cl, Object * o, struct gpRender *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, o);
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
 
        getgadgetcoords(G(o), msg->gpr_GInfo, &gadx, &gady, &gadw, &gadh);

    	if (msg->gpr_Redraw == GREDRAW_REDRAW)
	{
	    im_tags[0].ti_Data = gadw;
	    im_tags[1].ti_Data = gadh;

	    SetAttrsA(data->frame, im_tags);

	    DrawImageState(msg->gpr_RPort,
			   (struct Image *)data->frame,
			   gadx,
			   gady,
			   (G(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
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
    
    return 0;
}

/***********************************************************************************/

static IPTR aslcolorpicker_goactive(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslColorPickerData 	*data = INST_DATA(cl, o);
    struct DrawInfo 	    	*dri = msg->gpi_GInfo->gi_DrInfo;
    WORD 		    	 x, y, x2, y2, gadx, gady, gadw, gadh;   
    IPTR 		    	 rc = GMR_MEACTIVE;

    if (!msg->gpi_IEvent || !data->numcolors) return GMR_NOREUSE;

    data->sentgadgetup = FALSE;
    
    getgadgetcoords(G(o), msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);

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
	
	DrawAllCells(cl, o);
    }
    else
    {
    	rc = GMR_NOREUSE;
    }
	
    return rc;
}

/***********************************************************************************/

static IPTR aslcolorpicker_handleinput(Class * cl, Object * o, struct gpInput *msg)
{
    struct AslColorPickerData  *data = INST_DATA(cl, o);
    WORD 		    	gadx, gady, gadw, gadh, x, y, sel;
    IPTR 		    	rc = GMR_MEACTIVE;

    getgadgetcoords(G(o), msg->gpi_GInfo, &gadx, &gady, &gadw, &gadh);
    
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
		    DrawCellMark(cl, o, msg->gpi_GInfo->gi_DrInfo, data->selected, FALSE);
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
		    	DrawCellMark(cl, o, msg->gpi_GInfo->gi_DrInfo, data->selected, FALSE);
			data->selected = -1;
		    }
		    
		    if (sel < data->numcolors)
		    {
		    	data->selected = sel;
		    	DrawCellMark(cl, o, msg->gpi_GInfo->gi_DrInfo, data->selected, TRUE);		    
		    }
		}
		
	    }    
	    break;
	    
    } /* switch(msg->gpi_IEvent->ie_Class) */
    
    return rc;
    
}

/***********************************************************************************/

static IPTR aslcolorpicker_goinactive(Class * cl, Object * o, struct gpGoInactive *msg)
{
    struct AslColorPickerData *data = INST_DATA(cl, o);
    
    if (data->popupwindow)
    {
        CloseWindow(data->popupwindow);
        data->popupwindow = 0;
    }

    if (data->sentgadgetup)
    {
        RenderObject_Update(cl, o, msg->gpgi_GInfo);
    }
         
    return 0;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslcolorpickerclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, o, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = aslcolorpicker_new(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    retval = aslcolorpicker_dispose(cl, o, msg);
	    break;

	case OM_SET:
	    retval = aslcolorpicker_set(cl, o, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = aslcolorpicker_get(cl, o, (struct opGet *)msg);
	    break;
	
	case GM_RENDER:
	    retval = aslcolorpicker_render(cl, o, (struct gpRender *)msg);
	    break;
	
	case GM_GOACTIVE:
	    retval = aslcolorpicker_goactive(cl, o, (struct gpInput *)msg);
	    break;
	
	case GM_HANDLEINPUT:
	    retval = aslcolorpicker_handleinput(cl, o, (struct gpInput *)msg);
	    break;
	
	case GM_GOINACTIVE:
	    retval = aslcolorpicker_goinactive(cl, o, (struct gpGoInactive *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch (msg->MethodID) */
    
    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslcolorpickerclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslcolorpickerclass)
	return AslBase->aslcolorpickerclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslColorPickerData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslcolorpickerclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslcolorpickerclass = cl;

    return cl;
}

/***********************************************************************************/
