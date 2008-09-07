/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (AROS only) Graphics function NewRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH4(struct Region *, NewRectRegion,

/*  SYNOPSIS */
    	AROS_LHA(WORD, MinX, D0),
	AROS_LHA(WORD, MinY, D1),
	AROS_LHA(WORD, MaxX, D2),
	AROS_LHA(WORD, MaxY, D3),
		
/*  LOCATION */
	struct GfxBase *, GfxBase, 194, Graphics)

/*  FUNCTION
    	Creates a new rectangular Region
		
    INPUTS
    	MinX, MinY, MaxX, MaxY - The extent of the Rect
		
    RESULT
    	Pointer to the newly created Region. NULL on failure.

    NOTES
	This function does not exist in AmigaOS.
    	It does basically the same as:

	    struct Rectangle rect;
	    struct Region *region;

	    rect.MinX = MinX;
	    rect.MinY = MinY;
	    rect.MaxX = MaxX;
	    rect.MaxY = MaxY;

	    region = NewRegion();
	    OrRectRegion(region, &rect);

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion(), OrRectRegion()

    INTERNALS

    HISTORY
	15-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region   	    *region;
    struct RegionRectangle  *rr;

    if ((region = NewRegion()))
    {
        struct RegionRectangle *last = NULL;

        if ((rr = _NewRegionRectangle(&last, GfxBase)))
	{
	    region->bounds.MinX = MinX;
	    region->bounds.MinY = MinY;
	    region->bounds.MaxX = MaxX;
	    region->bounds.MaxY = MaxY;

	    rr->bounds.MinX = 0;
	    rr->bounds.MinY = 0;
	    rr->bounds.MaxX = MaxX - MinX;
	    rr->bounds.MaxY = MaxY - MinY;

	    region->RegionRectangle = rr;
	}
	else
	{
	    DisposeRegion(region);
	    region = NULL;
	}
    }

    return region;

    AROS_LIBFUNC_EXIT

} /* NewRectRegion */
