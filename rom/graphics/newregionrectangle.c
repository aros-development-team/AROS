/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function NewRegionRectangle()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH0(struct RegionRectangle *, NewRegionRectangle,

/*  SYNOPSIS */
	/* void */
	
/*  LOCATION */
	struct GfxBase *, GfxBase, 192, Graphics)

/*  FUNCTION
    	Allocate memory for a RegionRectangle
	
    INPUTS

    RESULT
	regionrectangle - pointer to a newly created RegionRectangle
	    	    	  structure that should be freed by a call to
			  DisposeRegionRectangle().

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion() DisposeRegion() DisposeRegionRectangle()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RegionRectangle *rr;	
    
    rr = AllocMem(sizeof(struct RegionRectangle), MEMF_CLEAR);
    
    return rr;
    
    AROS_LIBFUNC_EXIT
    
} /* NewRegionRectangle */
