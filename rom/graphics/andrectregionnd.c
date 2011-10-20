/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AndRectRegion()
    Lang: english
*/

#include <graphics/regions.h>
#include <proto/exec.h>
#include <clib/macros.h>

#include "graphics_intern.h"
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(struct Region *, AndRectRegionND,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg, A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 186, Graphics)

/*  FUNCTION
        Remove everything inside 'region' that is outside 'rectangle'

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    RESULT
        The resulting region or NULL in case there's no enough free memory

    NOTES

    BUGS

    SEE ALSO
	AndRegionRegion(), OrRectRegion(), XorRectRegion(), ClearRectRegion()
	NewRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region *Res = NewRegion();

    if (IS_RECT_EVIL(Rect))
    {
    	return Res;
    }

    if (!Reg->RegionRectangle)
    {
	/* The region is already empty, return empty copy */
    	return Res;
    }

    if (Res)
    {
	if (Rect->MinX <= MinX(Reg) &&
            Rect->MinY <= MinY(Reg) &&
            Rect->MaxX >= MaxX(Reg) &&
            Rect->MaxY >= MaxY(Reg))
    	{
    	    /* The rectangle completely covers the region. Make a plain copy. */
	    if (_CopyRegionRectangles(Reg, Res, GfxBase))
	    	return Res;
    	}
    	else
    	{
            struct RegionRectangle rr;

            rr.bounds = *Rect;
            rr.Next   = NULL;
            rr.Prev   = NULL;

            if (_DoOperationBandBand(_AndBandBand,
				     MinX(Reg), 0, MinY(Reg), 0,
			             Reg->RegionRectangle, &rr, &Res->RegionRectangle, &Res->bounds, GfxBase))
            {
            	_TranslateRegionRectangles(Res->RegionRectangle, -MinX(Res), -MinY(Res));

            	return Res;
            }
        }

        DisposeRegion(Res);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */

