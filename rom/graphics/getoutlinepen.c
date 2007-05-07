/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$	 $Log

    Desc: Graphics function GetOutlinePen()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(ULONG, GetOutlinePen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 146, Graphics)

/*  FUNCTION
	Get the outline pen value for a RastPort.

    INPUTS
	rp - RastPort.

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

    return (UBYTE)rp->AOlPen;

    AROS_LIBFUNC_EXIT
} /* GetOutlinePen */
