/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <hidd/graphics.h>
#include "graphics_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

VOID bitmap_fillmemrect8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect8 *msg)
{
    UBYTE *start;
    LONG phase, width, height, w, p;
    ULONG fill32;
    ULONG start_add;
    
    start = msg->dstBuf + msg->minY * msg->dstMod + msg->minX;
    width = msg->maxX - msg->minX + 1;
    height = msg->maxY - msg->minY + 1;
    start_add = msg->dstMod - width;
        
    if ((phase = (IPTR)start & 3L))
    {
    	phase = 4 - phase;
	if (phase > width) phase = width;
	width -= phase;
    }
    
    fill32 = msg->fill;
    fill32 |= (fill32 << 8);
    fill32 |= (fill32 << 16);
    
    while(height--)
    {
    	w = width;
	p = phase;
	
	while(p--)
	{
	    *start++ = (UBYTE)fill32;
	}
	while(w >= 4)
	{
	    *((ULONG *)start) = fill32;
	    w -= 4; start += 4;
	}
	while(w--)
	{
	    *start++ = (UBYTE)fill32;
	}
	start += start_add;
    }
    
};

/****************************************************************************************/

VOID bitmap_fillmemrect16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect16 *msg)
{
    UBYTE *start;
    LONG phase, width, height, w, p;
    ULONG fill32;
    ULONG start_add;
    
    start = msg->dstBuf + msg->minY * msg->dstMod + msg->minX * 2;
    width = msg->maxX - msg->minX + 1;
    height = msg->maxY - msg->minY + 1;
    start_add = msg->dstMod - width * 2;
        
    if ((phase = (IPTR)start & 1L))
    {
    	phase = 2 - phase;
	if (phase > width) phase = width;
	width -= phase;
    }
    
    fill32 = msg->fill;
    fill32 |= (fill32 << 16);
    
    while(height--)
    {
    	w = width;
	p = phase;
	
	while(p--)
	{
	    *((UWORD *)start) = (UWORD)fill32;
	    start += 2; 
	}
	while(w >= 2)
	{
	    *((ULONG *)start) = fill32;
	    w -= 2; start += 4;
	}
	while(w--)
	{
	    *((UWORD *)start) = (UWORD)fill32;
	    start += 2;
	}
	start += start_add;
    }
    
}

/****************************************************************************************/

VOID bitmap_fillmemrect24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect24 *msg)
{
    UBYTE *start;
    LONG width, height, w;
    UBYTE fill1, fill2, fill3;
    ULONG start_add;
    
    start = msg->dstBuf + msg->minY * msg->dstMod + msg->minX * 3;
    width = msg->maxX - msg->minX + 1;
    height = msg->maxY - msg->minY + 1;
    start_add = msg->dstMod - width * 3;

#if AROS_BIG_ENDIAN        
    fill1 = (msg->fill >> 16) & 0xFF;
    fill2 = (msg->fill >> 8) & 0xFF;
    fill3 =  msg->fill & 0xFF;
#else
    fill1 =  msg->fill & 0xFF;
    fill2 = (msg->fill >> 8) & 0xFF;
    fill3 = (msg->fill >> 16) & 0xFF;
#endif

    while(height--)
    {
    	w = width;
	
	while(w--)
	{
	    *start++ = fill1;
	    *start++ = fill2;
	    *start++ = fill3;
	}

	start += start_add;
    }
}

/****************************************************************************************/

VOID bitmap_fillmemrect32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect32 *msg)
{
    UBYTE *start;
    LONG width, height, w;
    ULONG fill32;
    ULONG start_add;
    
    start = msg->dstBuf + msg->minY * msg->dstMod + msg->minX * 4;
    width = msg->maxX - msg->minX + 1;
    height = msg->maxY - msg->minY + 1;
    start_add = msg->dstMod - width * 4;
        
    fill32 = msg->fill;
        
    while(height--)
    {
    	w = width;
	
	while(w--)
	{
	    *((ULONG *)start) = fill32;
	    start += 4;
	}

	start += start_add;
    }
}

/****************************************************************************************/

