/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <hidd/graphics.h>
#include "graphics_intern.h"
#include <aros/machine.h>

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
	    *((ULONG *)start)++ = fill32;
	    w -= 4;
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
	    *((UWORD *)start)++ = (UWORD)fill32;
	}
	while(w >= 2)
	{
	    *((ULONG *)start)++ = fill32;
	    w -= 2;
	}
	while(w--)
	{
	    *((UWORD *)start)++ = (UWORD)fill32;
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
	    *((ULONG *)start)++ = fill32;
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
	    *((ULONG *)start)++ = ~bg32;
	    w -= 4;
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
