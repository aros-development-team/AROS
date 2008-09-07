/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ScrollRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH4(BOOL, ScrollRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *,    region, A0),
	AROS_LHA(struct Rectangle *, rect,   A1),
	AROS_LHA(WORD,               dx,     D0),
	AROS_LHA(WORD,               dy,     D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 191, Graphics)

/*  FUNCTION
	Scroll the rectangles in the region by the amount of pixels specified, within the
        specified rectangle.

    INPUTS
	region - pointer to a region structure
	rect   - pointer to the rectangle within which the scrolling has to happen.
                 If NULL, the region's bounds are used instead.
	dx, dy - the amount of pixels by which to scroll the region. Negative values mean
                 respectively left and up, positive values mean right and down.
    RESULT
	TRUE if the operation succeeded, FALSE otherwise.

    NOTES
       This function doesn't exist in AmigaOS

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL res = FALSE;

    if (!rect)
    {
        TranslateRect(Bounds(region), dx, dy);

        res = TRUE;
    }
    else
    {
        struct Region *cutRegion;

        cutRegion = AndRectRegionND(region, rect);
        if (cutRegion)
        {
	    struct Region *newRegion;

            TranslateRect(Bounds(cutRegion), dx, dy);

            AndRectRegion(cutRegion, rect);

            newRegion = ClearRectRegionND(region, rect);
            if (newRegion)
            {
                if (OrRegionRegion(cutRegion, newRegion))
                {
		    _SwapRegions(region, newRegion);

                    res = TRUE;
                }

                DisposeRegion(newRegion);
            }

            DisposeRegion(cutRegion);
        }
    }

    return res;

    AROS_LIBFUNC_EXIT
}
