/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AreaMove()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(ULONG, AreaMove,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , x , D0),
	AROS_LHA(WORD             , y , D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 42, Graphics)

/*  FUNCTION
	Define a new starting point in the vector list for the following
	polygon defined by calls to AreaDraw(). The last polygon is closed
	if it wasn't closed by the user and the new starting points are
	added to the vector collection matrix.

    INPUTS
	rp - pointer to a valid RastPort structure with a pointer to
	     the previously initilized AreaInfo structure.
	x  - x-coordinate of the starting-point in the raster
	y  - y-coordinate of the starting-point in the raster

    RESULT
	error -  0 for success
	        -1 if the vector collection matrix is full

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitArea() AreaDraw() AreaEllipse() AreaCircle() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct AreaInfo * areainfo = rp->AreaInfo;

    /* Is there still enough storage area in the areainfo-buffer?
     * We just need room for one entry.
     */

    if (areainfo->Count + 1 <= areainfo->MaxCount)
    {
    	FIX_GFXCOORD(x);
	FIX_GFXCOORD(y);
	
	/* is this the very first entry in the vector collection matrix */
	if (0 == areainfo->Count)
	{
	    areainfo->FirstX = x;
	    areainfo->FirstY = y;

	    /* Insert the new point into the matrix */
	    areainfo->VctrPtr[0] = x;
	    areainfo->VctrPtr[1] = y;
	    areainfo->VctrPtr    = &areainfo->VctrPtr[2];

	    areainfo->FlagPtr[0] = AREAINFOFLAG_MOVE;
	    areainfo->FlagPtr++;

	    areainfo->Count++;
	}
	else
	{
	    /* if the previous command was also an AreaMove() then we will replace
	     * that one ...
	     */
	    if ( AREAINFOFLAG_MOVE == areainfo->FlagPtr[-1])
	    {
        	areainfo->FirstX = x;
        	areainfo->FirstY = y;

        	/* replace the previous point */
        	areainfo->VctrPtr[-2] = x;
        	areainfo->VctrPtr[-1] = y;
	    }
	    else /* it's not the first command and the previous command wasn't AreaMove() */
	    {
        	/* ... otherwise close the polygon if necessary */
		
		areaclosepolygon(areainfo);
		
 		/* Need to check again, if there is enough room, because
		   areaclosepolygon might have eaten one vector!! */
		   
		if (areainfo->Count + 1 > areainfo->MaxCount)
    	    	    return -1;
		    
        	areainfo->FirstX = x;
        	areainfo->FirstY = y;

        	/* Insert the new point into the matrix */
        	areainfo->VctrPtr[0] = x;
        	areainfo->VctrPtr[1] = y;
        	areainfo->VctrPtr    = &areainfo->VctrPtr[2];

        	areainfo->FlagPtr[0] = AREAINFOFLAG_MOVE;
        	areainfo->FlagPtr++;

        	areainfo->Count++;
	    }
	    
	} /* if (0 == areainfo->Count) */

    } /* if (areainfo->Count + 1 <= areainfo->MaxCount) */
    else
	return -1;

    return 0;

    AROS_LIBFUNC_EXIT
    
} /* AreaMove */
