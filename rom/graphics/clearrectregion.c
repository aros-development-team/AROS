/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function ClearRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, ClearRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region   , A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 87, Graphics)

/*  FUNCTION
	Clear the given Rectangle from the given Region

    INPUTS
	region - pointer to a Region structure
	rectangle - pointer to a Rectangle structure

    RESULT
	FALSE if not enough memory was available, else TRUE

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion() OrRectRegion() XorRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	20-01-96    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return _ClearRectRegion(region, rectangle, GfxBase);

    AROS_LIBFUNC_EXIT
} /* ClearRectRegion */
