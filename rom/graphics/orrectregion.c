/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function OrRectRegion()
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include "intregions.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region,    A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 85, Graphics)

/*  FUNCTION
	Add the given Rectangle to the given Region (if not
	already there)

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    RESULT
	TRUE if the operation was succesful, else FALSE
	(out of memory)

    NOTES
	All relevant data is copied, you may throw away the
	given rectangle after calling this function

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), XorRectRegion(), ClearRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RegionRectangle* nrect, *rr;
    WORD xoffset = 0;
    WORD yoffset = 0;

    /* create new RegionRectangle */
    if (!(nrect = NewRegionRectangle()))
	return FALSE;
    nrect->bounds = *rectangle;

    /* check if 'region' and 'rectangle' overlap */
    if (region->RegionRectangle && overlap(region->bounds, *rectangle)) {

	/* clear the rectangle from the region */
	if (!ClearRectRegion(region, rectangle)) {
	    DisposeRegionRectangle(nrect);
	    return FALSE; /* out of memory */
	}
    }

    if (region->RegionRectangle) {
	/* calculate xoffset, yoffset
	 */
	if (region->bounds.MinX > rectangle->MinX)
	    xoffset = region->bounds.MinX - rectangle->MinX;
	if (region->bounds.MinY > rectangle->MinY)
	    yoffset = region->bounds.MinY - rectangle->MinY;

	/* adjust RegionRectangle bounds */
	if (xoffset || yoffset) {
	    for (rr = region->RegionRectangle; rr; rr = rr->Next) {
		rr->bounds.MinX += xoffset;
		rr->bounds.MinY += yoffset;
		rr->bounds.MaxX += xoffset;
		rr->bounds.MaxY += yoffset;
	    }
	}

	/* adjust Region bounds
	 */
	if (xoffset) region->bounds.MinX = rectangle->MinX;
	if (yoffset) region->bounds.MinY = rectangle->MinY;
        if (region->bounds.MaxX < rectangle->MaxX)
	  region->bounds.MaxX = rectangle->MaxX;
	if (region->bounds.MaxY < rectangle->MaxY)
	  region->bounds.MaxY = rectangle->MaxY;
    } else {
	region->bounds.MinX = rectangle->MinX;
	region->bounds.MinY = rectangle->MinY;
	region->bounds.MaxX = rectangle->MaxX;
	region->bounds.MaxY = rectangle->MaxY;
    }

    /* adjust new Rectangle bounds
     */
    nrect->bounds.MinX -= region->bounds.MinX;
    nrect->bounds.MinY -= region->bounds.MinY;
    nrect->bounds.MaxX -= region->bounds.MinX;
    nrect->bounds.MaxY -= region->bounds.MinY;

    /* add new Rectangle to Region
     */
    rr = region->RegionRectangle;
    region->RegionRectangle = nrect;
    nrect->Prev = NULL;
    nrect->Next = rr;
    if (rr)
      rr->Prev = nrect;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* OrRectRegion */









