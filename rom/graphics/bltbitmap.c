/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy a rectangle in a bitmap to another place or another bitmap.
    Lang: english
*/
#include <aros/debug.h>
#include <string.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "objcache.h"

static void copyonepixel (PLANEPTR src, ULONG xsrc, PLANEPTR dest,
	ULONG xdest, ULONG minterm);

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH11(LONG, BltBitMap,

/*  SYNOPSIS */
	AROS_LHA(struct BitMap *, srcBitMap, A0),
	AROS_LHA(LONG           , xSrc, D0),
	AROS_LHA(LONG           , ySrc, D1),
	AROS_LHA(struct BitMap *, destBitMap, A1),
	AROS_LHA(LONG           , xDest, D2),
	AROS_LHA(LONG           , yDest, D3),
	AROS_LHA(LONG           , xSize, D4),
	AROS_LHA(LONG           , ySize, D5),
	AROS_LHA(ULONG          , minterm, D6),
	AROS_LHA(ULONG          , mask, D7),
	AROS_LHA(PLANEPTR       , tempA, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 5, Graphics)

/*  FUNCTION
	Moves a part of a bitmap around or into another bitmaps.

    INPUTS
	srcBitMap - Copy from this bitmap.
	xSrc, ySrc - This is the upper left corner of the area to copy.
	destBitMap - Copy to this bitmap. May be the same as srcBitMap.
	xDest, yDest - Upper left corner where to place the copy
	xSize, ySize - The size of the area to copy
	minterm - How to copy. Most useful values are 0x00C0 for a vanilla
		copy, 0x0030 to invert the source and then copy or 0x0050
		to ignore the source and just invert the destination. If
		you want to calculate other values, then you must know that
		channel A is set, if you are inside the rectangle, channel
		B is the source and channel C is the destination of the
		rectangle.

		Bit	ABC
		 0	000
		 1	001
		 2	010
		 3	011
		 4	100
		 5	101
		 6	110
		 7	111

		So 0x00C0 means: D is set if one is inside the rectangle
		(A is set) and B (the source) is set and cleared otherwise.

		To fill the rectangle, you would want to set D when A is
		set, so the value is 0x00F0.

	mask - Which planes should be copied. Typically, you should set
		this to ~0L.
	tempA - If the copy overlaps exactly to the left or right (i.e. the
		scan line addresses overlap), and tempA is non-zero, it
		points to enough chip accessible memory to hold a line of a
		source for the blit (ie. CHIP RAM). BltBitMap will allocate
		(and free) the needed TempA if none is provided and one is
		needed.  Blit overlap is determined from the relation of
		the first non-masked planes in the source and destination
		bit maps.

    RESULT
	The number of planes actually involved in the blit.

    NOTES
	If a special hardware is available, this function will use it.

	As a special case, plane pointers of destBitMap can contain NULL
	or -1, which will act as if the plane was filled with 0's or 1's,
	respectively.

    EXAMPLE

    BUGS

    SEE ALSO
	ClipBlit(), BltBitMapRastPort()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    LONG planecnt;
    
    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
    
    /* nlorentz: Also check for BMF_AROS_DISPLAYED flag which if set tells
       that this is a HIDD bitmap and should be handled by the driver */

    if ( srcBitMap->pad != 0 || destBitMap->pad != 0 
      || srcBitMap->Flags & BMF_AROS_HIDD || destBitMap->Flags & BMF_AROS_HIDD)
    {
	ULONG wSrc, wDest;
	ULONG x;
	ULONG depth;

	OOP_Object *tmp_gc;

	EnterFunc(bug("driver_BltBitMap()\n"));

	/* bug("BltBitMap(%p, %d, %d, %p, %d, %d, %d, %d, %x)\n"
			,srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, minterm);

	*/
		
	wSrc  = GetBitMapAttr( srcBitMap, BMA_WIDTH);
	wDest = GetBitMapAttr(destBitMap, BMA_WIDTH);

	/* Clip all blits */

	depth = GetBitMapAttr ( srcBitMap, BMA_DEPTH);
	x     = GetBitMapAttr (destBitMap, BMA_DEPTH);
	
	if (x < depth) depth = x;

	/* Clip X and Y */
	if (xSrc < 0)
	{
	    xDest += -xSrc;
	    xSize -= -xSrc;
	    xSrc = 0;
	}

	if (ySrc < 0)
	{
	    yDest += -ySrc;
	    ySize -= -ySrc;
	    ySrc = 0;
	}

	/* Clip width and height for source and dest */
	if (ySrc + ySize > srcBitMap->Rows)
	{
	    ySize = srcBitMap->Rows - ySrc;
	}

	if (yDest + ySize > destBitMap->Rows)
	{
	    ySize = destBitMap->Rows - yDest;
	}

	if (xSrc + xSize >= wSrc)
	{
	    xSize = wSrc - xSrc;
	}

	if (xDest + xSize >= wDest)
	{
    	    xSize = wDest - xDest;
	}

	/* If the size is illegal or we need not copy anything, return */
	if (ySize <= 0 || xSize <= 0 || !mask) return 0;

	tmp_gc = obtain_cache_object(SDD(GfxBase)->gc_cache, GfxBase);
	if (NULL != tmp_gc)
	{
	    OOP_Object *srcbm_obj;

    	    srcbm_obj = OBTAIN_HIDD_BM(srcBitMap);
	    if (NULL != srcbm_obj)
	    {	
		OOP_Object *dstbm_obj;

		dstbm_obj = OBTAIN_HIDD_BM(destBitMap);
		if (NULL != dstbm_obj)
		{

	    	    int_bltbitmap(srcBitMap, srcbm_obj
			    , xSrc, ySrc
			    , destBitMap, dstbm_obj
			    , xDest, yDest
			    , xSize, ySize
			    , minterm
			    , tmp_gc
			    , GfxBase);

	    	    RELEASE_HIDD_BM(dstbm_obj, destBitMap);
		}

		RELEASE_HIDD_BM(srcbm_obj, srcBitMap);
	    }
	    release_cache_object(SDD(GfxBase)->gc_cache, tmp_gc, GfxBase);
	}

    	#warning: dummy return value
	planecnt = 8;
 
    }
    else
    {
	ULONG wSrc, wDest;
	ULONG x, y, plane;
	ULONG depth;
	PLANEPTR src, dest, temp;

	wSrc  = GetBitMapAttr( srcBitMap, BMA_WIDTH);
	wDest = GetBitMapAttr(destBitMap, BMA_WIDTH);
	temp = NULL;

	depth = GetBitMapAttr ( srcBitMap, BMA_DEPTH);
	x     = GetBitMapAttr (destBitMap, BMA_DEPTH);
	if (x < depth)
	    depth = x;

	/* Clip X and Y */
	if (xSrc < 0)
	{
	    xDest += -xSrc;
	    xSize -= -xSrc;
	    xSrc = 0;
	}

	if (ySrc < 0)
	{
	    yDest += -ySrc;
	    ySize -= -ySrc;
	    ySrc = 0;
	}

	/* Clip width and height for source and dest */
	if (ySrc + ySize > srcBitMap->Rows)
	{
	    ySize = srcBitMap->Rows - ySrc;
	}

	if (yDest + ySize > destBitMap->Rows)
	{
	    ySize = destBitMap->Rows - yDest;
        }

	if (xSrc + xSize >= wSrc)
	{
	    xSize = wSrc - xSrc;
        }
        
	if (xDest + xSize >= wDest)
	{
	    xSize = wDest - xDest;
        }

	/* If the size is illegal or we need not copy anything, return */
	if (ySize <= 0 || xSize <= 0 || !mask)
	    return 0;

	planecnt = 0;

	/* For all planes */
	for (plane=0; plane<depth; plane ++)
	{
	    /* Don't do anything if destination planeptr is NULL (means treat like
	       a plane with all zeros) or -1 (means treat like a plane with all ones) */
	       
	    if ((destBitMap->Planes[plane] != NULL) && (destBitMap->Planes[plane] != (PLANEPTR)-1))
	    {
		/* Copy this plane ? */
		if ((1L << plane) & mask)
		{

		    planecnt ++; /* count it */

		    for (y=0; y<ySize; y++)
		    {
			src  =  srcBitMap->Planes[plane] + (y+ySrc) * srcBitMap->BytesPerRow;
			dest = destBitMap->Planes[plane] + (y+yDest)*destBitMap->BytesPerRow;

                	/*
                	   If the source address is less or equal to
                	   the destination address 
                	 */
			if ((src <= dest && src+srcBitMap->BytesPerRow > dest)
			    || (dest <= src && dest+destBitMap->BytesPerRow > src)
			)
	        	{
			    if (!temp)
			    {
				if (tempA)
				    temp = tempA;
				else
				    temp = AllocMem (srcBitMap->BytesPerRow, MEMF_ANY);

				if (!temp)
				    return 0;
			    }

			    memmove (temp, src, srcBitMap->BytesPerRow);

			    for (x=0; x<xSize; x++)
				copyonepixel (temp, x+xSrc, dest, x+xDest, minterm);
			}
			else
			{
			    for (x=0; x<xSize; x++)
				copyonepixel (src, x+xSrc, dest, x+xDest, minterm);
			}

		    } /* for (y=0; y<ySize; y++) */

		} /* if ((1L << plane) & mask) */
	    
	    } /* if dest plane != NULL and dest plane != -1 */
	    
	} /* for (plane=0; plane<depth; plane ++) */

	if (temp && !tempA)
	    FreeMem (temp, srcBitMap->BytesPerRow);
    }

    return planecnt;
    
    AROS_LIBFUNC_EXIT
    
} /* BltBitMap */

