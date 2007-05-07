/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set one color register for this Viewport
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <proto/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/view.h>
#include <proto/graphics.h>

	AROS_LH5(void, SetRGB32,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),
	AROS_LHA(ULONG            , n, D0),
	AROS_LHA(ULONG            , r, D1),
	AROS_LHA(ULONG            , g, D2),
	AROS_LHA(ULONG            , b, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 142, Graphics)

/*  FUNCTION
	Changes a single color of a viewport.

    INPUTS
	vp - Modify this viewport
	n - Change this color. If the color is outside the range of
		valid colors, it will be ignored.
	r, g, b - The new values for the red, green and blue. The
		valid range is from 0x000000 (no intensity) to
		0xFFFFFFFF (full intensity).

    RESULT
	If there is a ColorMap for this viewport, then the value will
	be stored in the ColorMap.
	The selected color register is changed to match your specs.
	If the color value is unused then nothing will happen.

    NOTES
	Lower order bits of the palette specification will be discarded,
	depending on the color palette resolution of the target graphics
	device. Use 0xffffffff for the full value, 0x7fffffff for 50%,
	etc. You can find out the palette range for your screen by
	querying the graphics data base.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct BitMap   	*bm;
    HIDDT_Color     	hidd_col;
    OOP_Object      	*pf;
    HIDDT_ColorModel 	colmod;
 
    if (vp->ColorMap) SetRGB32CM(vp->ColorMap, n, r, g, b);
    
    /* Get bitmap object */
    bm = vp->RasInfo->BitMap;

    if (!IS_HIDD_BM(bm))
    {
    	 D(bug("!!!!! Trying to use SetRGB32() call on non-hidd bitmap!!!\n"));
    	 return;
    }
    
    if (NULL == HIDD_BM_COLMAP(bm))
    {
    	 D(bug("!!!!! Trying to use SetRGB32() call on bitmap with no CLUT !!!\n"));
	 return;
    }
   
   
    /* HIDDT_Color entries are UWORD */
    hidd_col.red   = r >> 16;
    hidd_col.green = g >> 16 ;
    hidd_col.blue  = b >> 16;
    hidd_col.alpha = 0;

    OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

    if (vHidd_ColorModel_Palette == colmod || vHidd_ColorModel_TrueColor == colmod)
    {
   	 HIDD_BM_SetColors(HIDD_BM_OBJ(bm), &hidd_col, n, 1);

	 /*
	  bug("SetRGB32: bm %p, hbm %p, col %d (%x %x %x %x) mapped to %x\n"
			 , bm
			 , HIDD_BM_OBJ(bm)
			 , n
			 , hidd_col.red, hidd_col.green, hidd_col.blue, hidd_col.alpha
			 , hidd_col.pixval);

	 */
	 HIDD_BM_PIXTAB(bm)[n] = hidd_col.pixval;
    }

    AROS_LIBFUNC_EXIT
    
} /* SetRGB32 */
