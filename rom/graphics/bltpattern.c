/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(void, BltPattern,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(PLANEPTR         , mask, A0),
	AROS_LHA(LONG             , xMin, D0),
	AROS_LHA(LONG             , yMin, D1),
	AROS_LHA(LONG             , xMax, D2),
	AROS_LHA(LONG             , yMax, D3),
	AROS_LHA(ULONG            , byteCnt, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 52, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    if (rp->AreaPtrn)
    {
	driver_BltPattern(rp
    	    , mask
	    , xMin, yMin
	    , xMax, yMax
	    , byteCnt
	    , GfxBase
	);
    }
    else
    {
    	if (mask)
	{
	    ULONG old_drawmode = GetDrMd(rp);
	    
	    if ((old_drawmode & ~INVERSVID) == JAM2)
	    	SetDrMd(rp, JAM1 | (old_drawmode & INVERSVID));
		
	    BltTemplate(mask, 0, byteCnt, rp, xMin, yMin, xMax - xMin + 1, yMax - yMin + 1);
	    
	    SetDrMd(rp, old_drawmode);
	}
	else
	{
	    RectFill(rp, xMin, yMin, xMax, yMax);
	}
    }
    
    AROS_LIBFUNC_EXIT
} /* BltPattern */
