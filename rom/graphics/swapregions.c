/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ScrollRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2I(VOID, SwapRegions,

/*  SYNOPSIS */
	AROS_LHA(struct Region *,    region1, A0),
	AROS_LHA(struct Region *,    region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 192, Graphics)

/*  FUNCTION
	Swap the contents of the two regions.

    INPUTS
	region1, region2 - pointers to the two regions to swap the contents of

    RESULT

    NOTES
       This function doesn't exist in AmigaOS

    EXAMPLE

    BUGS

    SEE ALSO
	CreateRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    _SwapRegions(region1, region2);

    AROS_LIBFUNC_EXIT
}
