/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH8(void, BltTemplate,

/*  SYNOPSIS */

	AROS_LHA(PLANEPTR		, source	, A0),
	AROS_LHA(LONG              	, xSrc		, D0),
	AROS_LHA(LONG              	, srcMod	, D1),
	AROS_LHA(struct RastPort * 	, destRP	, A1),
	AROS_LHA(LONG              	, xDest		, D2),
	AROS_LHA(LONG              	, yDest		, D3),
	AROS_LHA(LONG              	, xSize		, D4),
	AROS_LHA(LONG              	, ySize		, D5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 6, Graphics)
	
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    driver_BltTemplate(source, xSrc, srcMod, destRP, xDest, yDest, xSize, ySize, GfxBase);
    return;

    AROS_LIBFUNC_EXIT
} /* BltTemplate */
