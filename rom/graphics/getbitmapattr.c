/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get an attribute from a bitmap.
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH2(IPTR, GetBitMapAttr,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, bitmap, A0),
	AROS_LHA(ULONG          , attribute, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 160, Graphics)

/*  FUNCTION
	Returns a specific information about a bitmap. Do not access the
	bitmap directly!

    INPUTS
	bitmap - The BitMap structure to get information about.
	attribute - Number of the attribute to get. See <graphics/gfx.h> for
	            possible values.

    RESULT
	The information you requested. If you asked for an unknown attribute,
	0 or NULL is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocBitMap()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    switch(attribute)
    {
    case BMA_HEIGHT:
        return (IPTR)bitmap->Rows;
    case BMA_WIDTH:
      /* must return width in pixel! */
        return ((IPTR)bitmap->BytesPerRow * 8);
    case BMA_DEPTH:
        return (IPTR)bitmap->Depth;
    case BMA_FLAGS:
        return (IPTR)(bitmap->Flags & (BMF_DISPLAYABLE |
				       BMF_INTERLEAVED |
				       BMF_STANDARD));
    default:
        return (IPTR)0UL;
    }

    AROS_LIBFUNC_EXIT
} /* GetBitMapAttr */
