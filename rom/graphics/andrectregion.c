/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AndRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <proto/exec.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(void, AndRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region, A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 84, Graphics)

/*  FUNCTION
	Remove everything inside 'region' that is outside 'rectangle'

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    NOTES
	This is the only *RectRegion function that cannot fail

    BUGS
	Although the header claims that this function is documented with
	"Lang: english", it should perhaps better be "Lang: broken english"

    SEE ALSO
	AndRegionRegion() OrRectRegion() XorRectRegion() ClearRectRegion()
	NewRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    _AndRectRegion(region, rectangle, GfxBase);

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




