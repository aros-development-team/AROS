/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function ClearRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/exec.h>
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, ClearRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 88, Graphics)

/*  FUNCTION
	Removes all rectangles in the specified region.

    INPUTS
	region - pointer to the region structure

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	15-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    disposerrects(region->RegionRectangle);
    region->RegionRectangle = NULL;
    region->bounds.MinX = region->bounds.MaxX = 0;
    region->bounds.MinY = region->bounds.MaxY = 0;

    AROS_LIBFUNC_EXIT
} /* ClearRegion */






