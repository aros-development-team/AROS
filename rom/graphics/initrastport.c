/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function InitRastPort()
    Lang: english
*/

#include <graphics/rastport.h>
#include <proto/exec.h>

#include <string.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 33, Graphics)

/*  FUNCTION
	Initializes a RastPort structure.

    INPUTS
	rp - The RastPort to initialize.

    RESULT
	all entries in RastPort get zeroed out, with the following exceptions:

	    Mask, FgPen, AOLPen, and LinePtrn are set to -1.
	    The DrawMode is set to JAM2
	    The font is set to the standard system font

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

    /* Zero out everything, then set some valid defaults */
    SetMem(rp, 0, sizeof(struct RastPort));

    rp->Mask     = 0xFF;
    rp->FgPen    = -1;
    rp->AOlPen   = -1;
    rp->DrawMode = JAM2;
    rp->LinePtrn = 0xFFFF;

    SetFont (rp, GfxBase->DefaultFont);

    AROS_LIBFUNC_EXIT
    
} /* InitRastPort */
