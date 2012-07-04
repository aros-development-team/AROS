/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code for miscellaneous operations needed bz graphics
    Lang: english
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <clib/macros.h>
#include <exec/memory.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"



BOOL pattern_pen(struct RastPort *rp, WORD x, WORD y, ULONG apen, ULONG bpen, ULONG *pixval_ptr, struct GfxBase *GfxBase)
{
    ULONG idx, mask;
    ULONG set_pixel, pixval = 0;
    
    ULONG drmd = GetDrMd(rp);
    ULONG pattern_height = 1L << ABS(rp->AreaPtSz);
    const UBYTE *apt = (UBYTE *)rp->AreaPtrn;
    
		    
    idx = COORD_TO_BYTEIDX(x & 0x0F, y & (pattern_height - 1), 2);
    mask = XCOORD_TO_MASK( x );

/* kprintf("palette_pen: idx=%d, mask=%d,apen=%d, bpen=%d, drmd=%d, apt[idx]=%d\n"
	, idx, mask, apen, bpen, drmd, apt[idx]);
*/	    
    /* Mono- or multicolor ? */
    if (rp->AreaPtSz > 0)
    {
	/* mono */
	set_pixel = apt[idx] & mask;
	if (drmd & INVERSVID)
	    set_pixel = ((set_pixel != 0) ? 0UL : 1UL );
			
	if (set_pixel)
	{
	    /* Use FGPen to render */
	    pixval = apen;
/* kprintf("use apen\n");	    
*/	}
	else
	{
	    if ((drmd & JAM2) != 0)
	    {
		pixval = bpen;
		set_pixel = TRUE;
/* kprintf("use bpen\n");	    
*/	    }
	    else
	    {   
		/* Do not set pixel */
		set_pixel = FALSE;
	    }
			
	}

			
    }
    else
    {
	UBYTE i, depth;
	ULONG plane_size, pen_mask = 0;
	const UBYTE *plane;
			
	plane_size = (/* bytesperrow = */ 2 ) * pattern_height;
	depth = GetBitMapAttr(rp->BitMap, BMA_DEPTH);
	
	plane = apt;
			
	/* multicolored pattern, get pixel from all planes */
	for (i = 0; i < depth; i ++)
	{

	    pen_mask <<= 1;
	
	    if ((plane[idx] & mask) != 0)
		pixval |= pen_mask;
	}
	plane += plane_size;
			
	set_pixel = TRUE;
    }
    
    if (set_pixel)
    	*pixval_ptr = pixval;
    
    return set_pixel;
}