VOID bitmap_invertmemrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_InvertMemRect *msg)
{
    UBYTE *start;
    LONG phase, width, height, w, p;
    ULONG start_add;
    
    start = msg->dstBuf + msg->minY * msg->dstMod + msg->minX;
    width = msg->maxX - msg->minX + 1;
    height = msg->maxY - msg->minY + 1;
    start_add = msg->dstMod - width;
        
    if ((phase = (IPTR)start & 3L))
    {
    	phase = 4 - phase;
	if (phase > width) phase = width;
	width -= phase;
    }
    
    while(height--)
    {
    	UBYTE bg;
    	ULONG bg32;
	
    	w = width;
	p = phase;
	
	while(p--)
	{
	    bg = *start;
	    *start++ = ~bg;
	}
	while(w >= 4)
	{
	    bg32 = *(ULONG *)start;
	    *((ULONG *)start) = ~bg32;
	    w -= 4; start += 4;
	}
	while(w--)
	{
	    bg = *start;
	    *start++ = ~bg;
	}
	start += start_add;
    }
        
}

/****************************************************************************************/

VOID bitmap_copymembox8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox8 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG phase, width, height, w, p;
    ULONG src_start_add, dst_start_add;
    BOOL descending;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX;
    src_start_add = msg->srcMod - width;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX;
    dst_start_add = msg->dstMod - width;
        
    if ((IPTR)src_start > (IPTR)dst_start)
    {
	if ((phase = (IPTR)src_start & 3L))
	{
    	    phase = 4 - phase;
	    if (phase > width) phase = width;
	    width -= phase;
	}
    	descending = FALSE;
    }
    else
    {
    	src_start += (height - 1) * msg->srcMod + width;
	dst_start += (height - 1) * msg->dstMod + width;
	
	phase = ((IPTR)src_start & 3L);
	if (phase > width) phase = width;
	width -= phase;
	
	descending = TRUE;
    }
 
    /* NOTE: This can write LONGs to odd addresses, which might not work
       on some CPUs (MC68000) */

    if (!descending)
    {
	while(height--)
	{
    	    w = width;
	    p = phase;

	    while(p--)
	    {
		*dst_start++ = *src_start++;
	    }
	    while(w >= 4)
	    {
		*((ULONG *)dst_start) = *((ULONG *)src_start);
		w -= 4; dst_start += 4; src_start += 4;
	    }
	    while(w--)
	    {
		*dst_start++ = *src_start++;
	    }
	    src_start += src_start_add;
	    dst_start += dst_start_add;
	}
    }
    else
    {
	while(height--)
	{
    	    w = width;
	    p = phase;

	    while(p--)
	    {
		*--dst_start = *--src_start;
	    }
	    while(w >= 4)
	    {
	        dst_start -= 4; src_start -= 4;
		*(ULONG *)dst_start = *(ULONG *)src_start;
		w -= 4;
	    }
	    while(w--)
	    {
		*--dst_start = *--src_start;
	    }
	    src_start -= src_start_add;
	    dst_start -= dst_start_add;
	}
     }
    
}

/****************************************************************************************/

VOID bitmap_copymembox16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox16 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG phase, width, height, w, p;
    ULONG src_start_add, dst_start_add;
    BOOL descending;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX * 2;
    src_start_add = msg->srcMod - width * 2;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 2;
    dst_start_add = msg->dstMod - width * 2;
        
    if ((IPTR)src_start > (IPTR)dst_start)
    {
	if ((phase = (IPTR)src_start & 1L))
	{
    	    phase = 2 - phase;
	    if (phase > width) phase = width;
	    width -= phase;
	}
    	descending = FALSE;
    }
    else
    {
    	src_start += (height - 1) * msg->srcMod + width * 2;
	dst_start += (height - 1) * msg->dstMod + width * 2;
	
	phase = ((IPTR)src_start & 1L);
	if (phase > width) phase = width;
	width -= phase;
	
	descending = TRUE;
    }
 
    if (!descending)
    {
	while(height--)
	{
    	    w = width;
	    p = phase;

	    while(p--)
	    {
		*((UWORD *)dst_start) = *((UWORD *)src_start);
		dst_start += 2; src_start += 2;
	    }
	    while(w >= 2)
	    {
		*((ULONG *)dst_start) = *((ULONG *)src_start);
		w -= 2; dst_start += 4; src_start += 4;
	    }
	    while(w--)
	    {
		*((UWORD *)dst_start) = *((UWORD *)src_start);
		dst_start += 2; src_start += 2;
	    }
	    src_start += src_start_add;
	    dst_start += dst_start_add;
	}
    }
    else
    {
	while(height--)
	{
    	    w = width;
	    p = phase;

	    while(p--)
	    {
	        dst_start -= 2; src_start -= 2;
		*(UWORD *)dst_start = *(UWORD *)src_start;
	    }
	    while(w >= 2)
	    {
	        dst_start -= 4; src_start -= 4;
		*(ULONG *)dst_start = *(ULONG *)src_start;
		w -= 2;
	    }
	    while(w--)
	    {
	        dst_start -= 2; src_start -= 2;
		*(UWORD *)dst_start = *(UWORD *)src_start;
	    }
	    src_start -= src_start_add;
	    dst_start -= dst_start_add;
	}
     }
    
}

