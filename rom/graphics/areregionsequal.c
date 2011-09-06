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

	AROS_LH2(BOOL, AreRegionsEqual,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 183, Graphics)

/*  FUNCTION
	Compares two regions.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the regions are equal, FALSE otherwise.

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

    struct RegionRectangle *rr1, *rr2;

    if (!_AreRectsEqual(Bounds(R1), Bounds(R2)))
	return FALSE;

    for
    (
        rr1 = R1->RegionRectangle, rr2 = R2->RegionRectangle;
	rr1 && rr2 && _AreRectsEqual(Bounds(rr1), Bounds(rr2));
        rr1 = rr1->Next, rr2 = rr2->Next
    );

    return rr1 == rr2;

    AROS_LIBFUNC_EXIT
}
