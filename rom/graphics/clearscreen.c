/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ClearScreen()
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


	AROS_LH1(void, ClearScreen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 8, Graphics)

/*  FUNCTION

    Clear from the current position to the end of the rastport. Clearing
    means setting the colour to 0 (or to BgPen if the drawmode is JAM2).
    This includes a ClearEOL().

    INPUTS

    rp   --  pointer to rastport

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    Text(), ClearEOL(), SetRast(), graphics/text.h, graphics/rastport.h

    INTERNALS

    HISTORY

    24.7.98  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    ULONG ymin   = rp->cp_y - rp->TxBaseline + rp->Font->tf_YSize;
    ULONG height = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);
    ULONG width  = GetBitMapAttr(rp->BitMap, BMA_WIDTH);

    ClearEOL(rp);

    if(height >= ymin)
    {
	ULONG      oldDrMd = GetDrMd(rp);

	SetDrMd(rp, oldDrMd ^ INVERSVID);
	RectFill(rp, 0, ymin, width - 1, height - 1);	
	SetDrMd(rp, oldDrMd);
    }
	
    AROS_LIBFUNC_EXIT
} /* ClearScreen */
