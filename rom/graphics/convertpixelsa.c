/*
    Copyright  1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function to convert pixels from one pixfmt into another
    Lang: english
*/

#include "graphics_intern.h"
#include <proto/graphics.h>
#include <proto/oop.h>

#include <aros/debug.h>

/*****************************************************************************

    NAME */

	AROS_LH9(ULONG, ConvertPixelsA,

/*  SYNOPSIS */
    	AROS_LHA(APTR, srcPixels, A0),
	AROS_LHA(ULONG, srcMod, D0),
	AROS_LHA(ULONG, srcPixFmt, D1),
	AROS_LHA(APTR, dstPixels, A1),
	AROS_LHA(ULONG, dstMod, D2),
	AROS_LHA(ULONG, dstPixFmt, D3),
	AROS_LHA(ULONG, width, D4),
	AROS_LHA(ULONG, height, D5),
	AROS_LHA(struct TagItem *, tags, A2),
	
/*  LOCATION */
	struct GfxBase *, GfxBase, 199, Graphics)

/*  FUNCTION
    	Convert pixels in pixel buffer srcPixels from srcPixFmt to
	dstPixFmt putting result into pixel buffer dstPixels.
	
    INPUTS
        srcPixels, dstPixels - Pointer to source/dest pixel buffer
    	srcPixFmt, dstPixFmt - One of the truecolor vHidd_StdPixFmt_#?
	srcMod, dstMod - Modulo for src/dest pixel buffer
	width, height - size of area to convert
	tags - none defined yet
	
    RESULT
    	0 on failure (bad pixfmts), 1 on success.
	
    NOTES
	
    EXAMPLE
	
    BUGS

    SEE ALSO
    	<hidd/graphics.h>
	
    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
	
    OOP_Object *srcpf, *dstpf, *bm;
    APTR    	src = srcPixels;
    APTR    	dst = dstPixels;
    
    (void)tags;
    
    if (!SDD(GfxBase)->gfxhidd) return 0;
    bm = SDD(GfxBase)->bm_bak;
    if (!bm)
        bm = SDD(GfxBase)->framebuffer;
    if (!bm)
        return 0;
    
    srcpf = HIDD_Gfx_GetPixFmt(SDD(GfxBase)->gfxhidd, srcPixFmt);
    dstpf = HIDD_Gfx_GetPixFmt(SDD(GfxBase)->gfxhidd, dstPixFmt);

    if (!srcpf || !dstpf)
    {
    	D(bug("graphics.library/ConvertPixelsA(): Bad source (%d) or dest (%d) pixfmt!\n", srcPixFmt, dstPixFmt));
	return 0;
    }
        	
    HIDD_BM_ConvertPixels(bm, &src, srcpf, srcMod, 
    	    	    	  &dst, dstpf, dstMod,
			  width, height, NULL);
        
    return 1;
    
    AROS_LIBFUNC_EXIT
	
} /* ConvertPixelsA */
