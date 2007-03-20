/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function XorRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, XorRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, R1, A0),
	AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 103, Graphics)

/*  FUNCTION
	Exclusive-OR of one region with another region,
	leaving result in second region.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the operation was successful,
        FALSE otherwise (out of memory)

    NOTES


    EXAMPLE

    BUGS

    SEE ALSO
	AndRegionRegion(), OrRegionRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla  automatically created from
			     graphics_lib.fd and clib/graphics_protos.h
	19-01-97    mreckt   intital version

        22-09-2001  falemagn changed implementation

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region R3;
    struct RegionRectangle *Diff1 = NULL, *Diff2 = NULL;

    LONG res;

    if
    (
        !R1->RegionRectangle             ||
        !R2->RegionRectangle             ||
        !overlap(R1->bounds, R2->bounds)
    )
    {
        return OrRegionRegion(R1, R2);
    }

    InitRegion(&R3);

    res = _DoOperationBandBand
    (
        _ClearBandBand,
        MinX(R1),
        MinX(R2),
	MinY(R1),
        MinY(R2),
        R1->RegionRectangle,
        R2->RegionRectangle,
        &Diff1,
        &R3.bounds,
        GfxBase
    ) &&
    _DoOperationBandBand
    (
        _ClearBandBand,
        MinX(R2),
        MinX(R1),
        MinY(R2),
	MinY(R1),
        R2->RegionRectangle,
        R1->RegionRectangle,
        &Diff2,
        &R3.bounds,
        GfxBase
    ) &&
    _DoOperationBandBand
    (
        _OrBandBand,
        0,
        0,
        0,
        0,
        Diff1,
        Diff2,
        &R3.RegionRectangle,
        &R3.bounds,
        GfxBase
    );

    _DisposeRegionRectangleList(Diff1, GfxBase);
    _DisposeRegionRectangleList(Diff2, GfxBase);

    if (res)
    {
        ClearRegion(R2);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));
    }

    return res;

    AROS_LIBFUNC_EXIT
}
