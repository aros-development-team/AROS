/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetAPen()
    Lang: english
*/

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(void, SetAPen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , pen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 57, Graphics)

/*  FUNCTION
	Set primary pen for rastport

    INPUTS
	rp  - RastPort
	pen - pen number (0...255)

    RESULT

    NOTES
    	This functions turns on PenMode for the RastPort.

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

    rp->FgPen     = pen;
    rp->linpatcnt = 15;
    rp->Flags    &= ~RPF_NO_PENS;

    AROS_LIBFUNC_EXIT
} /* SetAPen */
