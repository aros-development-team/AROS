/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy content of a rastport to another rastport
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/layers.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/clip.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

#define LayersBase (struct LayersBase *)(GfxBase->gb_LayersBase)

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH9(void, ClipBlit,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, srcRP , A0),
	AROS_LHA(LONG             , xSrc  , D0),
	AROS_LHA(LONG             , ySrc  , D1),
	AROS_LHA(struct RastPort *, destRP, A1),
	AROS_LHA(LONG             , xDest , D2),
	AROS_LHA(LONG             , yDest , D3),
	AROS_LHA(LONG             , xSize , D4),
	AROS_LHA(LONG             , ySize , D5),
	AROS_LHA(UBYTE            , minterm, D6),

/*  LOCATION */
	struct GfxBase *, GfxBase, 92, Graphics)

/*  FUNCTION
	Copies the contents of one rastport to another rastport.
        Takes care of layered and non-layered source and destination
        rastports. 
        If you have a window you should always use this function instead
        of BltBitMap().

    INPUTS
	srcRP        - Copy from this RastPort.
	xSrc, ySrc   - This is the upper left corner of the area to copy.
	destRP       - Copy to this RastPort.
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



    RESULT
        None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	BltBitMapRastPort()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL li_locked = FALSE;


    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

    if (!OBTAIN_DRIVERDATA(srcRP, GfxBase))
    	return;

    /* overlapping and non-overlapping blits are handled differently. */

    if (srcRP->Layer &&
	destRP->Layer &&
	(srcRP->Layer != destRP->Layer) &&
	(srcRP->Layer->LayerInfo == destRP->Layer->LayerInfo))
    {
    	/* If two layers which belong to the same LayerInfo (~screen)
	   have to be locked, lock first LayerInfo, otherwise there
	   is a potential deadlock problem */
    	LockLayerInfo(srcRP->Layer->LayerInfo);
        li_locked = TRUE;
    }

    if (srcRP->Layer) LockLayerRom( srcRP->Layer);
    if (destRP->Layer && (destRP->Layer != srcRP->Layer)) LockLayerRom(destRP->Layer);

    /* Once the layers are locked there's no more need to hold the layerinfo lock */

    if (li_locked) UnlockLayerInfo(srcRP->Layer->LayerInfo);

    /* check for overlapping blits */
    if ( srcRP == destRP )
    {
	struct Region * R;
	struct Rectangle Rect;
	struct RegionRectangle * RR;

    	if (!(R = NewRectRegion(xDest, yDest, xDest + xSize - 1, yDest + ySize - 1)))
	{
	    goto exit;
	}

	/* define the rectangle of the source */
	Rect.MinX = xSrc;
	Rect.MaxX = xSrc+xSize-1;
	Rect.MinY = ySrc;
	Rect.MaxY = ySrc+ySize-1;
	/* combine them to check for overlapping areas */
	AndRectRegion(R, &Rect); /* this call cannot fail! */

	RR = R->RegionRectangle;

	/* check whether they overlap */
	if (NULL != RR)
	{
	    int xs, ys;
	    /*
               It's overlapping; depending on how bad it is overlapping I
               will have to split this up into several calls to the
               internal ClipBlit routine
	    */

	    xs = xDest-xSrc;
	    ys = yDest-ySrc;
	    if (xs < 0) xs = -xs;
	    if (ys < 0) ys = -ys;

	    /*
              if the destination area is overlapping more than half of the
              width or height of the source area, then it is the more
              difficult case
	    */

	    if (xs * 2 < xSize ||
        	ys * 2 < ySize)
	    {
        	/*
        	   In this case I use a special routine to copy the rectangle
        	*/

        	MoveRaster(srcRP,
                           xSrc - xDest,
                           ySrc - yDest,
                           (xSrc < xDest) ? xSrc : xDest,
                           (ySrc < yDest) ? ySrc : yDest,
                           (xSrc > xDest) ? xSrc + xSize - 1 : xDest + xSize - 1,
                           (ySrc > yDest) ? ySrc + ySize - 1 : yDest + ySize - 1,
                           FALSE,
                           GfxBase);
	    }
	    else
	    {
	    	LONG dx, dy;
		
        	/*
        	   This case is not as difficult as the overlapping
        	   part can be copied first and then the other parts can
        	   be copied.
        	*/
        	/* first copy the overlapping part to its destination */
		
		dx = R->bounds.MinX + RR->bounds.MinX - xSrc;
		dy = R->bounds.MinY + RR->bounds.MinY - ySrc;
		
        	internal_ClipBlit(srcRP,
                        	  xSrc + dx,
                        	  ySrc + dy,
                        	  srcRP,
                        	  xDest + dx,
                        	  yDest + dy,
                        	  RR->bounds.MaxX-RR->bounds.MinX+1,
                        	  RR->bounds.MaxY-RR->bounds.MinY+1,
                        	  minterm,
                        	  GfxBase);

        	/* and now I invert the Region with the source rectangle */
        	if (!XorRectRegion(R, &Rect))
        	{
        	    /* too bad! no more memory */
        	    DisposeRegion(R);
        	    goto exit;
        	}
        	RR = R->RegionRectangle;

        	while (NULL != RR)
        	{
		    dx = R->bounds.MinX + RR->bounds.MinX - xSrc;
		    dy = R->bounds.MinY + RR->bounds.MinY - ySrc;

        	    internal_ClipBlit(srcRP,
                        	      xSrc + dx,
                        	      ySrc + dy,
                        	      srcRP,
                        	      xDest + dx,
                        	      yDest + dy,
                        	      RR->bounds.MaxX-RR->bounds.MinX+1,
                        	      RR->bounds.MaxY-RR->bounds.MinY+1,
                        	      minterm,
                        	      GfxBase);
        	    RR = RR->Next;

        	} /* while */

	    }

	} /* if (NULL != RR)*/
	else
	{
	  /* they don't overlap */
	  internal_ClipBlit(srcRP,
                            xSrc,
                            ySrc,
                            srcRP,
                            xDest,
                            yDest,
                            xSize,
                            ySize,
                            minterm,
                            GfxBase);
	}

	DisposeRegion(R);

    } /* if (destRP == srcRP) */
    else
    {

	/* here: process the case when the source and destination rastports
        	 are different */

	internal_ClipBlit(srcRP,
                	  xSrc,
                	  ySrc,
                	  destRP,
                	  xDest,
                	  yDest,
                	  xSize,
                	  ySize,
                	  minterm,
                	  GfxBase);
    }

    /* the way out, even in failure */
