/*
    (C) 2000 AROS - The Amiga Research OS
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
	struct GfxBase *, GfxBase, 195, Graphics)

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
	NewRegion() DisposeRegion() DisposeRegionRectangle()
	CopyRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct RegionRectangle * rrs, *rrd, *rrd_prev, **addr;

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

    DisposeRegionRectangleList(rrd);

    if (rrd_prev)
        addr = &rrd_prev->Next;
    else
        addr = &dest->RegionRectangle;

#if REGIONS_HAVE_RRPOOL
    if (!CopyRegionRectangleList(rrs, addr, &dest->RectPoolList))
#else
    if (!CopyRegionRectangleList(rrs, addr))
#endif
        return FALSE;

    if (*addr)
        (*addr)->Prev = rrd_prev;

    return TRUE;

    AROS_LIBFUNC_EXIT

} /* SetRegion */
