/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for allocating screen bitmaps
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

	AROS_LH2(BOOL , SetFrontBitMap,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, bitmap, A0),
	AROS_LHA(BOOL, copyback, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 184, Graphics)

/*  FUNCTION
	Sets the supplied screen as the frontmost, eg. shows it in the display.

    INPUTS
	bitmap - The bitmap to put in front. Must be a displayable bitmap.
	copyback - Whether to copy back from the framebuffer into
	           the previously front bitmap. !!!! Only set this to TRUE 
		   this if you are 100% SURE that
		   the previously shown bitmap has not been disposed

    RESULT
    	success - TRUE if successfull, FALSE otherwize.

    NOTES
	This function is private and AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    return driver_SetFrontBitMap(bitmap, copyback, GfxBase);

    AROS_LIBFUNC_EXIT
} /* AllocScreenBitMap */
