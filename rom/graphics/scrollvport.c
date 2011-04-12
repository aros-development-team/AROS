/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ScrollVPort()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, ScrollVPort,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 98, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
	AROS video drivers can perform a validation of offsets, and may refuse
	to scroll the screen too far (if they somehow can't provide the requested
	offset). In this case offset values in the ViewPort will be updated in
	order to reflect the real result of the operation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Bitmap object pointer is contained in struct ViewPortData,
     * connected to a ViewPortExtra.
     * This is true even for planar Amiga bitmaps.
     */
    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(vp);

    if (vpe)
    {
    	struct TagItem tags[] =
    	{
            { .ti_Tag = aHidd_BitMap_LeftEdge, .ti_Data = vp->DxOffset, },
            { .ti_Tag = aHidd_BitMap_TopEdge,  .ti_Data = vp->DyOffset, },
	    { .ti_Tag = TAG_DONE, }
        };
	IPTR offset;

	/* Actually move the bitmap */
	OOP_SetAttrs(VPE_DATA(vpe)->Bitmap, tags);

	/* The bitmap may fail to move. Fix up offsets now */
	OOP_GetAttr(VPE_DATA(vpe)->Bitmap, aHidd_BitMap_LeftEdge, &offset);
	vp->DxOffset = offset;
	OOP_GetAttr(VPE_DATA(vpe)->Bitmap, aHidd_BitMap_TopEdge, &offset);
	vp->DyOffset = offset;
    }

    AROS_LIBFUNC_EXIT
} /* ScrollVPort */
