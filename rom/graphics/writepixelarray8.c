/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH7(LONG, WritePixelArray8,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(ULONG            , xstart, D0),
	AROS_LHA(ULONG            , ystart, D1),
	AROS_LHA(ULONG            , xstop, D2),
	AROS_LHA(ULONG            , ystop, D3),
	AROS_LHA(UBYTE           *, array, A2),
	AROS_LHA(struct RastPort *, temprp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 131, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
   
    HIDDT_PixelLUT pixlut;
    LONG    	   pixwritten;
    
    EnterFunc(bug("WritePixelArray8(%p, %d, %d, %d, %d)\n",
    	rp, xstart, ystart, xstop, ystop));
	
    pixlut.entries = AROS_PALETTE_SIZE;
    pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;
        
    if (!pixlut.pixels)
    {
    	if (GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8)
	{
	    D(bug("WritePixelArray8: Can't work on hicolor/truecolor screen without LUT"));
    	    ReturnInt("WritePixelArray8", LONG, 0);
	}
    }
    
    pixwritten = write_pixels_8(rp, array
    	, ((xstop - xstart + 1) + 15) & ~15 /* modulo */
	, xstart, ystart
	, xstop, ystop
	, &pixlut
	, GfxBase);

    ReturnInt("WritePixelArray8", LONG, pixwritten);

    AROS_LIBFUNC_EXIT
    
} /* WritePixelArray8 */
