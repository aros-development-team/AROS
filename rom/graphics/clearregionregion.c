/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function ClearRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, ClearRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region1, A0),
	AROS_LHA(struct Region *, region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 187, Graphics)

/*  FUNCTION
	Clear the given Region region1 from the given Region region2
	leaving the result in region2.

    INPUTS
	region1 - pointer to a Region structure
	region2 - pointer to a Rectangle structure

    RESULT
	FALSE if not enough memory was available, else TRUE

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	ClearRectRegion() AndRectRegion() OrRectRegion() XorRectRegion()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
#if USE_BANDED_FUNCTIONS

    return _ClearRegionRegion(region1, region2, GfxBase);

#else

    struct Region *bak = CopyRegion(region2);

    ASSERT_VALID_PTR(region1);
    ASSERT_VALID_PTR(region2);

    if (region2->RegionRectangle &&
    	region1->RegionRectangle &&
	overlap(region2->bounds, region1->bounds))
    {
	struct RegionRectangle  *rr;
	struct RegionRectangle	*backup;
	struct Rectangle    	clearrect;

	if (!CopyRegionRectangleList(region2->RegionRectangle, &backup))
	    return FALSE;

	for (rr = region1->RegionRectangle; rr; rr = rr->Next)
	{
            clearrect.MinX = rr->bounds.MinX + region1->bounds.MinX;
	    clearrect.MinY = rr->bounds.MinY + region1->bounds.MinY;
	    clearrect.MaxX = rr->bounds.MaxX + region1->bounds.MinX;
	    clearrect.MaxY = rr->bounds.MaxY + region1->bounds.MinY;
	    
	    if (!ClearRectRegion(region2, &clearrect))
	    {
		DisposeRegionRectangleList(region2->RegionRectangle);
		region2->RegionRectangle = backup;
		return FALSE;
	    }
	}
	/* the backup is not needed anymore in this case */
	DisposeRegionRectangleList(backup);
    }
    return TRUE;
#endif
    AROS_LIBFUNC_EXIT

} /* ClearRegionRegion */
