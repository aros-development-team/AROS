/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS Graphics function FreeRastPort
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(void, FreeRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 180, Graphics)

/*  FUNCTION
	This frees a RastPort obtained with CloneRastPort() or
	CreateRastPort().

    INPUTS
	rp - The result of CloneRastPort() or CreateRastPort().

    RESULT
	None.

    NOTES
	This function is AROS specific. For compatibility, there is a function
	with the same name in aros.lib on Amiga.

    EXAMPLE

    BUGS

    SEE ALSO
	CloneRastPort(), CreateRastPort()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DeinitRastPort (rp);

    FreeMem (rp, sizeof (struct RastPort));

    AROS_LIBFUNC_EXIT
} /* FreeRastPort */
