/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

	AROS_LH3(void, DoHookClipRects,

/*  SYNOPSIS */
	AROS_LHA(struct Hook      *, hook,  A0),
	AROS_LHA(struct RastPort  *, rport, A1),
	AROS_LHA(struct Rectangle *, rect,  A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 36, Layers)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{ /* sv */
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    struct BitMap    *bm = rport->BitMap;
    struct Rectangle  rect1;
    struct Rectangle *CurRect;
    struct ClipRect  *cr;
    WORD             BaseX;
    WORD             BaseY;

    D(bug("DoHookClipRects(hook @ $%lx, rport @ $%lx, rect @ $%lx)\n", hook, rport, rect));

    if(hook == (struct Hook *)LAYERS_NOBACKFILL)
    {
	return;
    }

    if(!rport->Layer)
    {
	/* non-layered rastport */

	/* You MUST supply a rect to clip the hook's actions! */
	_CallLayerHook(hook, rport->Layer, rport, rect, rect, 0, 0, LayersBase);
    }
    else
    {
	struct Rectangle *mainrect;

	/* layered rastport */
	LockLayer(0, rport->Layer);

	BaseX = rport->Layer->bounds.MinX - rport->Layer->Scroll_X;
	BaseY = rport->Layer->bounds.MinY - rport->Layer->Scroll_Y;

	mainrect = rect;

	if(rect)
	{
	    mainrect = &rect1;
	    rect1.MinX = rect->MinX + BaseX;
	    rect1.MinY = rect->MinY + BaseY;
	    rect1.MaxX = rect->MaxX + BaseX;
	    rect1.MaxY = rect->MaxY + BaseY;
	}

	/* Consider all ClipRects in this layer */
	for(cr = rport->Layer->ClipRect; cr; cr = cr->Next)
	{
	    struct Rectangle rect2;

	    CurRect = &cr->bounds;

	    if(mainrect)
	    {
		/* If a rect was supplied to DoHookClipRects, we must use this
		   rect to bound (clip) our actions to this rect. */

		/* Get a cross-section between the main rect and this rect (store in rect2) */
		IntersectRects(CurRect, mainrect, &rect2);

		/* Check if there actually _is_ a cross-section. If there isn't
		   one, the Min values will be larger than the Max values */
		if(rect2.MaxY >= rect2.MinY &&
		   rect2.MaxX >= rect2.MinX)
		{
		    /* If there is, this is now our current rectangle */
		    CurRect = &rect2;
		}
		else
		{
		    /* If there is no cross-section, we go to the next ClipRect */

		    continue;
		}
	    }

	    if(!cr->lobs)
	    {
		_CallLayerHook(hook, rport->Layer, rport, CurRect, CurRect, BaseX, BaseY, LayersBase);
		continue;
	    }

	    if(cr->BitMap)
	    {
		struct Rectangle rect3;

		rect3.MaxX = (cr->bounds.MinX & 0xf) - cr->bounds.MinX + CurRect->MaxX;
		rect3.MaxY = CurRect->MaxY -   cr->bounds.MinY;
		rect3.MinX = CurRect->MinX + ((cr->bounds.MinX & 0xf) - cr->bounds.MinX);
		rect3.MinY = CurRect->MinY -   cr->bounds.MinY;

		/* Use the ClipRect's bitmap for this hook */
		rport->BitMap = cr->BitMap;

		_CallLayerHook(hook, rport->Layer, rport, &rect3, CurRect, BaseX, BaseY, LayersBase);

		/* And restore the original RastPort's bitmap */
		rport->BitMap = bm;
	    }
	}

	mainrect = rect;

	if(rport->Layer->SuperBitMap)
	{
	    /* NOTE: SuperBitMap handling is untested! */

	    rport->BitMap = rport->Layer->SuperBitMap;

	    cr = rport->Layer->SuperClipRect;

	    while( (cr = cr->Next))
	    {
		CurRect = &cr->bounds;

		if(mainrect)
		{
		    IntersectRects(&cr->bounds, mainrect, &rect1);

		    if(rect1.MaxY >= rect1.MinY &&
		       rect1.MaxX >= rect1.MinX)
		    {
			CurRect = &rect1;
		    }
		    else
			continue;
		}

		_CallLayerHook(hook, rport->Layer, rport, CurRect, CurRect, 0, 0, LayersBase);
	    }
	}
    }

    /* Restore rport's original bitmap */
    rport->BitMap = bm;

    if(rport->Layer)
	UnlockLayer(rport->Layer);

    AROS_LIBFUNC_EXIT
} /* DoHookClipRects */
