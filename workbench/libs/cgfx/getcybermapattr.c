/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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
    
    OOP_Object *bm_obj;
    OOP_Object *pf;
    
    IPTR retval;
    
    /* This is cybergraphx. We only work wih HIDD bitmaps */
    if (!IS_HIDD_BM(bitMap)) {
    	D(bug("!!!!! Trying to use CGFX call on non-hidd bitmap in GetCyberMapAttr() !!!\n"));
    	return 0;
    }
	
    bm_obj = HIDD_BM_OBJ(bitMap);
    
    OOP_GetAttr(bm_obj, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    switch (attribute) {
   	case CYBRMATTR_XMOD:
	    OOP_GetAttr(bm_obj, aHidd_BitMap_BytesPerRow, &retval);
	    break;
	
   	case CYBRMATTR_BPPIX:
	    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &retval);
	    break;
	
   	case CYBRMATTR_PIXFMT:
        case CYBRMATTR_PIXFMT_ALPHA: {
	    IPTR stdpf;
	    UWORD cpf;
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, (IPTR *)&stdpf);
	    
	    /* Convert to cybergfx */
	    cpf = hidd2cyber_pixfmt(stdpf);

            /* CYBRMATTR_PIXFMT doesn't know about non-alpha 32-bit modes */
            if (attribute == CYBRMATTR_PIXFMT) {
                switch (cpf) {
                    case PIXFMT_0RGB32: cpf = PIXFMT_ARGB32; break;
                    case PIXFMT_BGR032: cpf = PIXFMT_BGRA32; break;
                    case PIXFMT_RGB032: cpf = PIXFMT_RGBA32; break;
                    case PIXFMT_0BGR32: cpf = PIXFMT_ABGR32; break;
                }
            }
	    
	    if (cpf == (UWORD)-1) {
	    	D(bug("!!! UNKNOWN PIXEL FORMAT IN GetCyberMapAttr()\n"));
	    }
	    
	    retval = (IPTR)cpf;
	    break;
	    
	}
	
   	case CYBRMATTR_WIDTH:
	#if 0 /* stegerg: doesn't really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(bm_obj, aHidd_BitMap_Width, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_WIDTH);
	#endif
	    break;
	
   	case CYBRMATTR_HEIGHT:
	#if 0 /* stegerg: doesn't really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(bm_obj, aHidd_BitMap_Height, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_HEIGHT);
	#endif
	    break;
	
   	case CYBRMATTR_DEPTH:
	#if 0 /* stegerg: might not really work, because of framebuffer bitmap object stuff */
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &retval);
	#else
	    retval = GetBitMapAttr(bitMap, BMA_DEPTH);
	#endif
	    break;
	
   	case CYBRMATTR_ISCYBERGFX: {
	    IPTR depth;
	    
	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    
	    if (depth < 8) {
	    	retval = 0;
	    } else {
	    /* We allways have a HIDD bitmap */
	    	retval = 0xFFFFFFFF; /* Some apps seem to rely on this retval */
	    }
	    break; }
	
   	case CYBRMATTR_ISLINEARMEM:
	    OOP_GetAttr(bm_obj, aHidd_BitMap_IsLinearMem, &retval);
	    break;
	
	default:
	    D(bug("!!! UNKNOWN ATTRIBUTE PASSED TO GetCyberMapAttr()\n"));
	    break;
	
	
    } /* switch (attribute) */
    
    return retval;

    AROS_LIBFUNC_EXIT
} /* GetCyberMapAttr */
