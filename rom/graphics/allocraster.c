/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH2(PLANEPTR, AllocRaster,

/*  SYNOPSIS */
	AROS_LHA(UWORD          , width, D0),
	AROS_LHA(UWORD          , height, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 82, Graphics)

/*  FUNCTION
	Allocates memory for a single bitplane with the specified size in
	pixels.

    INPUTS
	width, height - The size of the resulting bitplane in pixels.

    RESULT
	A pointer to the single bitplane.

    NOTES
	You should not use this function to create BitMaps. Call
	AllocBitMap() instead.

    EXAMPLE

    BUGS

    SEE ALSO
	AllocBitMap(), FreeRaster(), FreeBitMap()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AllocMem (RASSIZE(width,height),MEMF_CHIP);
    AROS_LIBFUNC_EXIT
} /* AllocRaster */
