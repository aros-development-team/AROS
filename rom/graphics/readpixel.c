/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ReadPixel()
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, ReadPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 53, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This function takes layers into account. Some pixel that is
        being read is not found on the display-bitmap but in some
        clipped rectangle (cliprect) in a layer structure.
        There is no support of anything else than bitplanes now.
        (No chunky pixels)
        This function resembles very much the function WritePixel()!!

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
  
  return driver_ReadPixel(rp, x, y, GfxBase);
 
  AROS_LIBFUNC_EXIT
} /* ReadPixel */
