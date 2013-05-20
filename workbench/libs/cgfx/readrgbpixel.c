/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <aros/debug.h>

#include "cybergraphics_intern.h"

struct render_data
{
    HIDDT_Pixel pixel;
};

static LONG PixelHook(struct render_data *data, OOP_Object *bm, OOP_Object *gc,
    LONG x, LONG y, struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH3(ULONG, ReadRGBPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp	, A1),
	AROS_LHA(UWORD            , x	, D0),
	AROS_LHA(UWORD            , y	, D1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 18, Cybergraphics)

/*  FUNCTION
        Reads a particular pixel's color value from a RastPort.

    INPUTS
        rp - the RastPort to read from.
        x, y - the coordinates of the pixel to read.

    RESULT
        color - the pixel's color value in 32-bit ARGB format: 1 byte
            per component, in the order alpha, red, green, blue.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct render_data data;
    HIDDT_Color col;
    HIDDT_Pixel pix;
    LONG ret;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in ReadRGBPixel()!!!\n"));
    	return (ULONG)-1;
    }

    /* Get the HIDD pixel val */
    ret = DoPixelFunc(rp, x, y, PixelHook, &data, FALSE);

    if (-1 == ret)
    	return (ULONG)-1;

    HIDD_BM_UnmapPixel(HIDD_BM_OBJ(rp->BitMap), data.pixel, &col);

    /* HIDDT_ColComp are 16 Bit */

    pix = ((col.alpha & 0xFF00) << 16)
        | ((col.red & 0xFF00) << 8)
        | (col.green & 0xFF00)
        | ((col.blue & 0xFF00) >> 8);

    return pix;

    AROS_LIBFUNC_EXIT
} /* ReadRGBPixel */

static LONG PixelHook(struct render_data *data, OOP_Object *bm, OOP_Object *gc,
    LONG x, LONG y, struct GfxBase *GfxBase)
{   
    data->pixel = HIDD_BM_GetPixel(bm, x, y);
    return 0;
}

