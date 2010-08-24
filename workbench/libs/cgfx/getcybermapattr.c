/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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
#include <clib/cybergraphics_protos.h>

	AROS_LH2(ULONG, GetCyberMapAttr,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, bitMap, A0),
	AROS_LHA(ULONG          , attribute, D0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 16, Cybergraphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

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
