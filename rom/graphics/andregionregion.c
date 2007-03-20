/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	AROS_LH2(BOOL, AndRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 104, Graphics)

/*  FUNCTION
	AND of one region with another region, leaving result in 
	second region.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the operation was successful, else FALSE
	(out of memory)

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

    if (!R1->RegionRectangle || !R2->RegionRectangle || !overlap(R1->bounds, R2->bounds))
    {
	ClearRegion(R2);
        return TRUE;
    }
    else
    {
        struct Region R3;

        InitRegion(&R3);

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
                &R3.RegionRectangle,
                &R3.bounds,
                GfxBase
            )
        )
        {
	    ClearRegion(R2);

            *R2 = R3;

            _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

            return TRUE;
        }
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
}
