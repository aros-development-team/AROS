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

    return _ClearRegionRegion(region1, region2, GfxBase);

    AROS_LIBFUNC_EXIT

} /* ClearRegionRegion */
