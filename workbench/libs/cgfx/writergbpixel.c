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

	AROS_LH4(LONG, WriteRGBPixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , x		, D0),
	AROS_LHA(UWORD            , y		, D1),
	AROS_LHA(ULONG            , pixel	, D2),

/*  LOCATION */
	struct Library *, CyberGfxBase, 19, Cybergraphics)

/*  FUNCTION
        Writes a new color value to a pixel in a RastPort.

    INPUTS
        rp - the RastPort to write to.
        x, y - the coordinates of the pixel to write.
        pixel - the pixel's new color value in 32-bit ARGB format: 1 byte
            per component, in the order alpha, red, green, blue.

    RESULT
        error - 0 if no error occurred, or -1 if (x, y) is outside the
            RastPort.

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
    LONG retval;

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
        D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in WriteRGBPixel() !!!\n"));
        return 0;
    }

    /* HIDDT_ColComp are 16 Bit */

    col.alpha = (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red = (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green = (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue = (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);

    data.pixel = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);

    retval = DoPixelFunc(rp, x, y, PixelHook, &data, TRUE);

    return retval;

    AROS_LIBFUNC_EXIT
} /* WriteRGBPixel */

static LONG PixelHook(struct render_data *data, OOP_Object *bm, OOP_Object *gc,
    LONG x, LONG y, struct GfxBase *GfxBase)
{
    HIDD_BM_PutPixel(bm, x, y, data->pixel);

    return 0;
}

