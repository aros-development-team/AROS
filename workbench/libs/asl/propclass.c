/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>

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

/********************** ASL PROP CLASS **************************************************/

struct AslPropData
{
    Object *frame;
    LONG    deltafactor;
};

/***********************************************************************************/

static IPTR aslprop_new(Class * cl, Object * o, struct opSet * msg)
{
    struct AslPropData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_BUTTON},
	{IA_EdgesOnly, TRUE 	   },
	{TAG_DONE, 0UL}
    };
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	data = INST_DATA(cl, o);

	EG(o)->BoundsLeftEdge = G(o)->LeftEdge;
	EG(o)->BoundsTopEdge  = G(o)->TopEdge;
	EG(o)->BoundsWidth    = G(o)->Width;
	EG(o)->BoundsHeight   = G(o)->Height;
	EG(o)->MoreFlags |= GMORE_BOUNDS;

	G(o)->LeftEdge += BORDERPROPSPACINGX;
	G(o)->TopEdge  += BORDERPROPSPACINGY;
	G(o)->Width    -= BORDERPROPSPACINGX * 2;
	G(o)->Height   -= BORDERPROPSPACINGY * 2;
		
    	data->deltafactor = 1;
	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (!data->frame)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    }

    return (IPTR)o;
}

/***********************************************************************************/

static IPTR aslprop_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslPropData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

static IPTR aslprop_set(Class * cl, Object * o, struct opSet * msg)
{
    struct AslPropData *data;
    struct TagItem *tstate = msg->ops_AttrList, *ti;
    IPTR retval;
    BOOL rerender = FALSE;
    
    data = INST_DATA(cl, o);
    
    while((ti = NextTagItem(&tstate)))
    {
        switch(ti->ti_Tag)
	{
	    case ASLSC_DeltaFactor:
	    	data->deltafactor = (LONG)ti->ti_Data;
		break;
		
	    case ASLSC_Inc:
	    case ASLSC_Dec:
	        if (((LONG)ti->ti_Data > 0) &&
		    (((struct opUpdate *)msg)->opu_Flags & OPUF_INTERIM))
		{
		    IPTR top, total, visible, newtop;

		    GetAttr(PGA_Top,     o, &top);
		    GetAttr(PGA_Total,   o, &total);
		    GetAttr(PGA_Visible, o, &visible);

		    newtop = top;
		    if (ti->ti_Data == ID_ARROWDEC)
		    {
		        if (newtop < data->deltafactor)
			{
			   newtop = 0;
		        }
    	    	    	else
			{
			    newtop -= data->deltafactor;
			}
			
		    }
		    else
		    {
		        if (top <= total - visible - data->deltafactor)
			{
			    newtop += data->deltafactor;
			}
			else
			{
			    newtop = total - visible;
			}
		    }

		    if (newtop != top)
		    {
		        struct TagItem set_tags [] =
			{
			    {PGA_Top, newtop 	},
			    {TAG_DONE		}
			};
			struct opSet ops;

			ops.MethodID  	  = OM_SET;
			ops.ops_AttrList = set_tags;
			ops.ops_GInfo 	  = msg->ops_GInfo;

			DoMethodA(o, (Msg)&ops);

			/* rerender = TRUE; */
		    }
		     
		} /* if ((ti->ti_Data > 0) && (((struct opUpdate *)msg)->opu_Flags & OPUF_INTERIM)) */
				
	} /* switch(ti->ti_Tag) */

    } /* while((ti = NextTagItem(&tstate))) */
    
    retval = DoSuperMethodA(cl, o, (Msg) msg);
 
    if (rerender)
    {
        struct RastPort *rp;
	
	rp = ObtainGIRPort(msg->ops_GInfo);
    	if (NULL != rp)
	{
	    DoMethod(o, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
	}
    }
      
    return retval;
}

/***********************************************************************************/

static IPTR aslprop_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslPropData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        struct TagItem im_tags[] =
	{
	    {IA_Width	, 0	},
	    {IA_Height	, 0	},
	    {TAG_DONE		}
	};	
	WORD x, y, w, h;
	
	getgadgetcoords(G(o), msg->gpr_GInfo, &x, &y, &w, &h);
	
	x -= BORDERPROPSPACINGX;
	y -= BORDERPROPSPACINGY;
	w += BORDERPROPSPACINGX * 2;
	h += BORDERPROPSPACINGY * 2;
	
	im_tags[0].ti_Data = w;
	im_tags[1].ti_Data = h;
	
	SetAttrsA(data->frame, im_tags);
	
	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       x,
		       y,
		       IDS_NORMAL,
		       msg->gpr_GInfo->gi_DrInfo);

    #if AVOID_FLICKER
    	{
	    struct IBox ibox, fbox;
	    
	    fbox.Left = x;
	    fbox.Top = y;
	    fbox.Width = w;
	    fbox.Height = h;
	    
	    ibox.Left   = x + BORDERPROPSPACINGX;
	    ibox.Top    = y + BORDERPROPSPACINGY;
	    ibox.Width  = w - BORDERPROPSPACINGX * 2;
	    ibox.Height = h - BORDERPROPSPACINGY * 2;
	    
	    PaintInnerFrame(msg->gpr_RPort,
	    	    	    msg->gpr_GInfo->gi_DrInfo,
			    data->frame,
			    &fbox,
			    &ibox,
			    msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN],
			    AslBase);
	    
	}
    #endif
    
    } /* if (msg->gpr_Redraw == GREDRAW_REDRAW) */
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    return retval;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslpropclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = aslprop_new(cl, obj, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    retval = aslprop_dispose(cl, obj, msg);
	    break;
	
	case OM_SET:
	case OM_UPDATE:
	    retval = aslprop_set(cl, obj, (struct opSet *)msg);
	    break;
	
	case GM_RENDER:
	    retval = aslprop_render(cl, obj, (struct gpRender *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/***********************************************************************************/

#undef AslBase

Class *makeaslpropclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslpropclass)
	return AslBase->aslpropclass;

    cl = MakeClass(NULL, PROPGCLASS, NULL, sizeof(struct AslPropData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslpropclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslpropclass = cl;

    return cl;
}

/***********************************************************************************/
