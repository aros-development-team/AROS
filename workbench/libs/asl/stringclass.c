/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL STRING CLASS **************************************************/

IPTR AslString__OM_NEW(Class * cl, Object * o, struct opSet * msg)
{
    struct AslStringData *data;
    struct TagItem fitags[] =
    {
	{IA_FrameType, FRAME_RIDGE},
	{TAG_DONE, 0UL}
    };
    
    struct ExtGadget *eg = (struct ExtGadget *)DoSuperMethodA(cl, o, (Msg)msg);
    if (eg)
    {
    	data = INST_DATA(cl, eg);

	eg->BoundsLeftEdge = eg->LeftEdge;
	eg->BoundsTopEdge  = eg->TopEdge;
	eg->BoundsWidth    = eg->Width;
	eg->BoundsHeight   = eg->Height;
	eg->MoreFlags |= GMORE_BOUNDS;
	
	eg->LeftEdge += BORDERSTRINGSPACINGX;
	eg->TopEdge  += BORDERSTRINGSPACINGY;
	eg->Width    -= BORDERSTRINGSPACINGX * 2;
	eg->Height   -= BORDERSTRINGSPACINGY * 2;

	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
	if (!data->frame)
	{
	    CoerceMethod(cl, (Object *)eg, OM_DISPOSE);
	    eg = NULL;
	}
    }

    return (IPTR)eg;
}

/***********************************************************************************/

IPTR AslString__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct AslStringData *data;
    IPTR retval;
    
    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);
    
    retval = DoSuperMethodA(cl, o, msg);
    
    return retval;
}

/***********************************************************************************/

IPTR AslString__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct AslStringData *data;
    IPTR retval;
    
    data = INST_DATA(cl, g);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        struct TagItem im_tags[] =
	{
	    {IA_Width	, 0	},
	    {IA_Height	, 0	},
	    {TAG_DONE		}
	};	
	WORD x, y, w, h;
	
	getgadgetcoords(g, msg->gpr_GInfo, &x, &y, &w, &h);
	
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
    
    retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    return retval;
}

/***********************************************************************************/
