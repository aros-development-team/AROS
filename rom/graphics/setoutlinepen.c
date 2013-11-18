/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetOutlinePen()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(ULONG, SetOutlinePen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(ULONG,             pen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 163, Graphics)

/*  FUNCTION
	Set the outline pen and turn on area outline mode.

    INPUTS
	rp  - RastPort
	pen - pen

    RESULT
	Previous outline pen.

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

    ULONG oldPen;

    oldPen = rp->AOlPen;

    rp->AOlPen = pen;
    rp->Flags |= AREAOUTLINE;

    return oldPen;
    
    AROS_LIBFUNC_EXIT
    
} /* SetOutlinePen */
