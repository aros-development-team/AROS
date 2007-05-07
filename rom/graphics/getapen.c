/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function GetAPen()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(ULONG, GetAPen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 143, Graphics)

/*  FUNCTION
	Return the current value of the A pen for the rastport.

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

    return (UBYTE)rp->FgPen;

    AROS_LIBFUNC_EXIT
} /* GetAPen */
