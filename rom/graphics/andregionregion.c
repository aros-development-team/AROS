/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AndRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, AndRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region1, A0),
	AROS_LHA(struct Region *, region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 104, Graphics)

/*  FUNCTION
	AND of one region with another region, leaving result in 
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
	XorRegionRegion(), OrRegionRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return _AndRegionRegion(region1, region2, GfxBase);

    AROS_LIBFUNC_EXIT
}
