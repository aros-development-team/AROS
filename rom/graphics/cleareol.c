/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ClearEOL()
    Lang: English
*/

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 0
#include <aros/debug.h>

	AROS_LH1(void, ClearEOL,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 7, Graphics)

/*  FUNCTION

    Clear a rectangular area from the current position to the end of the
    rastport, the height of which is the height of the current text font.

    INPUTS

    pr   --  pointer to rastport

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    Text(), ClearScreen(), SetRast(), graphics/text.h, graphics/rastport.h

    INTERNALS

    HISTORY

    24.7.98  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    UBYTE      oldFgPen;

    ULONG      width = GetBitMapAttr(rp->BitMap, BMA_WIDTH);

    oldFgPen = rp->FgPen;

    SetAPen(rp, (rp->DrawMode == JAM2) ? rp->BgPen : 0);

    RectFill(rp, rp->cp_x, rp->cp_y - rp->TxBaseline, width - 1,
	     rp->cp_y - rp->TxBaseline + rp->Font->tf_YSize - 1);

    SetAPen(rp, oldFgPen);

    AROS_LIBFUNC_EXIT
} /* ClearEOL */
