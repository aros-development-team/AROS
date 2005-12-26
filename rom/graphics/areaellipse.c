/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AreaEllipse()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH5(ULONG, AreaEllipse,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , cx, D0),
	AROS_LHA(WORD             , cy, D1),
	AROS_LHA(WORD             , a , D2),
	AROS_LHA(WORD             , b , D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 31, Graphics)

/*  FUNCTION
	Add an ellipse to the vector buffer. An ellipse takes up two
	entries in the buffer.

    INPUTS
	rp - pointer to a valid RastPort structure with a pointer to
	     the previously initilized AreaInfo structure.
	cx - x coordinate of the centerpoint relative to rastport
	cy - y coordinate of the centerpoint relative to rastport
	a  - horizontal radius of the ellipse (> 0)
	b  - vertical radius of the ellipse (> 0)

    RESULT
	error -  0 for success
	        -1 if the vector collection matrix is full

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitArea() AreaMove() AreaDraw() AreaCircle() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct AreaInfo * areainfo = rp->AreaInfo;

    /*  Is there still enough storage area in the areainfo-buffer?
     *  We need at least a storage area for two vectors
     */
    if (areainfo->Count + 2 <= areainfo->MaxCount)
    {
    	FIX_GFXCOORD(cx);
	FIX_GFXCOORD(cy);
	FIX_GFXCOORD(a);
	FIX_GFXCOORD(b);
	
	/* is this the very first entry in the vector collection matrix */
	if (0 == areainfo->Count)
	{
	    areainfo->VctrPtr[0] = cx;
	    areainfo->VctrPtr[1] = cy;
	    areainfo->FlagPtr[0] = AREAINFOFLAG_ELLIPSE;

	    areainfo->VctrPtr[2] = a;
	    areainfo->VctrPtr[3] = b;
	    areainfo->FlagPtr[1] = AREAINFOFLAG_ELLIPSE;

	    areainfo->VctrPtr    = &areainfo->VctrPtr[4];
	    areainfo->FlagPtr    = &areainfo->FlagPtr[2];

	    areainfo->Count += 2;
	}
	else
	{
	    areaclosepolygon(areainfo);

 	    /* Need to check again, if there is enough room, because
	       areaclosepolygon might have eaten one vector!! */

	    if (areainfo->Count + 2 > areainfo->MaxCount)
    	    	return -1;
	    
	    /*  If the previous command in the vector collection matrix was a move then
	     *  erase that one
	     */

	    if (AREAINFOFLAG_MOVE == areainfo->FlagPtr[-1])
	    {
        	areainfo->VctrPtr = &areainfo->VctrPtr[-2];
        	areainfo->FlagPtr--;
        	areainfo->Count--;
	    }

	    /* still enough storage area?? */
	    if (areainfo->Count + 2 <= areainfo->MaxCount)
	    {
        	areainfo->VctrPtr[0] = cx;
        	areainfo->VctrPtr[1] = cy;
        	areainfo->FlagPtr[0] = AREAINFOFLAG_ELLIPSE;

        	areainfo->VctrPtr[2] = a;
        	areainfo->VctrPtr[3] = b;
        	areainfo->FlagPtr[1] = AREAINFOFLAG_ELLIPSE;

        	areainfo->VctrPtr    = &areainfo->VctrPtr[4];
        	areainfo->FlagPtr    = &areainfo->FlagPtr[2];

        	areainfo->Count += 2;

        	return 0;
	    }
	    else
        	return -1;
	    
	} /* else branch of if (0 == areainfo->Count) */

	/* will never get to this point! */

    } /* if (areainfo->Count + 2 < areainfo->MaxCount) */

    return -1;  

    AROS_LIBFUNC_EXIT
  
} /* AreaEllipse */
