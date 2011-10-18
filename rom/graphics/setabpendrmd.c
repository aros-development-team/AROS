/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function SetABPenDrMd()
    Lang: english
*/

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH4I(void, SetABPenDrMd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , apen, D0),
	AROS_LHA(ULONG            , bpen, D1),
	AROS_LHA(ULONG            , drawMode, D2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 149, Graphics)

/*  FUNCTION
	Changes the foreground and background pen and the drawmode in one
	step.

    INPUTS
	rp - Modify this RastPort
	apen - The new foreground pen
	bpen - The new background pen
	drawmode - The new drawmode

    RESULT
	None.

    NOTES
	This function is faster than the sequence SetAPen(), SetBPen(),
	SetDrMd().

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

    rp->FgPen     = apen;
    rp->BgPen     = bpen;
    rp->DrawMode  = drawMode;
    rp->linpatcnt = 15;
    rp->Flags    &= ~RPF_NO_PENS;

    AROS_LIBFUNC_EXIT
} /* SetABPenDrMd */
