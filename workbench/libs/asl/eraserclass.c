/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>


#ifdef __MORPHOS__
#ifndef NewRectRegion
#define NewRectRegion(_MinX,_MinY,_MaxX,_MaxY) \
({ struct Region *_region; \
	if ((_region = NewRegion())) \
	{ struct Rectangle _rect; \
		_rect.MinX = _MinX; \
		_rect.MinY = _MinY; \
		_rect.MaxX = _MaxX; \
		_rect.MaxY = _MaxY; \
		if (!OrRectRegion(_region, &_rect)) { DisposeRegion(_region); _region = NULL; } \
	} \
	_region; \
})
#endif
#endif


#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL FONTPREVIEW CLASS **************************************************/

struct AslEraserData
{
    UBYTE dummy;
};

/***********************************************************************************/

static IPTR asleraser_new(Class * cl, Object * o, struct opSet * msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	G(o)->Flags |= GFLG_RELSPECIAL;
	
	G(o)->LeftEdge = 20000;
	G(o)->TopEdge  = 20000;
	G(o)->Width    = 1;
	G(o)->Height   = 1;
		
    } /* if (o) */

    return (IPTR)o;
}

/***********************************************************************************/

static IPTR asleraser_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    return 0;
}


/***********************************************************************************/

static IPTR asleraser_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct AslEraserData *data;
    
    WORD x, y, w, h, x2, y2;

    data = INST_DATA(cl, o);
    
    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
    	struct Window *win = msg->gpr_GInfo->gi_Window;
    	struct RastPort *rp = msg->gpr_RPort;
    	struct Region *clip;
	
    	if ((clip = NewRectRegion(win->BorderLeft,
	    	    	    	  win->BorderTop,
				  win->Width - 1 - win->BorderRight,
				  win->Height - 1 - win->BorderBottom)))
	{
	    struct Gadget *g = win->FirstGadget;
	    struct RegionRectangle *rr;
	    
	    while(g)
	    {
	    	struct Rectangle rect;
		
    	    	getgadgetbounds(g, msg->gpr_GInfo, &x, &y, &w, &h);
		
		rect.MinX = x;
		rect.MinY = y;
		rect.MaxX = x + w - 1;
		rect.MaxY = y + h - 1;
		
		ClearRectRegion(clip, &rect);
		
	    	g = g->NextGadget;
	    }
	    
	    
	    SetABPenDrMd(rp, msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN], 0, JAM2);
	    rr = clip->RegionRectangle;
	    while(rr)
	    {
	    	x  = clip->bounds.MinX + rr->bounds.MinX;
	    	y  = clip->bounds.MinY + rr->bounds.MinY;
	    	x2 = clip->bounds.MinX + rr->bounds.MaxX;
	    	y2 = clip->bounds.MinY + rr->bounds.MaxY;
	    	
		RectFill(rp, x, y, x2, y2);
		
	    	rr = rr->Next;
	    }
	    
	    DisposeRegion(clip);
	    
	}
	
    } /* if (msg->gpr_Redraw == GREDRAW_REDRAW) */
      
    return 0;
}

/***********************************************************************************/

AROS_UFH3S(IPTR, dispatch_asleraserclass,
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
	    retval = asleraser_new(cl, obj, (struct opSet *)msg);
	    break;
	    
	case GM_HITTEST:
	    retval = asleraser_hittest(cl, obj, (struct gpHitTest *)msg);
	    break;
	
	case GM_RENDER:
	    retval = asleraser_render(cl, obj, (struct gpRender *)msg);
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

Class *makeasleraserclass(struct AslBase_intern * AslBase)
{
    Class *cl = NULL;

    if (AslBase->asleraserclass)
	return AslBase->asleraserclass;

    cl = MakeClass(NULL, GADGETCLASS, NULL, sizeof(struct AslEraserData), 0UL);

    if (!cl)
	return NULL;
	
    cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_asleraserclass);
    cl->cl_Dispatcher.h_SubEntry = NULL;
    cl->cl_UserData = (IPTR) AslBase;

    AslBase->asleraserclass = cl;

    return cl;
}

/***********************************************************************************/