/****************************************************************************************/

VOID bitmap_copymembox24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox24 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    BOOL descending;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX * 3;
    src_start_add = msg->srcMod - width * 3;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 3;
    dst_start_add = msg->dstMod - width * 3;
        
    if ((IPTR)src_start > (IPTR)dst_start)
    {
    	descending = FALSE;
    }
    else
    {
    	src_start += (height - 1) * msg->srcMod + width * 3;
	dst_start += (height - 1) * msg->dstMod + width * 3;
	
	descending = TRUE;
    }
 
    if (!descending)
    {
	while(height--)
	{
    	    w = width;

	    while(w--)
	    {
		*dst_start++ = *src_start++;
		*dst_start++ = *src_start++;
		*dst_start++ = *src_start++;
	    }

	    src_start += src_start_add;
	    dst_start += dst_start_add;
	}
    }
    else
    {
	while(height--)
	{
    	    w = width;

	    while(w--)
	    {
		*--dst_start = *--src_start;
		*--dst_start = *--src_start;
		*--dst_start = *--src_start;
	    }
	    
	    src_start -= src_start_add;
	    dst_start -= dst_start_add;
	}
     }
    
}

/****************************************************************************************/

VOID bitmap_copymembox32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox32 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    BOOL descending;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX * 4;
    src_start_add = msg->srcMod - width * 4;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 4;
    dst_start_add = msg->dstMod - width * 4;
        
    if ((IPTR)src_start > (IPTR)dst_start)
    {
    	descending = FALSE;
    }
    else
    {
    	src_start += (height - 1) * msg->srcMod + width * 4;
	dst_start += (height - 1) * msg->dstMod + width * 4;
	
	descending = TRUE;
    }
 
    if (!descending)
    {
	while(height--)
	{
    	    w = width;

	    while(w--)
	    {    
		*((ULONG *)dst_start) = *((ULONG *)src_start);
		dst_start += 4; src_start += 4;
	    }

	    src_start += src_start_add;
	    dst_start += dst_start_add;
	}
    }
    else
    {
	while(height--)
	{
    	    w = width;

	    while(w--)
	    {
	        dst_start -= 4; src_start -= 4;
		*(ULONG *)dst_start = *(ULONG *)src_start;
	    }
	    
	    src_start -= src_start_add;
	    dst_start -= dst_start_add;
	}
     }
    
}

/****************************************************************************************/

