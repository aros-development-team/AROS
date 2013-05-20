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

	AROS_LH5(ULONG, InvertPixelArray,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp		, A1),
	AROS_LHA(UWORD            , destx	, D0),
	AROS_LHA(UWORD            , desty	, D1),
	AROS_LHA(UWORD            , width	, D2),
	AROS_LHA(UWORD            , height	, D3),

/*  LOCATION */
	struct Library *, CyberGfxBase, 24, Cybergraphics)

/*  FUNCTION
        Inverts each pixel in rectangular portion of a RastPort, i.e. applies
        a NOT operation to each bit of pixel data.

    INPUTS
        rp - the RastPort to write to.
        destx, desty - top-lefthand corner of portion of RastPort to invert.
        width, height - size of the area to invert.

    RESULT
        count - the number of pixels inverted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(rp->BitMap))
    {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap "
            "in InvertPixelArray() !!!\n"));
    	return 0;
    }

    return (LONG)FillRectPenDrMd(rp, destx, desty, destx + width  - 1,
        desty + height - 1, 0xFF, vHidd_GC_DrawMode_Invert, TRUE);

    AROS_LIBFUNC_EXIT
} /* InvertPixelArray */
