/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function InitRegion()
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, InitRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 189, Graphics)

/*  FUNCTION
    	Initialize a "struct Region" varible, ie. a Region you
	did not allocate with NewRegion().

    INPUTS
	region - pointer to a Region structure

    RESULT
	none

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion() DisposeRegion()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(region);

    /*
    ** Important: ClearRegion calls InitRegion(). If you change something here
    ** which should not be done in case of ClearRegion() then don't forget to
    ** fix ClearRegion!
    */

    region->bounds.MinX = 0;
    region->bounds.MinY = 0;
    region->bounds.MaxX = 0;
    region->bounds.MaxY = 0;
    region->RegionRectangle = NULL;

#if REGIONS_HAVE_RRPOOL
    region->RectPoolList = NULL;
#endif

    AROS_LIBFUNC_EXIT
    
} /* InitRegion */
