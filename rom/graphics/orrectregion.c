/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function OrRectRegion()
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include "intregions.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, region,    A0),
	AROS_LHA(struct Rectangle *, rectangle, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 85, Graphics)

/*  FUNCTION
	Add the given Rectangle to the given Region (if not
	already there)

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    RESULT
	TRUE if the operation was succesful, else FALSE
	(out of memory)

    NOTES
	All relevant data is copied, you may throw away the
	given rectangle after calling this function

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), XorRectRegion(), ClearRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return _OrRectRegion(region, rectangle, GfxBase);

    AROS_LIBFUNC_EXIT
} /* OrRectRegion */
