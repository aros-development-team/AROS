/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(struct Region *, XorRectRegionND,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg,  A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 190, Graphics)

/*  FUNCTION
	Exclusive-OR the given rectangle to the given
	region

    INPUTS
	region - pointer to a region structure
	rectangle - pointer to a rectangle structure

    RESULT
	The resulting region or NULL in case there's no enough free memory

    NOTES
	All relevant data is copied, you may throw away the
	given rectangle after calling this function

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), OrRectRegion(), ClearRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	19-01-97    mreckt  intital version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region R;
    struct RegionRectangle rr;

    if (IS_RECT_EVIL(Rect))
    {
    	return CopyRegion(Reg);
    }
    
    InitRegion(&R);

    R.bounds = *Rect;
    R.RegionRectangle = &rr;

    rr.Next = NULL;
    MinX(&rr) = MinY(&rr) = 0;
    MaxX(&rr) = Rect->MaxX - Rect->MinX;
    MaxY(&rr) = Rect->MaxY - Rect->MinY;

    return XorRegionRegionND(&R, Reg);

    AROS_LIBFUNC_EXIT
} /* XorRectRegion */