VOID bitmap_copylutmembox16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox16 *msg)
{
    HIDDT_Pixel *pixlut = msg->pixlut->pixels;
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    if (!pixlut) return;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX;
    src_start_add = msg->srcMod - width;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 2;
    dst_start_add = msg->dstMod - width * 2;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *(UWORD *)dst_start = (UWORD)(pixlut[*src_start++]);
	    dst_start += 2;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_copylutmembox24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox24 *msg)
{
    HIDDT_Pixel *pixlut = msg->pixlut->pixels;
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;

    if (!pixlut) return;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX;
    src_start_add = msg->srcMod - width;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 3;
    dst_start_add = msg->dstMod - width * 3;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    HIDDT_Pixel pix = pixlut[*src_start++];
	    
	#if AROS_BIG_ENDIAN
	    *dst_start++ = (pix >> 16) & 0xFF;
	    *dst_start++ = (pix >> 8) & 0xFF;
	    *dst_start++ =  pix & 0xFF;
	#else
	    *dst_start++ =  pix & 0xFF;
	    *dst_start++ = (pix >> 8) & 0xFF;
	    *dst_start++ = (pix >> 16) & 0xFF;
	#endif
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_copylutmembox32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox32 *msg)
{
    HIDDT_Pixel *pixlut = msg->pixlut->pixels;
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;

    if (!pixlut) return;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX;
    src_start_add = msg->srcMod - width;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 4;
    dst_start_add = msg->dstMod - width * 4;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *((ULONG *)dst_start) = (ULONG)(pixlut[*src_start++]);
	    dst_start += 4;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_putmem32image8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image8 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src;
    src_start_add = msg->srcMod - width * 4;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX;
    dst_start_add = msg->dstMod - width;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *dst_start++ = (UBYTE)(*(ULONG *)src_start);
	    src_start += 4;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_putmem32image16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image16 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src;
    src_start_add = msg->srcMod - width * 4;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 2;
    dst_start_add = msg->dstMod - width * 2;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *(UWORD *)dst_start = (UWORD)(*(ULONG *)src_start);
	    dst_start += 2; src_start += 4;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_putmem32image24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image24 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src;
    src_start_add = msg->srcMod - width * 4;

    dst_start = msg->dst + msg->dstY * msg->dstMod + msg->dstX * 3;
    dst_start_add = msg->dstMod - width * 3;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    ULONG pix = *(ULONG *)src_start;

	    src_start += 4;
	    
	#if AROS_BIG_ENDIAN
	    *dst_start++ = (pix >> 16) & 0xFF;
	    *dst_start++ = (pix >> 8) & 0xFF;
	    *dst_start++ =  pix & 0xFF;
	#else
	    *dst_start++ =  pix & 0xFF;
	    *dst_start++ = (pix >> 8) & 0xFF;
	    *dst_start++ = (pix >> 16) & 0xFF;
	#endif

	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_getmem32image8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image8 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX;
    src_start_add = msg->srcMod - width;

    dst_start = msg->dst;
    dst_start_add = msg->dstMod - width * 4;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *(ULONG *)dst_start = (ULONG)(*src_start++);
	    dst_start += 4; 
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_getmem32image16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image16 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX * 2;
    src_start_add = msg->srcMod - width * 2;

    dst_start = msg->dst;
    dst_start_add = msg->dstMod - width * 4;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    *(ULONG *)dst_start = (ULONG)(*(UWORD *)src_start);
	    dst_start += 4; src_start += 2;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_getmem32image24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image24 *msg)
{
    UBYTE *src_start, *dst_start;
    LONG width, height, w;
    ULONG src_start_add, dst_start_add;
    
    width = msg->width;
    height = msg->height;

    src_start = msg->src + msg->srcY * msg->srcMod + msg->srcX * 3;
    src_start_add = msg->srcMod - width * 3;

    dst_start = msg->dst;
    dst_start_add = msg->dstMod - width * 4;
        
    while(height--)
    {
    	w = width;

	while(w--)
	{
	    UBYTE pix1 = *src_start++;
	    UBYTE pix2 = *src_start++;
	    UBYTE pix3 = *src_start++;
	    
	#if AROS_BIG_ENDIAN
	    *(ULONG *)dst_start = (pix1 << 16) | (pix2 << 8) | pix3;
	#else
	    *(ULONG *)dst_start = (pix3 << 16) | (pix2 << 8) | pix1;
	#endif
	
	    dst_start += 4;
	}
	src_start += src_start_add;
	dst_start += dst_start_add;
    }
}

/****************************************************************************************/

VOID bitmap_putmemtemplate8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMemTemplate8 *msg)
{
    WORD    	    	     x, y;
    UBYTE   	    	    *bitarray, *buf;
    UWORD    	    	     bitmask;
    OOP_Object	    	    *gc = msg->gc;
    ULONG	     	     fg = GC_FG(gc);
    ULONG	    	     bg = GC_BG(gc);
    WORD    	    	     type = 0;
    
    if (msg->width <= 0 || msg->height <= 0)
	return;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;
    }
    
    if (msg->inverttemplate) type++;
    
    bitarray = msg->template + ((msg->srcx / 16) * 2);
    bitmask = 0x8000 >> (msg->srcx & 0xF);
    
    buf = msg->dst + msg->y * msg->dstMod + msg->x;
    
    for(y = 0; y < msg->height; y++)
    {
	ULONG  mask = bitmask;
	UWORD *array = (UWORD *)bitarray;
	UWORD  bitword = AROS_BE2WORD(*array);
    	UBYTE *xbuf = buf;

	switch(type)
	{
	    case 0:	/* JAM1 */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 1:	/* JAM1 | INVERSVID */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

    	    case 2: /* COMPLEMENT */
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 3: /* COMPLEMENT | INVERSVID*/
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		 } /* for(x = 0; x < msg->width; x++) */
		break;

	    case 4:	/* JAM2 */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? fg : bg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 5:	/* JAM2 | INVERSVID */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? bg : fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	} /* switch(type) */

	buf += msg->dstMod;			     
	bitarray += msg->modulo;

    } /* for(y = 0; y < msg->height; y++) */ 
}

