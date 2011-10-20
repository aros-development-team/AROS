/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function OrRegionRegion()
    Lang: english
*/

#include <graphics/regions.h>

#include "graphics_intern.h"
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 102, Graphics)

/*  FUNCTION
	OR of one region with another region, leaving result in 
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
	AndRegionRegion(), XOrRegionRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region R3;

    if (!R1->RegionRectangle)
    {
    	/* First region is empty, second left intact */
        return TRUE;
    }

    if (!R2->RegionRectangle)
    {
	/* Second region was empty. Make a plain copy. */
    	return _CopyRegionRectangles(R1, R2, GfxBase);
    }

    /* Do the complete slow thing here */
    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _OrBandBand,
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

    return FALSE;

    AROS_LIBFUNC_EXIT
}
