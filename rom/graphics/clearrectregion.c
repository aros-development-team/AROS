/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function ClearRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, ClearRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region   , A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 87, Graphics)

/*  FUNCTION
	Clear the given Rectangle from the given Region

    INPUTS
	region - pointer to a Region structure
	rectangle - pointer to a Rectangle structure

    RESULT
	FALSE if not enough memory was available, else TRUE

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion() OrRectRegion() XorRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	20-01-96    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
#if USE_BANDED_FUNCTIONS

    return _ClearRectRegion(region, rectangle, GfxBase);

#else

    struct RegionRectangle* lastrects = NULL;

    /* no overlap, nothing  (not much :-) to do */
    if (region->RegionRectangle && overlap(region->bounds, *rectangle)) {

	struct RegionRectangle* newrects = NULL;
	struct RegionRectangle* cur;
	struct Rectangle inside;

	/* make coordinates relative to region->bounds
	 */
	inside.MinX = rectangle->MinX - region->bounds.MinX;
	inside.MinY = rectangle->MinY - region->bounds.MinY;
	inside.MaxX = rectangle->MaxX - region->bounds.MinX;
	inside.MaxY = rectangle->MaxY - region->bounds.MinY;

	/* search first remaining RegionRectangle
	 */
	for (cur = region->RegionRectangle; cur; cur = cur->Next) {
	    if (clearrectrect(&inside, &cur->bounds, &newrects)) {
		if (newrects) {
		    break;
		}
	    } else {  /* out of memory */
		return FALSE;
	    }
	}

	/* go through the remaining RegionRectangles
	 */
	if (newrects) {
	    struct RegionRectangle* rr;
	    WORD xoffset = newrects->bounds.MinX;
	    WORD yoffset = newrects->bounds.MinY;
	    WORD xmax = newrects->bounds.MaxX;
	    WORD ymax = newrects->bounds.MaxY;

	    /* get bounds
	     */
	    for (rr = newrects; rr; rr = rr->Next) {
		if (xoffset > rr->bounds.MinX) xoffset = rr->bounds.MinX;
		if (yoffset > rr->bounds.MinY) yoffset = rr->bounds.MinY;
		if (xmax < rr->bounds.MaxX) xmax = rr->bounds.MaxX;
		if (ymax < rr->bounds.MaxY) ymax = rr->bounds.MaxY;
	    }

	    lastrects = newrects;
	    cur = cur->Next;

	    for (; cur; cur = cur->Next) {

		if (clearrectrect(&inside, &cur->bounds, &newrects)) {

		    if (newrects) {
			/* search last RegionRectangle and adjust bounds
			 */
			for (rr = newrects; rr; rr = rr->Next) {
			    if (xoffset > rr->bounds.MinX) xoffset = rr->bounds.MinX;
			    if (yoffset > rr->bounds.MinY) yoffset = rr->bounds.MinY;
			    if (xmax < rr->bounds.MaxX) xmax = rr->bounds.MaxX;
			    if (ymax < rr->bounds.MaxY) ymax = rr->bounds.MaxY;
			    if (!rr->Next)
				break;
			}
			rr->Next = lastrects;
			lastrects->Prev = rr;
			lastrects = newrects;
		    }
		} else {  /* out of memory */
		    if (lastrects)
			DisposeRegionRectangleList(lastrects);
		    return FALSE;
		}
	    }  /* FOR() */

	    /* adjust Region bounds
	     */
	    region->bounds.MaxX = region->bounds.MinX + xmax;
	    region->bounds.MaxY = region->bounds.MinY + ymax;
	    region->bounds.MinX += xoffset;
	    region->bounds.MinY += yoffset;

	    /* adjust RegionRectangle bounds
	     */
	    if (xoffset || yoffset) {
		for (rr = lastrects; rr; rr = rr->Next) {
		    rr->bounds.MinX = rr->bounds.MinX - xoffset;
		    rr->bounds.MinY = rr->bounds.MinY - yoffset;
		    rr->bounds.MaxX = rr->bounds.MaxX - xoffset;
		    rr->bounds.MaxY = rr->bounds.MaxY - yoffset;
		}
	    }

	}  /* IF (newrects) */
    }  /* IF (overlap) */
    else /* no overlap at all */
      return TRUE;
     
    if (region->RegionRectangle)
	DisposeRegionRectangleList(region->RegionRectangle);
    region->RegionRectangle = lastrects;

    return TRUE;
#endif
    AROS_LIBFUNC_EXIT
} /* ClearRectRegion */
