/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>
#include <aros/debug.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH6(ULONG, FillPixelArray,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , width	, D2),
	AROS_LHA(UWORD            , height	, D3),
	AROS_LHA(ULONG            , pixel	, D4),

/*  LOCATION */
	struct Library *, CyberGfxBase, 25, Cybergraphics)

/*  FUNCTION
        Writes the same color value to all pixels in a rectangular region of
        a RastPort.

    INPUTS
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of destination RastPort
            to write to (in pixels).
        width, height - size of the affected area (in pixels).
        pixel - the color value to use, in 32-bit ARGB format: 1 byte per
            component, in the order alpha, red, green, blue.

    RESULT
        count - the number of pixels filled.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    HIDDT_Color col;
    HIDDT_Pixel pix;

    /* HIDDT_ColComp are 16 Bit */
    col.alpha	= (HIDDT_ColComp)((pixel >> 16) & 0x0000FF00);
    col.red	= (HIDDT_ColComp)((pixel >> 8) & 0x0000FF00);
    col.green	= (HIDDT_ColComp)(pixel & 0x0000FF00);
    col.blue	= (HIDDT_ColComp)((pixel << 8) & 0x0000FF00);

    pix = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);

    return (LONG)FillRectPenDrMd(rp, destx, desty, destx + width  - 1,
        desty + height - 1, pix, vHidd_GC_DrawMode_Copy, TRUE);

    AROS_LIBFUNC_EXIT
} /* FillPixelArray */

