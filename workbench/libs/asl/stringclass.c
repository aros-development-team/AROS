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

/********************** ASL STRING CLASS **************************************************/

struct AslStringData
{
    Object *frame;
};

/***********************************************************************************/

static IPTR aslstring_new(Class * cl, Object * o, struct opSet * msg)
{
    struct AslStringData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_RIDGE},
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
	
	G(o)->LeftEdge += BORDERSTRINGSPACINGX;
	G(o)->TopEdge  += BORDERSTRINGSPACINGY;
	G(o)->Width    -= BORDERSTRINGSPACINGX * 2;
	G(o)->Height   -= BORDERSTRINGSPACINGY * 2;

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

static IPTR aslstring_dispose(Class * cl, Object * o, Msg msg)
{
    struct AslStringData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

static IPTR aslstring_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslStringData *data;
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
	
	x -= BORDERSTRINGSPACINGX;
	y -= BORDERSTRINGSPACINGY;
	w += BORDERSTRINGSPACINGX * 2;
	h += BORDERSTRINGSPACINGY * 2;
	
	im_tags[0].ti_Data = w;
	im_tags[1].ti_Data = h;
	
	SetAttrsA(data->frame, im_tags);
	
	DrawImageState(msg->gpr_RPort,
		       (struct Image *)data->frame,
		       x,
		       y,
		       IDS_NORMAL,
		       msg->gpr_GInfo->gi_DrInfo);

    } /* if (msg->gpr_Redraw == GREDRAW_REDRAW) */
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    return retval;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_aslstringclass,
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
	    retval = aslstring_new(cl, obj, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    retval = aslstring_dispose(cl, obj, msg);
	    break;
	    
	case GM_RENDER:
	    retval = aslstring_render(cl, obj, (struct gpRender *)msg);
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

Class *makeaslstringclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->aslstringclass)
	return AslBase->aslstringclass;

   cl = MakeClass(NULL, STRGCLASS, NULL, sizeof(struct AslStringData), 0UL);
    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_aslstringclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->aslstringclass = cl;

    return cl;
}

/***********************************************************************************/
