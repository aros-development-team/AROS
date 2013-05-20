/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH2(ULONG, GetCyberMapAttr,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, bitMap, A0),
	AROS_LHA(ULONG          , attribute, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 16, Cybergraphics)

/*  FUNCTION
        Provides information about an RTG bitmap. If you are unsure whether
        the bitmap is an RTG one, you must retrieve and check
        CYBRMATTR_ISCYBERGFX first, as all other attributes are only allowed
        to be retrieved for RTG bitmaps.

    INPUTS
        bitMap - an RTG bitmap.
        attribute - one of the following bitmap attributes:
            CYBRMATTR_PIXFMT - the bitmap's pixel format. See
                LockBitMapTagList() for possible values.
            CYBRMATTR_WIDTH - the bitmap's width (in pixels).
            CYBRMATTR_HEIGHT - the bitmap's height (in pixels).
            CYBRMATTR_DEPTH - the number of bits per pixel.
            CYBRMATTR_BPPIX - the number of bytes per pixel.
            CYBRMATTR_XMOD - the number of bytes per row.
            CYBRMATTR_ISCYBERGFX - TRUE only if the bitmap is an RTG one.
            CYBRMATTR_ISLINEARMEM - TRUE only if the bitmap's display buffer
                is linear.
            CYBRMATTR_COLORMAP - the bitmap's color map.

    RESULT
        value - the value associated with the requested attribute.

    NOTES
        If an unknown attribute is requested, -1 is returned.

    EXAMPLE

    BUGS
        CYBRMATTR_COLORMAP is unimplemented.

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    OOP_Object *bm_obj = NULL;
    OOP_Object *pf = NULL;
    
    IPTR retval;
    
    if (IS_HIDD_BM(bitMap))
    {
	bm_obj = HIDD_BM_OBJ(bitMap);
	OOP_GetAttr(bm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    }

    switch (attribute) {
    case CYBRMATTR_XMOD:
	if (bm_obj)
	    OOP_GetAttr(bm_obj, aHidd_BitMap_BytesPerRow, &retval);
	else
	    retval = bitMap->BytesPerRow;
	break;

    case CYBRMATTR_BPPIX:
	if (pf)
	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
	else
	    retval = 0;
	break;

    case CYBRMATTR_PIXFMT:
    case CYBRMATTR_PIXFMT_ALPHA:
	if (pf)
	{
	    IPTR stdpf;

	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
	    retval = hidd2cyber_pixfmt[stdpf];

	    /* Backwards compatibility kludge. To be removed.
	       Used only by Cairo, do not use CYBRMATTR_PIXFMT_ALPHA
	       anywhere else. */
	    if ((attribute == CYBRMATTR_PIXFMT_ALPHA) &&
	        (stdpf >= vHidd_StdPixFmt_0RGB32) && (stdpf >= vHidd_StdPixFmt_0BGR32))
		retval += 91;

	    D(bug("[GetCyberMapAttr] Pixel format is %d\n", retval));
	}
	else
	    retval = -1;
	break;

    case CYBRMATTR_WIDTH:
	retval = GetBitMapAttr(bitMap, BMA_WIDTH);
	break;
	
    case CYBRMATTR_HEIGHT:
	retval = GetBitMapAttr(bitMap, BMA_HEIGHT);
	break;
	
    case CYBRMATTR_DEPTH:
	retval = GetBitMapAttr(bitMap, BMA_DEPTH);
	break;
	
    case CYBRMATTR_ISCYBERGFX:
	retval = bm_obj ? TRUE : FALSE;
	break;

    case CYBRMATTR_ISLINEARMEM:
	if (bm_obj)
	    OOP_GetAttr(bm_obj, aHidd_BitMap_IsLinearMem, &retval);
	else
	    retval = TRUE;
	break;

    default:
	D(bug("!!! UNKNOWN ATTRIBUTE PASSED TO GetCyberMapAttr()\n"));
	break;
    } /* switch (attribute) */

    return retval;

    AROS_LIBFUNC_EXIT
} /* GetCyberMapAttr */
