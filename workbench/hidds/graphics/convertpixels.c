/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <hidd/graphics.h>
#include "graphics_intern.h"
#include <aros/machine.h>

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#define SHIFT_PIX(pix, shift)	\
	(( (shift) < 0) ? (pix) >> (-shift) : (pix) << (shift) )


#define GETPIX32(s, pix)	\
	 pix = ( *((ULONG *)s) ++ );

#if AROS_BIG_ENDIAN
#define GETPIX24(s, pix) 	\
	pix = (      ( ((UBYTE *)s)[0] << 16 )	\
		   | ( ((UBYTE *)s)[1] << 8  )	\
		   | ( ((UBYTE *)s)[2] )	);	\
	((UBYTE *)s) += 3;

#else
#define GETPIX24(s, pix) 	\
	pix = (      ( ((UBYTE *)s)[2] << 16 )	\
		   | ( ((UBYTE *)s)[1] << 8  )	\
		   | ( ((UBYTE *)s)[0] )	);	\
	((UBYTE *)s) += 3;

#endif

#define GETPIX16(s, pix)	\
	pix = *((UWORD *)s) ++;


#define GETPIX8(s, pix)	\
	pix = *((UBYTE *)s) ++;

#define GET_TRUE_PIX(s, pix, pf)			\
	switch (pf->bytes_per_pixel) {			\
		case 4: GETPIX32(s, pix); break;	\
		case 3: GETPIX24(s, pix); break;	\
		case 2: GETPIX16(s, pix); break;	\
		default: D(bug("RUBBISH BYTES PER PIXEL IN GET_TRUE_PIX()\n")); break;	\
	}	


#define GET_PAL_PIX(s, pix, pf, lut)	\
	switch (pf->bytes_per_pixel) {			\
		case 4: GETPIX32(s, pix); break;	\
		case 3: GETPIX24(s, pix); break;	\
		case 2: GETPIX16(s, pix); break;	\
		case 1: GETPIX8 (s, pix); break;	\
		default: D(bug("RUBBISH BYTES PER PIXEL IN GET_PAL_PIX()\n")); break;	\
	}	\
	pix = lut[pix];

#define PUTPIX32(d, pix)	\
	*((ULONG *)d) ++ = pix;


#if AROS_BIG_ENDIAN	
#define PUTPIX24(d, pix)	\
	((UBYTE *)d)[0] = (UBYTE)((pix >> 16) & 0x000000FF);	\
	((UBYTE *)d)[1] = (UBYTE)((pix >> 8 ) & 0x000000FF);	\
	((UBYTE *)d)[2] = (UBYTE)( pix & 0x000000FF);	\
	((UBYTE *)d) += 3;
#else
#define PUTPIX24(d, pix)	\
	((UBYTE *)d)[2] = (UBYTE)((pix >> 16) & 0x000000FF);	\
	((UBYTE *)d)[1] = (UBYTE)((pix >> 8 ) & 0x000000FF);	\
	((UBYTE *)d)[0] = (UBYTE)( pix & 0x000000FF);	\
	((UBYTE *)d) += 3;

#endif

#define PUTPIX16(d, pix)	\
	*((UWORD *)d) ++ = (UWORD)pix;

#define PUT_TRUE_PIX(d, pix, pf)			\
	switch (pf->bytes_per_pixel) {			\
		case 4: PUTPIX32(d, pix); break;	\
		case 3: PUTPIX24(d, pix); break;	\
		case 2: PUTPIX16(d, pix); break;	\
		default: D(bug("RUBBISH BYTES PER PIXEL IN PUT_TRUE_PIX()\n")); break;	\
	}	





#define INIT_VARS()	\
	APTR src = *msg->srcPixels;			\
	APTR dst = *msg->dstBuf;			\
	
#define INIT_FMTVARS()	\
	HIDDT_PixelFormat *srcfmt = msg->srcPixFmt;	\
	HIDDT_PixelFormat *dstfmt = msg->dstPixFmt;

/****************************************************************************************/

static VOID true_to_true(OOP_Class *cl, OOP_Object *o,
    	    	    	 struct pHidd_BitMap_ConvertPixels *msg)
{
    LONG alpha_diff, red_diff, green_diff, blue_diff;

    INIT_VARS()
    INIT_FMTVARS()
    
