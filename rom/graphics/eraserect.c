/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function EraseRect()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <aros/asmcall.h>
#include <utility/hooks.h>
#include <proto/oop.h>
#include <proto/layers.h>
#include "gfxfuncsupport.h"

#define LayersBase (struct LayersBase *)(GfxBase->gb_LayersBase)

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH5(void, EraseRect,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , xMin, D0),
	AROS_LHA(LONG             , yMin, D1),
	AROS_LHA(LONG             , xMax, D2),
	AROS_LHA(LONG             , yMax, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 135, Graphics)

/*  FUNCTION
	Fill a rectangular area with the current BackFill hook.
	If layered the BackFill hook is used.
	If non-layered the region is cleared.

    INPUTS
	rp        - destination RastPort
	xMin,yMin - upper left corner
	xMax,YMax - lower right corner

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Rectangle r;
    struct Hook     *h = LAYERS_BACKFILL;
   
    if (!LayersBase) {
        rp->DrawMode ^= INVERSVID;
        RectFill(rp, xMin, yMin, xMax, yMax);
        rp->DrawMode ^= INVERSVID;
        return;
    }

    FIX_GFXCOORD(xMin);
    FIX_GFXCOORD(yMin);
    FIX_GFXCOORD(xMax);
    FIX_GFXCOORD(yMax);
    
    r.MinX = xMin;
    r.MinY = yMin;
    r.MaxX = xMax;
    r.MaxY = yMax;

    if (rp->Layer)
    {
        LockLayerRom(rp->Layer);
        h = rp->Layer->BackFill;
    }

    DoHookClipRects(h, rp, &r);
    
    if (rp->Layer)
    {
        UnlockLayerRom(rp->Layer);
    }

    ReturnVoid("EraseRect");

    AROS_LIBFUNC_EXIT

} /* EraseRect */

/****************************************************************************************/

#undef LayersBase

/****************************************************************************************/

