/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (AROS only) Graphics function SetRegion()
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, SetRegion,

/*  SYNOPSIS */
        AROS_LHA(struct Region *, src , A0),
        AROS_LHA(struct Region *, dest, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 181, Graphics)

/*  FUNCTION
        Sets the destination region to the source region.
        Allocates necessary RegionRectangles if necessary
        and deallocates any excessive RegionRectangles in
        the destination Region. The source Region remains
        untouched.
        If the system runs out of memory during allocation
        of RegionRectangles the destination Region will
        .
	
    INPUTS

    RESULT
        TRUE if everything went alright, FALSE otherwise
        (out of memory).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion(), DisposeRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RegionRectangle * rrs, *rrd, *rrd_prev, *addr;

    dest->bounds = src->bounds;

    for
    (
        rrs = src->RegionRectangle, rrd = dest->RegionRectangle, rrd_prev = NULL;
        rrs && rrd;
        rrd_prev = rrd, rrs = rrs->Next, rrd = rrd->Next
    )
    {
        rrd->bounds = rrs->bounds;
    }

    _DisposeRegionRectangleList(rrd, GfxBase);

    if (rrd_prev)
        addr = rrd_prev;
    else
    {
        dest->RegionRectangle = NULL;
        addr = dest->RegionRectangle;
    }

    if (!_LinkRegionRectangleList(rrs, &addr, GfxBase))
        return FALSE;

    if (!rrd_prev)
        dest->RegionRectangle = addr ? &Chunk(addr)->FirstChunk->Rects[0].RR : NULL;

    return TRUE;

    AROS_LIBFUNC_EXIT

} /* SetRegion */
