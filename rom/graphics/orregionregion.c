/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function OrRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region1, A0),
	AROS_LHA(struct Region *, region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 102, Graphics)

/*  FUNCTION
	OR of one region with another region, leaving result in 
	second region.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the operation was succesful, else FALSE
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

#if USE_BANDED_FUNCTIONS

    return _OrRegionRegion(region1, region2, GfxBase);

#else

    struct Rectangle CurRectangle;
    struct RegionRectangle * CurRR = region1 -> RegionRectangle;

    while (NULL != CurRR)
    {
      CurRectangle.MinX = region1->bounds.MinX + CurRR->bounds.MinX;
      CurRectangle.MaxX = region1->bounds.MinX + CurRR->bounds.MaxX;
      CurRectangle.MinY = region1->bounds.MinY + CurRR->bounds.MinY;
      CurRectangle.MaxY = region1->bounds.MinY + CurRR->bounds.MaxY;

      if (FALSE == OrRectRegion(region2, &CurRectangle))
        return FALSE;

      CurRR = CurRR -> Next;
    }

    return TRUE;
#endif
    AROS_LIBFUNC_EXIT
}
