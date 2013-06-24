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

	AROS_LH2(APTR, LockBitMapTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , bitmap, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 28, Cybergraphics)

/*  FUNCTION
        Obtains exclusive access to a bitmap in preparation for direct access
        to its pixel data. Direct access to a bitmap should only be done in
        exceptional cases, and the locking period should be limited to at most
        one frame.

        A taglist is passed in that contains pointers to variables in which to
        store the information necessary to directly access the bitmap. The
        tags used are as follows:
            LBMI_WIDTH (ULONG *) - the bitmap's width.
            LBMI_HEIGHT (ULONG *) - the bitmap's height.
            LBMI_DEPTH (ULONG *) - the bitmap's depth.
            LBMI_PIXFMT (ULONG *) - the bitmap's pixel format.
            LBMI_BYTESPERPIX (ULONG *) - the number of bytes per pixel.
            LBMI_BYTESPERROW (ULONG *) - the number of bytes per row.
            LBMI_BASEADDRESS (APTR *) - the start address of the pixel data.

        The value returned for LBMI_PIXFMT will be one of the following
        constants:
                PIXFMT_RGB24 - 3 bytes per pixel: 1 byte per component, in
                    the order red, green, blue.
                PIXFMT_RGBA32 - 4 bytes per pixel: 1 byte per component, in  
                    the order red, green, blue, alpha.
                PIXFMT_ARGB32 - 4 bytes per pixel: 1 byte per component, in
                    the order alpha, red, green, blue.
                PIXFMT_LUT8 - 1 byte per pixel: each byte is a pen number
                    rather than a direct colour value.
                PIXFMT_RGB15 - 2 bytes per pixel: one unused bit, then 5 bits
                    per component, in the order red, green, blue.
                PIXFMT_BGR15 - 2 bytes per pixel: 1 unused bit, then 5 bits
                    per component, in the order blue, green, red.
                PIXFMT_RGB15PC - 2 bytes per pixel, accessed as a little
                    endian value: 1 unused bit, then 5 bits per component, in
                    the order red, green, blue.
                PIXFMT_BGR15PC - 2 bytes per pixel, accessed as a little
                    endian value: 1 unused bit, then 5 bits per component, in
                    the order blue, green, red.
                PIXFMT_RGB16 - 2 bytes per pixel: 5 bits for red, then 6 bits
                    for green, then 5 bits for blue.
                PIXFMT_BGR16 - 2 bytes per pixel: 5 bits for blue, then 6 bits
                    for green, then 5 bits for red.
                PIXFMT_RGB16PC - 2 bytes per pixel, accessed as a little
                    endian value: 5 bits for red, then 6 bits for green, then
                    5 bits for blue. 
                PIXFMT_BGR16PC - 2 bytes per pixel, accessed as a little
                    endian value: 5 bits for blue, then 6 bits for green, then
                    5 bits for red.  
                PIXFMT_BGR24 - 3 bytes per pixel: 1 byte per component, in
                    the order blue, green, red.
                PIXFMT_BGRA32 - 4 bytes per pixel: 1 byte per component, in
                    the order blue, green, red, alpha.
                PIXFMT_ABGR32 - 4 bytes per pixel: 1 byte per component, in
                    the order alpha, blue, green, red (AROS extension).
                PIXFMT_0RGB32 - 4 bytes per pixel: 1 unused byte, then 1 byte
                    per component, in the order red, green, blue (AROS
                    extension).
                PIXFMT_BGR032 - 4 bytes per pixel: 1 byte per component, in
                    the order blue, green, red, followed by 1 unused byte
                    (AROS extension).
                PIXFMT_RGB032 - 4 bytes per pixel: 1 byte per component, in
                    the order red, green, blue, followed by 1 unused byte
                    (AROS extension).
                PIXFMT_0BGR32 - 4 bytes per pixel: 1 unused byte, then 1 byte
                    per component, in the order blue, green, red (AROS
                    extension).

    INPUTS
        bitmap - the bitmap to lock.
        tags - a taglist that will be filled with information necessary to
            directly access the bitmap.

    RESULT
        handle - a handle to be passed to UnLockBitMap() or
            UnLockBitMapTagList(), or NULL for failure.

    NOTES
        While the bitmap is locked, no cybergraphics.library or
        graphics.library related functions should be called (except to unlock
        it).

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockBitMap(), UnLockBitMapTagList() 

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct BitMap *bm = bitmap;
    struct TagItem *tag;
    UBYTE *baseaddress;
    ULONG width, height, banksize, memsize;
    OOP_Object *pf;
    IPTR cpf;
    
    if (!IS_HIDD_BM(bm))
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-HIDD BM !!!\n"));
	return NULL;
    }

    OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_CgxPixFmt, &cpf);
    if (-1 == cpf)
    {
    	D(bug("!!! TRYING TO CALL LockBitMapTagList() ON NON-CYBER PIXFMT BITMAP !!!\n"));
	return NULL;
    }
    
    /* Get some info from the bitmap object */
    if (!HIDD_BM_ObtainDirectAccess(HIDD_BM_OBJ(bm), &baseaddress, &width, &height, &banksize, &memsize)) {
        D(bug("!!! CAN'T HIDD_BM_ObtainDirectAccess() on the object\n"));
        return NULL;
    }
    
    while ((tag = NextTagItem(&tags)))
    {
    	switch (tag->ti_Tag)
	{
	    case LBMI_BASEADDRESS:
	    	*((IPTR **)tag->ti_Data) = (IPTR *)baseaddress;
	    	break;

	    case LBMI_BYTESPERROW:
        OOP_GetAttr(HIDD_BM_OBJ(bm), aHidd_BitMap_BytesPerRow, (IPTR *)tag->ti_Data);
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
