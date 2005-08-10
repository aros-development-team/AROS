/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy content of a rastport to another rastport
    Lang: english
*/
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/layers.h>
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include "gfxfuncsupport.h"

#include <aros/debug.h>

#define NEW_INTERNAL_CLIPBLIT 1

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    BOOL li_locked = FALSE;


    FIX_GFXCOORD(xSrc);
    FIX_GFXCOORD(ySrc);
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

#if NEW_INTERNAL_CLIPBLIT
    if (!OBTAIN_DRIVERDATA(srcRP, GfxBase))
    	return;
#endif

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
        	if (FALSE == XorRectRegion(R, &Rect))
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

#if NEW_INTERNAL_CLIPBLIT
    RELEASE_DRIVERDATA(srcRP, GfxBase);
#endif

    return;

    AROS_LIBFUNC_EXIT

} /* ClipBlit */

#if NEW_INTERNAL_CLIPBLIT

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
			     LONG x1, LONG y1, LONG x2, LONG y2, struct GfxBase *GfxBase)
{
    struct clipblit_render_data *crd = data;

    BltBitMapRastPort(crd->rsi.curbm, x1, y1,
    	    	      crd->destRP, crd->xDest + srcx, crd->yDest + srcy,
		      x2 - x1 + 1,
		      y2 - y1 + 1,
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

    do_render_func(srcRP, NULL, &rect, clipblit_render, &data, TRUE, GfxBase);
}

#else

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
  struct ClipRect * srcCR  = NULL;
  struct ClipRect * destCR = NULL;
  struct BitMap   * srcBM  =  srcRP->BitMap;
  struct BitMap   * destBM =  destRP->BitMap;
  struct Layer    * srcLayer =  srcRP->Layer;
  struct Layer    * destLayer= destRP->Layer;
  struct Rectangle destRect;
  LONG bltSrcX = 0, bltSrcY = 0, bltDstX, bltDstY, bltWidth, bltHeight;
  ULONG SrcOffsetX;
  ULONG bltMask;
  UBYTE useminterm = 0;
  
  
/* nlorentz: The below did not work because bitmaps may be more than 8 bit.
   Just setting it to 0xFFFFFFF should work fine for copying all planes

  UBYTE MaskTab[] = {0x00, 0x01, 0x03, 0x07, 0x0F,
                           0x1F, 0x3F, 0x7F, 0xFF};
  LONG i1 = GetBitMapAttr(srcBM , BMA_DEPTH);
  LONG i2 = GetBitMapAttr(destBM, BMA_DEPTH);
  bltMask = MaskTab[i1] & MaskTab[i2]; 
*/			   
  bltMask = 0xFFFFFFFF;
  
  if (NULL != srcLayer)
    srcCR  = srcLayer->ClipRect;

  /* process all source and destination bitmaps */
  while (TRUE)
  {
    /* 
       First I look for a source bitmap that fits into the given
       source area, then I will look for a destination bitmap to
       blit it into. 
     */
   
    /* Does the source have layers? */ 
    if (NULL != srcRP->Layer)
    {
      while (NULL != srcCR)
      {
          LONG crX0, crX1, crY0, crY1;
          /* cr?? have to be coordinates related to the rastport */
          crX0 = srcCR->bounds.MinX - srcLayer->bounds.MinX;
          crX1 = srcCR->bounds.MaxX - srcLayer->bounds.MinX;
          crY0 = srcCR->bounds.MinY - srcLayer->bounds.MinY;
          crY1 = srcCR->bounds.MaxY - srcLayer->bounds.MinY;

          /* the only case that must not happen is that
             this ClipRect is outside the destination area.  */
          if (!(crX0 > (xSrc+xSize-1) ||
                crX1 <  xSrc          || 
                crY0 > (ySrc+ySize-1) ||
                crY1 <  ySrc))
          {
            /* this cliprect contains bitmap data that need to be copied */
            /* 
              get the pointer to the bitmap structure and fill out
              the rectangle structure that shows which part we mean to copy    
             */
            if (NULL != srcCR->BitMap)
	    {
              if (0 == (srcLayer->Flags & LAYERSUPER))
	      {
                /* no superbitmap */
                SrcOffsetX = ALIGN_OFFSET(srcCR->bounds.MinX);

                if (xSrc >= crX0)
                  bltSrcX = xSrc - crX0 + SrcOffsetX;
                else
                  bltSrcX = SrcOffsetX;

                if (ySrc > crY0)
                  bltSrcY   = ySrc - crY0;
                else
                  bltSrcY   = 0;

                srcBM     = srcCR->BitMap;
	      }
              else
	      {
                /* with superbitmap */
                if (xSrc >= crX0)
                  bltSrcX = xSrc - srcLayer->Scroll_X;
                else
                  bltSrcX = crX0 - srcLayer->Scroll_X;
            
                if (ySrc >= crY0)
                  bltSrcY   = ySrc - srcLayer->Scroll_Y;
                else
                  bltSrcY   = crY0 - srcLayer->Scroll_Y;

                srcBM     = srcCR->BitMap;
	      }

            }
            else
	    {
              /* this part of the layer is not hidden. */
              /* The source bitmap is the bitmap of the rastport */
              srcBM   = srcRP->BitMap;
              
              /* xSrc and ySrc are relative to the rastport of the window
                 or layer - here we have to make them absolute to the
                 screen's rastport*/
              
              if (xSrc <= crX0)
                bltSrcX = srcCR->bounds.MinX;
              else  
                bltSrcX = xSrc + srcLayer->bounds.MinX;
                
              if (ySrc <= crY0)
                bltSrcY = srcCR->bounds.MinY;
              else
                bltSrcY = ySrc + srcLayer->bounds.MinY;
	    }

            if (crX0 > xSrc)
              destRect.MinX = crX0 - xSrc + xDest;
            else
              destRect.MinX = xDest;

            if (crX1 < (xSrc+xSize-1))
              destRect.MaxX = crX1 - xSrc + xDest;
            else
              destRect.MaxX = xDest+xSize-1;

            if (crY0 > ySrc)
              destRect.MinY = crY0 - ySrc + yDest;
            else
              destRect.MinY = yDest;

            if (crY1 < (ySrc+ySize-1))
              destRect.MaxY = crY1 - ySrc + yDest;
            else
              destRect.MaxY = yDest+ySize-1;
    
            if ((0 != (srcLayer->Flags & LAYERSIMPLE) &&
                (NULL != srcCR->lobs ||
                0 != (srcCR   ->Flags & CR_NEEDS_NO_CONCEALED_RASTERS))))
              useminterm = 0x0;     /* clear this area in the destination */
            else
              useminterm = minterm;

            break;
	  } /* if () */
	srcCR = srcCR -> Next;
      }
      if (NULL == srcCR)
        return;
    } /* if() */
    else /* no layer in the source rastport */
    {
      srcBM = srcRP->BitMap;
      destRect.MinX = xDest;
      destRect.MinY = yDest;
      destRect.MaxX = xDest+xSize-1;
      destRect.MaxY = yDest+ySize-1;
      
      bltSrcX = xSrc;
      bltSrcY = ySrc;
      
      useminterm = minterm;
    }


    /* Does the destination have layers */
    if (NULL != destLayer)
    {
      destCR = destLayer -> ClipRect;
      /* search for the first/next BitMap that is to be filled  */
      /* destRect contains the area that we want to copy to */
      while (NULL != destCR)
      {
        /* if the layer is a simple layer and the cliprect's flag
           has a certain bit set or the CR is hidden, then do 
           nothing here! */ 
        if (!(0    != (destLayer->Flags & LAYERSIMPLE) && 
             (NULL !=  destCR   ->lobs ||
              0    != (destCR   ->Flags & CR_NEEDS_NO_CONCEALED_RASTERS)))) 
	{
          struct Rectangle destRect2;
          LONG DestOffsetX, DestOffsetY;

          destRect2.MinX = destCR->bounds.MinX - destLayer->bounds.MinX;
          destRect2.MaxX = destCR->bounds.MaxX - destLayer->bounds.MinX;
          destRect2.MinY = destCR->bounds.MinY - destLayer->bounds.MinY;
          destRect2.MaxY = destCR->bounds.MaxY - destLayer->bounds.MinY;


         /* does this ClipRect fit into the destination area? 
             The only case that must not happen is that it lies
             outside of destRect */
          if (!(destRect.MinX  > destRect2.MaxX ||
                destRect.MaxX  < destRect2.MinX ||
                destRect.MinY  > destRect2.MaxY ||
                destRect.MaxY  < destRect2.MinY   )) 
          {
            LONG bltSrcX_tmp = bltSrcX;
            LONG bltSrcY_tmp = bltSrcY;        
            bltWidth  = destRect.MaxX - destRect.MinX + 1;
            bltHeight = destRect.MaxY - destRect.MinY + 1;
            
            /* 
               destCR actually contains the/a part of the rectangle that 
               we have to blit to 
             */
            if (NULL != destCR->BitMap)
            {
              if (0 == (destLayer->Flags & LAYERSUPER))
	      {
                /* no superbitmap */
                destBM      = destCR->BitMap;
                DestOffsetX = ALIGN_OFFSET(destCR->bounds.MinX);
                DestOffsetY = 0;
	      }
              else
	      {
                /* with superbitmap */
                destBM      = destLayer->SuperBitMap;
                DestOffsetX = destCR->bounds.MinX - destLayer->bounds.MinX -
                                                    destLayer->Scroll_X;
                DestOffsetY = destCR->bounds.MinY - destLayer->bounds.MinY -
                                                    destLayer->Scroll_Y;;
	      }
            }
            else  
            {
              destBM      = destRP->BitMap;
              
              DestOffsetX = destCR->bounds.MinX;
              DestOffsetY = destCR->bounds.MinY;           
	    }

            if (destRect.MinX > destRect2.MinX)
 	    {
              bltDstX   = destRect.MinX - destRect2.MinX + DestOffsetX; 
            }
            else
            {
              bltDstX   = DestOffsetX;
              bltWidth -= (destRect2.MinX - destRect.MinX); 
              bltSrcX_tmp  += (destRect2.MinX - destRect.MinX);
            }

            if (destRect.MaxX > destRect2.MaxX)
              bltWidth -= (destRect.MaxX - destRect2.MaxX);           

            if (destRect.MinY > destRect2.MinY) 
            {
              bltDstY    = destRect.MinY - destRect2.MinY + DestOffsetY;
	    }
            else
            {
              bltDstY    = DestOffsetY;
              bltHeight -= (destRect2.MinY - destRect.MinY);
              bltSrcY_tmp   += (destRect2.MinY - destRect.MinY);
            }

            if (destRect.MaxY > destRect2.MaxY)
              bltHeight -= (destRect.MaxY - destRect2.MaxY);
	      
            BltBitMap(srcBM,
                      bltSrcX_tmp,
                      bltSrcY_tmp,
                      destBM,
                      bltDstX,
                      bltDstY,
                      bltWidth,
                      bltHeight,
                      useminterm,
                      bltMask,
                      NULL);
	  } /* if (... ) */
	}
        destCR = destCR -> Next;
      } /* while (NULL != destCR) */
    } /* if (NULL != destRP->Layer) */
    else
    {
      /* the destination does not have a layer. So I copy from srcBM
         the whole rectangle that is given in destRect to the rastport's
         bitmap */
      BltBitMap(srcBM,
                bltSrcX,
                bltSrcY,
                destRP->BitMap,
                destRect.MinX,
                destRect.MinY,
                destRect.MaxX - destRect.MinX + 1,
                destRect.MaxY - destRect.MinY + 1,
                useminterm,
                bltMask,
                NULL);
    }    

    /* if the source rastport doesn't have a layer then I am done here
       as all the blits from the one bitmap have already happened.
     */

    if (NULL != srcCR)
      srcCR = srcCR->Next;
    else
      break;

    if (NULL == srcLayer)
      break;
  } /* while (TRUE) */

  return;
}

#endif /* NEW_INTERNAL_CLIPBLIT */

#undef LayersBase
