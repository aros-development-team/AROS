/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function DisposeRegionRectangleList()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, DisposeRegionRectangleList,

/*  SYNOPSIS */
	AROS_LHA(struct RegionRectangle *, regionrectangle, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 190, Graphics)

/*  FUNCTION
    	Free the linked list of RegionRectangles
	
    INPUTS
	regionrectangle - pointer to first regionrectangle. May be NULL.

    RESULT
	none

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion() DisposeRegion() DisposeRegionRectangle() ClearRegion()

    INTERNALS

    HISTORY
	13-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RegionRectangle *next;
    
    ASSERT_VALID_PTR_OR_NULL(regionrectangle);
    
    while(regionrectangle)
    {
    	next = regionrectangle->Next;
	DisposeRegionRectangle(regionrectangle);	
	regionrectangle = next;
    }
    
    AROS_LIBFUNC_EXIT
    
} /* DisposeRegionRectangleList */
