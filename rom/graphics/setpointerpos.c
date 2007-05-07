/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for setting mouse pointer position
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <oop/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(VOID , SetPointerPos,

/*  SYNOPSIS */
	AROS_LHA(UWORD, x, D0),
	AROS_LHA(UWORD, y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 185, Graphics)

/*  FUNCTION
	Set the current mouse pointer's position.

    INPUTS

    RESULT

    NOTES
	This function is private and AROS specific.

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
    
    HIDD_Gfx_SetCursorPos(SDD(GfxBase)->gfxhidd, x, y);

    AROS_LIBFUNC_EXIT
} /* SetPointerPos */
