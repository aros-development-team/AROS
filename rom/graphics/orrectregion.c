/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function OrRectRegion()
    Lang: english
*/

#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>

#include "graphics_intern.h"
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg,  A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 85, Graphics)

/*  FUNCTION
	Add the given Rectangle to the given Region (if not
	already there)

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    RESULT
	TRUE if the operation was successful, else FALSE
	(out of memory)

    NOTES
	All relevant data is copied, you may throw away the
	given rectangle after calling this function

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), XorRectRegion(), ClearRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (IS_RECT_EVIL(Rect)) return TRUE;

    if (Reg->RegionRectangle)
    {
	/* Region is not empty. Do the complete algorithm. */
    	struct Region Res;
    	struct RegionRectangle rr;

	InitRegion(&Res);

	rr.bounds = *Rect;
	rr.Next   = NULL;
	rr.Prev   = NULL;

    	if (_DoOperationBandBand(_OrBandBand,
			         MinX(Reg), 0, MinY(Reg), 0,
				 Reg->RegionRectangle, &rr, &Res.RegionRectangle, &Res.bounds, GfxBase))
    	{
	    ClearRegion(Reg);

            *Reg = Res;
            _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

            return TRUE;
    	}
    }
    else
    {
	/* Optimized version for empty destination Region. Just add a single rectangle. */
    	struct RegionRectangle *rr = _NewRegionRectangle(&Reg->RegionRectangle, GfxBase);

    	if (rr)
	{
	    Reg->bounds = *Rect;

	    rr->bounds.MinX = 0;
	    rr->bounds.MinY = 0;
	    rr->bounds.MaxX = Rect->MaxX - Rect->MinX;
	    rr->bounds.MaxY = Rect->MaxY - Rect->MinY;

	    return TRUE;
	}
    }
    
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* OrRectRegion */