/****************************************************************************************/

static void copyonepixel (PLANEPTR src, ULONG xsrc, PLANEPTR dest, ULONG xdest,
	ULONG minterm)
{
    ULONG sByte, sSet;
    ULONG dByte, dSet;
    UBYTE sBit;
    UBYTE dBit;
    BOOL set;

    if (src == NULL)
    {
        sSet = FALSE;
    } else if (src == (PLANEPTR)-1)
    {
        sSet = TRUE;
    } else {
	sByte = xsrc >> 3;
	sBit = 1L << (7 - (xsrc & 0x07));
	sSet = (src[sByte] & sBit) != 0;
    }
    
    /* dest PLANEPTR here will never be NULL or -1 */
    dByte = xdest >> 3;
    dBit = 1L << (7 - (xdest & 0x07));
    dSet = (dest[dByte] & dBit) != 0;

    set = 0;

    if (minterm & 0x0010)
    {
	if (!sSet && !dSet)
	    set = 1;
    }
    if (minterm & 0x0020)
    {
	if (!sSet && dSet)
	    set = 1;
    }
    if (minterm & 0x0040)
    {
	if (sSet && !dSet)
	    set = 1;
    }
    if (minterm & 0x0080)
    {
	if (sSet && dSet)
	    set = 1;
    }

    if (set)
	dest[dByte] |= dBit;
    else
	dest[dByte] &= ~dBit;
}

/****************************************************************************************/
