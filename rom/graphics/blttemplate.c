/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	void BltTemplate (

/*  SYNOPSIS */

	PLANEPTR	  source,
	LONG              xSrc,
	LONG              srcMod,
	struct RastPort * destRP,
	LONG              xDest,
	LONG              yDest,
	LONG              xSize,
	LONG              ySize)

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
