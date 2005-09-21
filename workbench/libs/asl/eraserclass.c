/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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


#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

/********************** ASL ERASER CLASS **************************************************/

IPTR AslEraser__OM_NEW(Class * cl, Object * o, struct opSet * msg)
{
    struct Gadget *g = (struct Gadget *)DoSuperMethodA(cl, o, (Msg)msg);

    if (g)
    {
    	g->Flags |= GFLG_RELSPECIAL;
	
	g->LeftEdge = 20000;
	g->TopEdge  = 20000;
	g->Width    = 1;
	g->Height   = 1;
		
    } /* if (g) */

    return (IPTR)g;
}

/***********************************************************************************/

IPTR AslEraser__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    return 0;
}


/***********************************************************************************/

IPTR AslEraser__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    WORD x, y, w, h, x2, y2;

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
