/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (AROS only) Graphics function ClearRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/graphics.h>
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, ClearRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 187, Graphics)

/*  FUNCTION
	Clear the given Region region1 from the given Region region2
	leaving the result in region2.

    INPUTS
	region1 - pointer to a Region structure
	region2 - pointer to a Rectangle structure

    RESULT
	FALSE if not enough memory was available, else TRUE

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	ClearRectRegion(), AndRectRegion(), OrRectRegion(), XorRectRegion()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region R3;

    /* If the regions don't overlap just return */
    if (!overlap(R1->bounds, R2->bounds))
        return TRUE;

    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
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

} /* ClearRegionRegion */
