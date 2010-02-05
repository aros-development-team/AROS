/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

    /* We use obtain/release pair because this makes ScrollVPort()
       operating also on planar bitmaps, which can aid in porting AROS
       to m68k Amiga */
    OOP_Object *bitmap = OBTAIN_HIDD_BM(vp->RasInfo->BitMap);
    IPTR tags[] = {
        aHidd_BitMap_LeftEdge, vp->DxOffset,
	aHidd_BitMap_TopEdge,  vp->DyOffset,
	TAG_DONE
    };
    IPTR offset;

    /* Actually move the bitmap */
    OOP_SetAttrs(bitmap, tags);

    /* The bitmap may fail to move. Fix up offsets now */
    OOP_GetAttr(bitmap, aHidd_BitMap_LeftEdge, &offset);
    vp->DxOffset = offset;
    OOP_GetAttr(bitmap, aHidd_BitMap_TopEdge, &offset);
    vp->DyOffset = offset;

    RELEASE_HIDD_BM(bitmap, vp->RasInfo->BitMap);

    AROS_LIBFUNC_EXIT
} /* ScrollVPort */
