/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function AndRegionRegion()
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct Region *, OrRegionRegionND,

/*  SYNOPSIS */
        AROS_LHA(struct Region *, R1, A0),
        AROS_LHA(struct Region *, R2, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 189, Graphics)

/*  FUNCTION
        OR of one region with another region.

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

    return _RegionOpND(_OrBandBand, R1, R2, NewRegion(), GfxBase);

    AROS_LIBFUNC_EXIT
}
