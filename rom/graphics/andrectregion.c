/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AndRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <proto/exec.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(void, AndRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region, A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 84, Graphics)

/*  FUNCTION
	Remove everything inside 'region' that is outside 'rectangle'

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    NOTES
	This is the only *RectRegion function that cannot fail

    BUGS
	Although the header claims that this function is documented with
	"Lang: english", it should perhaps better be "Lang: broken english"

    SEE ALSO
	AndRegionRegion() OrRectRegion() XorRectRegion() ClearRectRegion()
	NewRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#if USE_BANDED_FUNCTIONS

    _AndRectRegion(region, rectangle, GfxBase);

#else

    if (region->RegionRectangle) { /* no rectangles, nothing to do */
	struct RegionRectangle* rr, *rr2;
	struct Rectangle intersection;
	struct Rectangle inside;

	/* inside are the coordinates of the given rectangle relative */
	/* to the regions' bounds */
	/* the coordinates of all Rectangles in a Region are relative */
	/* to the Regions' bounds; this makes everything much more    */
	/* complicated :-( */
	inside.MinX = rectangle->MinX - region->bounds.MinX;
	inside.MinY = rectangle->MinY - region->bounds.MinY;
	inside.MaxX = rectangle->MaxX - region->bounds.MinX;
	inside.MaxY = rectangle->MaxY - region->bounds.MinY;

	/* search for the first RegionRectangle that intersects */
	/* with 'inside'                                        */
	for (rr = region->RegionRectangle; rr; rr = rr2) {
	    if (_AndRectRect(&rr->bounds, &inside, &intersection))
		break;
	    else {  /* no intersection -> dispose current RegionRectangle */
		rr2 = rr->Next;
		DisposeRegionRectangle(rr);
	    }
	}

	region->RegionRectangle = rr;

	if (rr) {
	    /* distance between the old bounds of 'region' and the smallest
	     * upper left corner of the remaining RegionRectangles
	     */
	    WORD xoffset = intersection.MinX;
	    WORD yoffset = intersection.MinY;
	    /* lower right corner of the new 'region' bounds, relative
	     * to its old bounds (I said it's getting complicated!)
	     */
	    WORD xmax = intersection.MaxX;
	    WORD ymax = intersection.MaxY;

	    rr->bounds     = intersection;
	    rr->Prev       = NULL;

	    /* Now go through all remaining RegionRectangles and find
	     * more intersections
	     */
	    for (rr = rr->Next; rr; rr = rr2) {
		if (_AndRectRect(&rr->bounds, &inside, &intersection)) {
		    /* cut to intersection */
		    rr->bounds = intersection;
		    /* adjust new 'region' bounds */
		    xoffset = MIN(xoffset, intersection.MinX);
		    yoffset = MIN(yoffset, intersection.MinY);
		    xmax = MAX(xmax, intersection.MaxX);
		    ymax = MAX(ymax, intersection.MaxY);
		    rr2 = rr->Next;
		} else {
		    /* no intersection -> dispose current RegionRectangle */
		    rr2 = rr->Next;			/* adjust  */
		    rr->Prev->Next = rr2;		/* linked  */
		    if (rr2) rr2->Prev = rr->Prev;	/* list    */
		    DisposeRegionRectangle(rr);
		}
	    }

	    /* set 'region' bounds
	     */
	    region->bounds.MaxX = region->bounds.MinX + xmax;
	    region->bounds.MaxY = region->bounds.MinY + ymax;
	    region->bounds.MinX = region->bounds.MinX + xoffset;
	    region->bounds.MinY = region->bounds.MinY + yoffset;

	    if (xoffset || yoffset) {
		/* adjust RegionRectangle bounds
		*/
		for (rr = region->RegionRectangle; rr; rr = rr->Next) {
		    rr->bounds.MinX = rr->bounds.MinX - xoffset;
		    rr->bounds.MinY = rr->bounds.MinY - yoffset;
		    rr->bounds.MaxX = rr->bounds.MaxX - xoffset;
		    rr->bounds.MaxY = rr->bounds.MaxY - yoffset;
		}
	    }
	} else { /* no intersection */
	    region->bounds.MinX = region->bounds.MaxX = 0;
	    region->bounds.MinY = region->bounds.MaxY = 0;
	}
    }

#endif
    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




