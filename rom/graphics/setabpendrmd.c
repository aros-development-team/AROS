/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function SetABPenDrMd()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH4(void, SetABPenDrMd,

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    /* Allow the driver to do its magic */
    driver_SetABPenDrMd (rp, apen, bpen, drawMode, GfxBase);

    /* Do it after the driver to allow it to inspect the previous value */
    rp->FgPen = apen;
    rp->BgPen = bpen;
    rp->DrawMode = drawMode;

    AROS_LIBFUNC_EXIT
} /* SetABPenDrMd */
