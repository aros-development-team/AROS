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

	AROS_LH2(APTR, LockBitMapTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , bitmap, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 28, Cybergraphics)

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
    
    struct BitMap *bm = bitmap;
    struct TagItem *tag;
    UBYTE *baseaddress;
    ULONG width, height, banksize, memsize;
    OOP_Object *pf;
    IPTR stdpf;
    UWORD cpf;
    
    if (!IS_HIDD_BM(bm))
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-HIDD BM !!!\n"));
	return NULL;
    }

    /* FIXME: What is it? Why?
    if (CyberGfxBase->Flags & GFXFLAG_PREVENT_DIRECT_BITMAP_ACCESS)
    {
    	return NULL;
    } */
    
    OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
    cpf = hidd2cyber_pixfmt(stdpf);
    if (((UWORD)-1) == cpf)
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-CYBER PIXFMT BITMAP !!!\n"));
	return NULL;
    }
    
    /* Get some info from the bitmap object */
    if (!HIDD_BM_ObtainDirectAccess(HIDD_BM_OBJ(bm), &baseaddress, &width, &height, &banksize, &memsize))
    	return NULL;
    
    
    while ((tag = NextTagItem((const struct TagItem **)&tags)))
    {
    	switch (tag->ti_Tag)
	{
	    case LBMI_BASEADDRESS:
	    	*((IPTR **)tag->ti_Data) = (IPTR *)baseaddress;
	    	break;
		
	    case LBMI_BYTESPERROW:
#if 1
    	    /* stegerg: I assume here that width returned by ObtainDirectAccess() is always the aligned
	                bitmap width, so that bytes per row can simply be calculated by multiplicating
			it with bytes per pixel. Nvidia and Radeon hidd override HIDD_BM_BytesPerLine
			for some strange reason and can return wrong result, because their real alignment
			is done on number of bytes, not number of pixels. If later this alignment calc
			is done again, but based on number of pixels, it may return different/wrong result. */
			
    	    {
    	    	IPTR bpp;
		
	    	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bpp);
		*((IPTR *)tag->ti_Data) = bpp * width;
    	    				
    	    }
#else	    
	    	*((IPTR *)tag->ti_Data) = 
			(ULONG)HIDD_BM_BytesPerLine(HIDD_BM_OBJ(bm), stdpf, width);
#endif
	    	break;
	    
	    case LBMI_BYTESPERPIX:
	    	OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, (IPTR *)tag->ti_Data);
	    	break;
	    
	    case LBMI_PIXFMT: 
		*((IPTR *)tag->ti_Data) = (IPTR)cpf;
	    	break;
		
	    case LBMI_DEPTH:
	    	OOP_GetAttr(pf, aHidd_PixFmt_Depth, (IPTR *)tag->ti_Data);
		break;
	    
	    case LBMI_WIDTH:
	    	OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_Width, (IPTR *)tag->ti_Data);
	    	break;
	    
	    case LBMI_HEIGHT:
	    	OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_Height, (IPTR *)tag->ti_Data);
	    	break;
		
	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO LockBitMapTagList() !!!\n"));
		break;
	}
    }
    
    return HIDD_BM_OBJ(bm);

    AROS_LIBFUNC_EXIT
} /* LockBitMapTagList */