    LONG x, y;
    
    
    alpha_diff	= srcfmt->alpha_shift	- dstfmt->alpha_shift;
    red_diff 	= srcfmt->red_shift 	- dstfmt->red_shift;
    green_diff  = srcfmt->green_shift	- dstfmt->green_shift;
    blue_diff	= srcfmt->blue_shift	- dstfmt->blue_shift;


#if 0
bug("true_to_true()\n: src = %x  dest = %x srcfmt = %d %d %d %d [%d] destfmt = %d %d %d %d [%d]\n",
	src, dst, srcfmt->alpha_shift, srcfmt->red_shift, srcfmt->green_shift, srcfmt->blue_shift, srcfmt->bytes_per_pixel,
		  dstfmt->alpha_shift, dstfmt->red_shift, dstfmt->green_shift, dstfmt->blue_shift, dstfmt->bytes_per_pixel);

bug("srcmasks = %p %p %p %p\n",
	srcfmt->alpha_mask,
	srcfmt->red_mask,
	srcfmt->green_mask,
	srcfmt->blue_mask);
bug("destmasks = %p %p %p %p  diffs = %d %d %d %d\n",
	dstfmt->alpha_mask,
	dstfmt->red_mask,
	dstfmt->green_mask,
	dstfmt->blue_mask,
	alpha_diff,
	red_diff,
	green_diff,
	blue_diff);
#endif	    
    for (y = 0; y < msg->height; y ++)
    {
    	APTR s = src;
	APTR d = dst;
	
    	for (x = 0; x < msg->width; x ++)
	{
	    /* Get the source pixel */
	    HIDDT_Pixel srcpix = 0, dstpix;
	    
	    GET_TRUE_PIX(s, srcpix, srcfmt);
	    
	    dstpix =      (SHIFT_PIX(srcpix & srcfmt->alpha_mask, alpha_diff) & dstfmt->alpha_mask)
	    		| (SHIFT_PIX(srcpix & srcfmt->red_mask  , red_diff)   & dstfmt->red_mask)
	    		| (SHIFT_PIX(srcpix & srcfmt->green_mask, green_diff) & dstfmt->green_mask)
	    		| (SHIFT_PIX(srcpix & srcfmt->blue_mask , blue_diff)  & dstfmt->blue_mask);

#if 0
	bug("[ %p, %p, %p, %p ] "
		, srcpix
		, srcpix & srcfmt->blue_mask
		, SHIFT_PIX(srcpix & srcfmt->blue_mask, blue_diff)
		, SHIFT_PIX(srcpix & srcfmt->blue_mask, blue_diff) & dstfmt->blue_mask);
		
#endif	    

// bug("[ %p => %p ] \n", srcpix, dstpix);
	    /* Write the pixel to the destination buffer */
	    PUT_TRUE_PIX(d, dstpix, dstfmt);
	    
	} /* for (x) */
	
	((UBYTE *)src) += msg->srcMod;
	((UBYTE *)dst) += msg->dstMod;
	
    } /* for (y) */
    
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
    
}
/****************************************************************************************/

static VOID true_to_pal(OOP_Class *cl, OOP_Object *o,
    	    	    	struct pHidd_BitMap_ConvertPixels *msg)
{
    D(bug("BitMap::ConvertPixels() : Truecolor to palette conversion not implemented yet\n"));
}

static VOID pal_to_true(OOP_Class *cl, OOP_Object *o,
    	    	    	struct pHidd_BitMap_ConvertPixels *msg)
{
    HIDDT_Pixel *lut;

    INIT_VARS()
    INIT_FMTVARS()
    
    LONG x, y;
    
    lut = msg->pixlut->pixels;
	
    for (y = 0; y < msg->height; y ++)
    {
    	APTR s = src;
	APTR d = dst;
	
	for (x = 0; x < msg->width; x ++)
	{
	    ULONG srcpix = 0;
	    
	    GET_PAL_PIX(s, srcpix, srcfmt, lut);
	    
	    /* We now have a pixel in Native32 format. Put it back */
	    PUT_TRUE_PIX(d, srcpix, dstfmt);
	    
	
	}
	((UBYTE *)src) += msg->srcMod;
	((UBYTE *)dst) += msg->dstMod;
	
    } 
    
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
}

/****************************************************************************************/

static VOID pal_to_pal(OOP_Class *cl, OOP_Object *o,
    	    	       struct pHidd_BitMap_ConvertPixels *msg)
{
    HIDDT_PixelFormat *spf, *dpf;
     
    spf = msg->srcPixFmt;
    dpf = msg->dstPixFmt;
     
     
    if (    spf->clut_shift == dpf->clut_shift
         && spf->clut_mask  == dpf->clut_mask )
    {
	/* This one is rather easy, just copy the data */
	
    }
    else
    {
     	/* Convert pixel-by pixel */
     
    }
    
    return;
}

/****************************************************************************************/

static void native32_to_native(OOP_Class *cl, OOP_Object *o,
    	    	    	       struct pHidd_BitMap_ConvertPixels *msg)
{
    INIT_VARS()
    HIDDT_PixelFormat 	*dstfmt = msg->dstPixFmt;    
    LONG    	    	x, y;
    
    D(bug("SRC: Native32, DST: Native, height=%d, width=%d, bytes per pixel: %d, srcmod: %d, dstmod: %d, depth: %d\n"
	, msg->height, msg->width, dstfmt->bytes_per_pixel, msg->srcMod, msg->dstMod, dstfmt->depth));

