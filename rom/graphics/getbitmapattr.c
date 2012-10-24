/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get an attribute from a bitmap.
    Lang: english
*/
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <graphics/gfx.h>
#include <proto/graphics.h>
#include <proto/oop.h>

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

    IPTR retval;
    
    switch(attribute)
    {
	case BMA_HEIGHT:
            retval = (IPTR)bitmap->Rows;
	    break;
	    
	case BMA_WIDTH:
	  /* must return width in pixel! */
            retval = (IPTR)(bitmap->BytesPerRow * 8);
	    break;
	    
	case BMA_DEPTH:
	    if (IS_HIDD_BM(bitmap))
	    {
	    	retval = (IPTR)HIDD_BM_REALDEPTH(bitmap);
	    }
	    else
	    {
            	retval = (IPTR)bitmap->Depth;
	    }
	    break;
	    
	case BMA_FLAGS:
            retval = (IPTR)(bitmap->Flags & (BMF_DISPLAYABLE |
					     BMF_INTERLEAVED |
					     BMF_STANDARD));
	    break;
	    
	default:
            retval = 0;
	    break;
    }

    return retval;

    AROS_LIBFUNC_EXIT
        
} /* GetBitMapAttr */
