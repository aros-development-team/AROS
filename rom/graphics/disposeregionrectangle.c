/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function DisposeRegionRectangle()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, DisposeRegionRectangle,

/*  SYNOPSIS */
	AROS_LHA(struct RegionRectangle *, regionrectangle, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 191, Graphics)

/*  FUNCTION
    	Free a single RegionRectangle
	
    INPUTS
	regionrectangle - pointer to regionrectangle.

    RESULT
	none

    NOTES
	This function does not exist in AmigaOS.
	You will usually not need this functions.
	ClearRegion() is probably what you are looking
	for!

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion() DisposeRegion() DisposeRegionRectangleList() ClearRegion()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RegionRectangle *next;
    
    ASSERT_VALID_PTR(regionrectangle);
    
    FreeMem(regionrectangle, sizeof(struct RegionRectangle));	
    
    AROS_LIBFUNC_EXIT
    
} /* DisposeRegionRectangle */
