/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function OrRegionRegion()
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

    if(!R1->RegionRectangle) {
        /* First region is empty, second left intact */
        return TRUE;
    }

    if(!R2->RegionRectangle) {
        /* Second region was empty. Make a plain copy. */
        return _CopyRegionRectangles(R1, R2, GfxBase);
    }

    /* Do the complete slow thing here */
    return _RegionOpInPlace(_OrBandBand, R1, R2, R2, GfxBase);

    AROS_LIBFUNC_EXIT
}