VOID bitmap_putmemtemplate16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMemTemplate16 *msg)
{
    WORD    	    	     x, y;
    UBYTE   	    	    *bitarray, *buf;
    UWORD   	    	     bitmask;
    OOP_Object	    	    *gc = msg->gc;
    ULONG	     	     fg = GC_FG(gc);
    ULONG	    	     bg = GC_BG(gc);
    WORD    	    	     type = 0;
    
    if (msg->width <= 0 || msg->height <= 0)
	return;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;
    }
    
    if (msg->inverttemplate) type++;
    
    bitarray = msg->template + ((msg->srcx / 16) * 2);
    bitmask = 0x8000 >> (msg->srcx & 0xF);
    
    buf = msg->dst + msg->y * msg->dstMod + msg->x * 2;

    for(y = 0; y < msg->height; y++)
    {
	ULONG  mask = bitmask;
	UWORD *array = (UWORD *)bitarray;
	UWORD  bitword = AROS_BE2WORD(*array);
    	UWORD *xbuf = (UWORD *)buf;

	switch(type)
	{
	    case 0:	/* JAM1 */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 1:	/* JAM1 | INVERSVID */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

    	    case 2: /* COMPLEMENT */
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 3: /* COMPLEMENT | INVERSVID*/
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		 } /* for(x = 0; x < msg->width; x++) */
		break;

	    case 4:	/* JAM2 */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? fg : bg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 5:	/* JAM2 | INVERSVID */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? bg : fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	} /* switch(type) */

	buf += msg->dstMod;			     
	bitarray += msg->modulo;

    } /* for(y = 0; y < msg->height; y++) */
    
}

