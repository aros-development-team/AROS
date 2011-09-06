/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AndRegionRegion()
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(struct Region *, AndRegionRegionND,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 187, Graphics)

/*  FUNCTION
	AND of one region with another region

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
        The resulting region or NULL in case there's no enough free memory

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	XorRegionRegion(), OrRegionRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region *R3 = NewRegion();

    if
    (
	R3                              &&
        R1->RegionRectangle             &&
        R2->RegionRectangle             &&
        overlap(R1->bounds, R2->bounds)
    )
    {
        if
        (
            _DoOperationBandBand
            (
                _AndBandBand,
                MinX(R1),
                MinX(R2),
	        MinY(R1),
                MinY(R2),
                R1->RegionRectangle,
                R2->RegionRectangle,
                &R3->RegionRectangle,
                &R3->bounds,
                GfxBase
            )
        )
        {
            _TranslateRegionRectangles(R3->RegionRectangle, -MinX(R3), -MinY(R3));
        }
	else
        {
            DisposeRegion(R3);
            R3 = NULL;
        }
    }

    return R3;

    AROS_LIBFUNC_EXIT
}
