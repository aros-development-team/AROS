/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetDrMd()
    Lang: english
*/

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2I(void, SetDrMd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , drawMode, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 59, Graphics)

/*  FUNCTION
	Set the drawing mode for lines, fills and text.

    INPUTS
	rp       - RastPort
	drawMode - see graphics/rastport.h for possible flags.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    rp->DrawMode  = drawMode;
    rp->linpatcnt = 15;

    AROS_LIBFUNC_EXIT
} /* SetDrMd */
