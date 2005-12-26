/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AreaDraw()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(ULONG, AreaDraw,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , x , D0),
	AROS_LHA(WORD             , y , D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 43, Graphics)

/*  FUNCTION
	Add a point to the vector buffer.

    INPUTS
	rp - pointer to a valid RastPort structure with a pointer to
	     the previously initilized AreaInfo structure.
	x  - x-coordinate of the point in the raster
	y  - y-coordinate of the point in the raster

    RESULT
	error -  0 for success
	        -1 if the vector collection matrix is full

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitArea() AreaMove() AreaEllipse() AreaCircle() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct AreaInfo * areainfo = rp->AreaInfo;
    
    /* Is there still enough storage area in the areainfo-buffer?
     * We only need on vector to store here.
     */

    if (areainfo->Count + 1 <= areainfo->MaxCount)
    {
    	FIX_GFXCOORD(x);
	FIX_GFXCOORD(y);
	
	/* increment counter */
	
	areainfo->Count++;
	areainfo->VctrPtr[0] = x;
	areainfo->VctrPtr[1] = y;
    	areainfo->FlagPtr[0] = AREAINFOFLAG_DRAW;
	
	areainfo->VctrPtr    = &areainfo->VctrPtr[2];
	areainfo->FlagPtr++;
	
	return 0;
    }

    return -1;

    AROS_LIBFUNC_EXIT
    
} /* AreaDraw */