    for ( y = 0; y < msg->height; y ++)
    {
	APTR d = dst;
	APTR s = src;
	
	for (x = 0; x < msg->width; x ++)
	{
	    
	    switch (dstfmt->bytes_per_pixel)
	    {
		case 4:
		    *((ULONG *)d) ++ = (ULONG)*((HIDDT_Pixel *)s) ++;
		    break;
	    
		case 3:
		{
		    HIDDT_Pixel dstpix;
		
		    dstpix = *((HIDDT_Pixel *)s) ++;
		
		    ((UBYTE *)d)[0] = (UBYTE)((dstpix >> 16) & 0x000000FF);
		    ((UBYTE *)d)[1] = (UBYTE)((dstpix >> 8)  & 0x000000FF);
		    ((UBYTE *)d)[2] = (UBYTE)(dstpix  & 0x000000FF);
	    
		    break;
		}
	    
		case 2:
		    *((UWORD *)d) ++ = (UWORD)(*((HIDDT_Pixel *)s) ++);
		    break;
	    
		case 1:
		    *((UBYTE *)d) ++ = (UBYTE)*((HIDDT_Pixel *)s) ++;
		    break;
			
    	    #if 0
		case 0:
		    if (dstfmt->depth == 1)
		    {
			UBYTE mask;

			mask = XCOORD_TO_MASK(x);
			d = ((UBYTE *)dst) + XCOORD_TO_BYTEIDX(x);
			if (*((HIDDT_Pixel *)s) ++) {
			    *((UBYTE *)d) |= mask;
			} else {
			    *((UBYTE *)d) &= ~mask;
			}
		    }
		    break;
    	    #endif
	    
	    } /* switch() */
				
	} /* for (x) */
	    
	((UBYTE *)src) += msg->srcMod;
	((UBYTE *)dst) += msg->dstMod;

    }
    
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
}

/****************************************************************************************/

static VOID quick_copy(OOP_Class *cl, OOP_Object *o,
    	    	       struct pHidd_BitMap_ConvertPixels *msg)
{
    	/* Just do a simple memcpy() of the pixels */
    INIT_VARS()
    HIDDT_PixelFormat 	*srcfmt = msg->srcPixFmt;    
    ULONG   	    	bpl = msg->width * srcfmt->bytes_per_pixel;
    
    #warning This does not work well for formats with bytes_per_pixel < 1
    
    if (msg->srcMod == bpl && msg->dstMod == bpl)
    {
	memcpy(dst, src, bpl * msg->height);
    }
    else
    {
	ULONG i;
	ULONG copy_width;

	copy_width = msg->width * srcfmt->bytes_per_pixel;
	
	for (i = 0; i < msg->height; i ++)
	{
	    memcpy(dst, src, copy_width);
	    ((UBYTE *)src) += msg->srcMod;
	    ((UBYTE *)dst) += msg->dstMod;
	}
    }
	
    *msg->srcPixels = src;
    *msg->dstBuf    = dst;
	
}

/****************************************************************************************/

#warning Discuss this design decision:

/* Should we pass HIDDT_PixelFormat * or HIDDT_StdPixFmt ?
  The first is more flexible for the user, as he will not only be restricted
  to standard pixek formats. However the user will have to convert
  from HIDDT_StdPixFmt to HIDDT_PixelFormat manually.
  
  In the latter case this conversion will be done inside the method below.
  This means that we can call an internal function directly
  to do the conversion and save two method calls.
*/  

/****************************************************************************************/

VOID bitmap_convertpixels(OOP_Class *cl, OOP_Object *o,
    	    	    	  struct pHidd_BitMap_ConvertPixels *msg)
{
    /* For now we assume truecolor */
    HIDDT_PixelFormat *srcfmt, *dstfmt;
    
    //bug("bitmap_convertpixels()\n");

    srcfmt = msg->srcPixFmt;
    dstfmt = msg->dstPixFmt;
    

/* bug("ConvertPixels: src=%d, dst=%d\n"
	, srcfmt->stdpixfmt, dstfmt->stdpixfmt);
*/    

    /* Check if source and dest are the same format */
    if (srcfmt->stdpixfmt == dstfmt->stdpixfmt)
    {
	quick_copy(cl, o, msg);
	return;
    }
    
    
    if (    srcfmt->stdpixfmt == vHidd_StdPixFmt_Native32
    	 && dstfmt->stdpixfmt == vHidd_StdPixFmt_Native    )
    {
	 
	 native32_to_native(cl, o, msg);
	 return;
    }
    
    switch (HIDD_PF_COLMODEL(srcfmt))
    {
	case vHidd_ColorModel_TrueColor:
	    switch (HIDD_PF_COLMODEL(dstfmt))
	    {
	    	case vHidd_ColorModel_TrueColor:
		     true_to_true(cl, o, msg);
		     break;
		     
		
		case vHidd_ColorModel_Palette:
		case vHidd_ColorModel_StaticPalette:
		     true_to_pal(cl, o, msg);
		     break;
		
	    }
	    break;
	
	case vHidd_ColorModel_Palette:
	case vHidd_ColorModel_StaticPalette:
	    switch (HIDD_PF_COLMODEL(dstfmt))
	    {
	    	case vHidd_ColorModel_TrueColor:
		     pal_to_true(cl, o, msg);
		     break;
		
		case vHidd_ColorModel_Palette:
		case vHidd_ColorModel_StaticPalette:
		     pal_to_pal(cl,o, msg);
		     break;
		
	    }
	    break;
    }
    
    return;
}

/****************************************************************************************/
