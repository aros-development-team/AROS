/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

struct extcol_render_data
{
    struct BitMap      *destbm;
    HIDDT_Pixel		pixel;
    struct IntCGFXBase *CyberGfxBase;
};

static ULONG extcol_render(struct extcol_render_data *ecrd, LONG x_src, LONG y_src,
			   OOP_Object *bm_obj, OOP_Object *dst_gc,
			   struct Rectangle *rect, struct GfxBase *GfxBase)
{
    /* Get the info from the hidd */
    struct IntCGFXBase *CyberGfxBase = ecrd->CyberGfxBase;
    struct BitMap      *bm     = ecrd->destbm;
    HIDDT_Pixel	       *pixbuf = CyberGfxBase->pixel_buf;
    LONG   x_dest = rect->MinX;
    LONG   y_dest = rect->MinY;
    ULONG  xsize  = rect->MaxX - rect->MinX + 1;
    ULONG  ysize  = rect->MaxY - rect->MinY + 1;
    LONG   pixels_left_to_process = xsize * ysize;
    ULONG  next_x = 0;
    ULONG  next_y = 0;
    ULONG  current_x, current_y, tocopy_w, tocopy_h;

    LOCK_PIXBUF

    while (pixels_left_to_process)
    {
	LONG srcx, srcy, dstx, dsty;
	LONG x, y;

	current_x = next_x;
	current_y = next_y;

	if (NUMPIX < xsize)
	{
	   /* buffer can't hold a single horizontal line, and must 
	      divide each line into copies */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
	    }
	    else
	    {	/* Start at a new line */
	    
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;	    
    	}
    	else
    	{
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);
	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
    	}

	srcx = x_src  + current_x;
	srcy = y_src  + current_y;
	dstx = x_dest + current_x;
	dsty = y_dest + current_y;

	/* Get some more pixels from the HIDD */
	HIDD_BM_GetImage(bm_obj, (UBYTE *)pixbuf, tocopy_w,
			 srcx, srcy, tocopy_w, tocopy_h, vHidd_StdPixFmt_Native32);

	/*  Write pixels to the destination */
	for (y = 0; y < tocopy_h; y ++)
    	{
    	    for (x = 0; x < tocopy_w; x ++)
    	    {
	    	if (*pixbuf ++ == ecrd->pixel)
	    	{	
		    ULONG i;

	    	    /* Set the according bit in the bitmap */
		    for (i = 0; i < bm->Depth; i++)
		    {
		    	UBYTE *plane = bm->Planes[i];

			if (NULL != plane)
		    	{
		    	    UBYTE mask = XCOORD_TO_MASK(x + dstx);

			    plane += COORD_TO_BYTEIDX(x + dstx, y + dsty, bm->BytesPerRow);
			    /* Set the pixel */
			    *plane |= mask;
		    	} /* if (plane allocated) */
		    } /* for (plane) */
	    	} /* if (color match) */
	    } /* for (x) */
    	} /* for (y) */

	pixels_left_to_process -= (tocopy_w * tocopy_h);
    }

    ULOCK_PIXBUF

    return 1;
}

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH7(ULONG, ExtractColor,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, RastPort, A0),
	AROS_LHA(struct BitMap   *, SingleMap, A1),
	AROS_LHA(ULONG            , Colour, D0),
	AROS_LHA(ULONG            , sX, D1),
	AROS_LHA(ULONG            , sY, D2),
	AROS_LHA(ULONG            , Width, D3),
	AROS_LHA(ULONG            , Height, D4),

/*  LOCATION */
	struct Library *, CyberGfxBase, 31, Cybergraphics)

/*  FUNCTION
        Create a single-plane bitmap that describes the coordinates where a
        particular colour is present in a portion of a RastPort (i.e. a mask).
        A one is stored in the bitmap where the requested colour is present,
        and a zero where it is absent.

        For true-colour RastPorts, the colour is specified in 32-bit ARGB
        format: 1 byte per component, in the order alpha, red, green, blue
        (the alpha byte is ignored for RastPorts without an alpha channel).
        For paletted RastPorts, a pen number is given instead.

    INPUTS
        RastPort - the RastPort to analyse.
        SingleMap - a planar bitmap to fill (its pixel dimensions must be at
            least as big as the rectangle being analysed).
        Colour - the colour to extract.
        sX, sY - top-lefthand corner of portion of RastPort to analyse
            (in pixels).
        Width, Height - size of the area to analyse (in pixels).

    RESULT
        result - Boolean success indicator.

    NOTES
        It is safe for the bitmap being filled to have more than one bitplane.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Rectangle rr;
    LONG pixread = 0;
    struct extcol_render_data ecrd;
    OOP_Object *pf;
    IPTR colmod;

    if (!IS_HIDD_BM(RastPort->BitMap))
    {
    	D(bug("!!! CALLING ExtractColor() ON NO-HIDD BITMAP !!!\n"));
	return FALSE;
    }

    rr.MinX = sX;
    rr.MinY = sY;
    rr.MaxX = sX + Width  - 1;
    rr.MaxY = sY + Height - 1;

    OOP_GetAttr(HIDD_BM_OBJ(RastPort->BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, (IPTR *)&colmod);

    if (vHidd_ColorModel_Palette == colmod)
    {
        ecrd.pixel = Colour;
    }
    else
    {
	HIDDT_Color col;

	col.alpha = (Colour >> 16) & 0x0000FF00;
	col.red	  = (Colour >> 8 ) & 0x0000FF00;
	col.green = Colour & 0x0000FF00;
	col.blue  = (Colour << 8) & 0x0000FF00;

	ecrd.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(RastPort->BitMap), &col);    
    }

    ecrd.destbm	      = SingleMap;
    ecrd.CyberGfxBase = (struct IntCGFXBase *)CyberGfxBase;

    pixread = DoRenderFunc(RastPort, NULL, &rr, extcol_render, &ecrd, FALSE);

    /*
     * In our render callback we return one if everything went ok.
     * DoRenderFunc() sums up all return values from callback. So, if all failed,
     * this will be zero. Otherwise it will indicate how many times the callback
     * was called.
     */
    return (pixread > 0);

    AROS_LIBFUNC_EXIT
} /* ExtractColor */