VOID bitmap_putmemtemplate24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMemTemplate24 *msg)
{
    WORD    	    	     x, y;
    UBYTE   	    	    *bitarray, *buf;
    UWORD   	    	     bitmask;
    OOP_Object	    	    *gc = msg->gc;
    ULONG	     	     fg = GC_FG(gc);
    UBYTE   	    	     fg1, fg2, fg3;
    ULONG	    	     bg = GC_BG(gc);
    UBYTE   	    	     bg1, bg2, bg3;
    WORD    	    	     type = 0;
    
    if (msg->width <= 0 || msg->height <= 0)
	return;

#if AROS_BIG_ENDIAN        
    fg1 = (fg >> 16) & 0xFF;
    fg2 = (fg >> 8) & 0xFF;
    fg3 =  fg & 0xFF;

    bg1 = (bg >> 16) & 0xFF;
    bg2 = (bg >> 8) & 0xFF;
    bg3 =  bg & 0xFF;
#else
    fg1 =  fg & 0xFF;
    fg2 = (fg >> 8) & 0xFF;
    fg3 = (fg >> 16) & 0xFF;

    bg1 =  bg & 0xFF;
    bg2 = (bg >> 8) & 0xFF;
    bg3 = (bg >> 16) & 0xFF;
#endif

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;
    }
    
    if (msg->inverttemplate) type++;
    
    bitarray = msg->template + ((msg->srcx / 16) * 2);
    bitmask = 0x8000 >> (msg->srcx & 0xF);
    
    buf = msg->dst + msg->y * msg->dstMod + msg->x * 3;
    
    for(y = 0; y < msg->height; y++)
    {
	ULONG  mask = bitmask;
	UWORD *array = (UWORD *)bitarray;
	UWORD  bitword = AROS_BE2WORD(*array);
    	UBYTE *xbuf = buf;

	switch(type)
	{
	    case 0:	/* JAM1 */	    
		for(x = 0; x < msg->width; x++, xbuf += 3)
		{
    	    	    if (bitword & mask)
		    {
			xbuf[0] = fg1;
			xbuf[1] = fg2;
			xbuf[2] = fg3;
		    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 1:	/* JAM1 | INVERSVID */	    
		for(x = 0; x < msg->width; x++, xbuf += 3)
		{
    	    	    if (!(bitword & mask))
		    {
			xbuf[0] = fg1;
			xbuf[1] = fg2;
			xbuf[2] = fg3;
    	    	    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

    	    case 2: /* COMPLEMENT */
		for(x = 0; x < msg->width; x++, xbuf += 3)
		{
    	    	    if (bitword & mask)
		    {
			xbuf[0] = ~xbuf[0];
			xbuf[1] = ~xbuf[1];
			xbuf[2] = ~xbuf[2];
    	    	    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 3: /* COMPLEMENT | INVERSVID*/
		for(x = 0; x < msg->width; x++, xbuf += 3)
		{
    	    	    if (!(bitword & mask))
		    {
			xbuf[0] = ~xbuf[0];
			xbuf[1] = ~xbuf[1];
			xbuf[2] = ~xbuf[2];
		    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		 } /* for(x = 0; x < msg->width; x++) */
		break;

	    case 4:	/* JAM2 */	    
		for(x = 0; x < msg->width; x++)
		{
		    if (bitword & mask)
		    {
			*xbuf++ = fg1;
			*xbuf++ = fg2;
			*xbuf++ = fg3;
		    }
		    else
		    {
			*xbuf++ = bg1;
			*xbuf++ = bg2;
			*xbuf++ = bg3;
		    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 5:	/* JAM2 | INVERSVID */	    
		for(x = 0; x < msg->width; x++)
		{
		    if (bitword & mask)
		    {
			*xbuf++ = bg1;
			*xbuf++ = bg2;
			*xbuf++ = bg3;
		    }
		    else
		    {
			*xbuf++ = fg1;
			*xbuf++ = fg2;
			*xbuf++ = fg3;
		    }

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	} /* switch(type) */

	buf += msg->dstMod;			     
	bitarray += msg->modulo;

    } /* for(y = 0; y < msg->height; y++) */
    
}

VOID bitmap_putmemtemplate32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMemTemplate32 *msg)
{
    WORD    	    	     x, y;
    UBYTE   	    	    *bitarray, *buf;
    UWORD   	    	     bitmask;
    OOP_Object	    	    *gc = msg->gc;
    ULONG	     	     fg = GC_FG(gc);
    ULONG	    	     bg = GC_BG(gc);
    WORD    	    	     type = 0;
    
    if (msg->width <= 0 || msg->height <= 0)
	return;

    if (GC_COLEXP(gc) == vHidd_GC_ColExp_Transparent)
    {
    	type = 0;
    }
    else if (GC_DRMD(gc) == vHidd_GC_DrawMode_Invert)
    {
    	type = 2;
    }
    else
    {
    	type = 4;
    }
    
    if (msg->inverttemplate) type++;
    
    bitarray = msg->template + ((msg->srcx / 16) * 2);
    bitmask = 0x8000 >> (msg->srcx & 0xF);
    
    buf = msg->dst + msg->y * msg->dstMod + msg->x * 4;
    
    for(y = 0; y < msg->height; y++)
    {
	ULONG  mask = bitmask;
	UWORD *array = (UWORD *)bitarray;
	UWORD  bitword = AROS_BE2WORD(*array);
	ULONG *xbuf = (ULONG *)buf;

	switch(type)
	{
	    case 0:	/* JAM1 */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 1:	/* JAM1 | INVERSVID */	    
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

    	    case 2: /* COMPLEMENT */
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (bitword & mask) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 3: /* COMPLEMENT | INVERSVID*/
		for(x = 0; x < msg->width; x++, xbuf++)
		{
    	    	    if (!(bitword & mask)) *xbuf = ~(*xbuf);

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		 } /* for(x = 0; x < msg->width; x++) */
		break;

	    case 4:	/* JAM2 */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? fg : bg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }

		} /* for(x = 0; x < msg->width; x++) */
		break;

	    case 5:	/* JAM2 | INVERSVID */	    
		for(x = 0; x < msg->width; x++)
		{
     	    	    *xbuf++ = (bitword & mask) ? bg : fg;

		    mask >>= 1;
		    if (!mask)
		    {
			mask = 0x8000;
			array++;
			bitword = AROS_BE2WORD(*array);
		    }
		} /* for(x = 0; x < msg->width; x++) */
		break;

	} /* switch(type) */

	buf += msg->dstMod;			     
	bitarray += msg->modulo;

    } /* for(y = 0; y < msg->height; y++) */
    
}