exit:

    if (destRP->Layer && (destRP->Layer != srcRP->Layer)) UnlockLayerRom(destRP->Layer);

    if (srcRP->Layer)  UnlockLayerRom( srcRP->Layer);

    RELEASE_DRIVERDATA(srcRP, GfxBase);

    return;

    AROS_LIBFUNC_EXIT

} /* ClipBlit */

struct clipblit_render_data
{
    struct render_special_info   rsi;
    ULONG   	    	         minterm;
    struct RastPort 	    	*destRP;
    LONG    	    	    	 xDest;
    LONG    	    	    	 yDest;
};

static ULONG clipblit_render(APTR data, LONG srcx, LONG srcy,
    	    	    	     OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	     struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct clipblit_render_data *crd = data;

    BltBitMapRastPort(crd->rsi.curbm, rect->MinX, rect->MinY,
    	    	      crd->destRP, crd->xDest + srcx, crd->yDest + srcy,
		      rect->MaxX - rect->MinX + 1,
		      rect->MaxY - rect->MinY + 1,
		      crd->minterm);

    return 0;		      
}

void internal_ClipBlit(struct RastPort * srcRP,
                       LONG xSrc,
                       LONG ySrc,
                       struct RastPort * destRP,
                       LONG xDest,
                       LONG yDest,
                       LONG xSize,
                       LONG ySize,
                       UBYTE minterm,
                       struct GfxBase * GfxBase)
{
    struct clipblit_render_data data;
    struct Rectangle	    	rect;
    
    data.minterm 	= minterm;
    data.destRP 	= destRP;
    data.xDest  	= xDest;
    data.yDest  	= yDest;
    
    rect.MinX = xSrc;
    rect.MinY = ySrc;
    rect.MaxX = xSrc + xSize - 1;
    rect.MaxY = ySrc + ySize - 1;

    do_render_func(srcRP, NULL, &rect, clipblit_render, &data, FALSE, TRUE, GfxBase);
}
