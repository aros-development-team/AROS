/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function DisposeRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/exec.h>
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, DisposeRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 89, Graphics)

/*  FUNCTION
	Frees all memory allocated by this region, including its
	RegionRectangles.

    INPUTS
	region - pointer to a Region structure

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
    FreeMem(region, sizeof(struct Region));

    AROS_LIBFUNC_EXIT
} /* DisposeRegion */
