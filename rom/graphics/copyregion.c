/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function CopyRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(struct Region *, CopyRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 188, Graphics)

/*  FUNCTION
    	Make a copy of the given Region.

    INPUTS
	region - pointer to a Region structure

    RESULT
	the copy of the Region, or NULL if not enough memory.

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

    struct Region *nreg;
    
    if ((nreg = NewRegion()))
    {
	nreg->bounds = region->bounds;
	if (region->RegionRectangle)
	{
	    if (!(nreg->RegionRectangle = copyrrects(region->RegionRectangle)))
	    {
		DisposeRegion(nreg);
		nreg = NULL;
	    }
	}
    }
    
    return nreg;
    
    AROS_LIBFUNC_EXIT
    
} /* CopyRegion */
