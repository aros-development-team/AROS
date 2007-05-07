/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Return the memory occupied by a single bitplane to the system.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH3(void, FreeRaster,

/*  SYNOPSIS */
	AROS_LHA(PLANEPTR, p,      A0),
	AROS_LHA(ULONG,    width,  D0),
	AROS_LHA(ULONG,    height, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 83, Graphics)

/*  FUNCTION
	Free the single bitplane allocated by AllocRaster().

    INPUTS
	p - The result of AllocRaster(). Must be non-NULL.
	width, height - The size of the bitplane as passed to AllocRaster().

    RESULT
	The memory occupied by the bitplane will be returned to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocRaster(), AllocBitMap(), FreeBitMap()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeMem (p, RASSIZE(width, height));

    AROS_LIBFUNC_EXIT
} /* FreeRaster */
