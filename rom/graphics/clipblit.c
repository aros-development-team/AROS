/*
    (C) 1995 AROS - The Amiga Research OS
    $Id$

    Desc: Copy content of a rastport to another rastport
    Lang: english
*/
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/clip.h>

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
 
  /* overlapping and non-overlapping blits are handled differently. */

  if (NULL != srcRP->Layer)
    LockLayerRom( srcRP->Layer);

  if (srcRP != destRP  && NULL != destRP->Layer)
    LockLayerRom(destRP->Layer);
  
  /* check for overlapping blits */
  if ( srcRP == destRP )
  {
    struct Region * R = NewRegion();
    struct Rectangle Rect;
    struct RegionRectangle * RR;

    /* did I get the region structure? */
    if (NULL == R)
      goto exit;

    /* define the rectangle of the destination */
    Rect.MinX = xDest;
    Rect.MaxX = xDest+xSize;
    Rect.MinY = yDest;
    Rect.MaxY = yDest+ySize;
    /* define the region with this rectangle */
    /* check whether operation succeeds = enough memory available*/
    if (FALSE == OrRectRegion(R,&Rect))
    {
      DisposeRegion(R);
      goto exit;
    }

    /* define the rectangle of the source */
    Rect.MinX = xSrc;
    Rect.MaxX = xSrc+xSize;
    Rect.MinY = ySrc;
    Rect.MaxY = ySrc+ySize;
    /* combine them to check for overlapping areas */
    AndRectRegion(R,&Rect); /* this call cannot fail! */

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
      if (xs * 2 > xSize || 
          ys * 2 > ySize)
      {
        /* 
           In this case I use a special routine to copy the rectangle 
        */
        driver_MoveRaster(srcRP,
                          xDest-xSrc,
                          yDest-ySrc,
                          (xSrc < xDest) ? xSrc : xDest,
                          (ySrc < yDest) ? ySrc : yDest,
                          (xSrc > xDest) ? xSrc : xDest,
                          (ySrc > yDest) ? ySrc : yDest,
                          FALSE,
                          GfxBase);
      }
      else
      {
        /* 
           This case is not as difficult as the overlapping
           part can be copied first and then the other parts can
           be copied. 
        */
        /* first copy the overlapping part to its destination */
        internal_ClipBlit(srcRP,
                          xSrc+RR->bounds.MinX,
                          ySrc+RR->bounds.MinY,
                          srcRP,
                          xDest+RR->bounds.MinX,
                          yDest+RR->bounds.MinY,
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
          internal_ClipBlit(srcRP,
                            xSrc+RR->bounds.MinX,
                            ySrc+RR->bounds.MinY,
                            srcRP,
                            xDest+RR->bounds.MinX,
                            yDest+RR->bounds.MinY,
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

  if (srcRP!=destRP && NULL != destRP->Layer)
    UnlockLayerRom(destRP->Layer);

  if (NULL != srcRP->Layer)
    UnlockLayerRom( srcRP->Layer);
  
  return;
                    
  AROS_LIBFUNC_EXIT
} /* ClipBlit */

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
  ULONG bltSrcX, bltSrcY, bltDstX, bltDstY, bltWidth, bltHeight;
  ULONG SrcOffsetX;
  UBYTE bltMask;
  UBYTE useminterm;
  UBYTE MaskTab[] = {0x00, 0x01, 0x03, 0x07, 0x0F,
                           0x1F, 0x3F, 0x7F, 0xFF};
  LONG i1 = GetBitMapAttr(srcBM , BMA_DEPTH);
  LONG i2 = GetBitMapAttr(destBM, BMA_DEPTH);
  bltMask = MaskTab[i1] & MaskTab[i2]; 

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
          ULONG crX0, crX1, crY0, crY1;
          /* cr?? have to be coordinates related to the rastport */
          crX0 = srcCR->bounds.MinX - srcLayer->bounds.MinX;
          crX1 = srcCR->bounds.MaxX - srcLayer->bounds.MinX;
          crY0 = srcCR->bounds.MinY - srcLayer->bounds.MinY;
          crY1 = srcCR->bounds.MaxY - srcLayer->bounds.MinY;
/*
kprintf("CB: found source cliprect with rastport-rel. coords:\n");
kprintf("X0: %d, Y0: %d, X1: %d, Y1: %d\n",crX0,crY0,crX1,crY1);
kprintf("%d, %d\n",srcCR->bounds.MinX,srcCR->bounds.MaxX);
*/
          /* the only case that must not happen is that
             this ClipRect is outside the destination area.  */
          if (!(crX0 > (xSrc+xSize-1) ||
                crX1 <  xSrc          || 
                crY0 > (ySrc+ySize-1) ||
                crY1 <  ySrc))
          {
	    /*
            kprintf("This cliprect will be used as source!\n");
	    */
            /* this cliprect contains bitmap data that need to be copied */
            /* 
              get the pointer to the bitmap structure and fill out
              the rectangle structure that shows which part we mean to copy    
             */
            if (NULL != srcCR->BitMap)
	    {
	      /*
              kprintf("this cliprect is hidden!\n");
	      */
              if (0 == (srcLayer->Flags & LAYERSUPER))
	      {
                /* no superbitmap */
                SrcOffsetX = srcCR->bounds.MinX & 0x0F;

                if (xSrc >= crX0)
                  bltSrcX = xSrc - crX0 + SrcOffsetX;
                else
                  bltSrcX = SrcOffsetX;
            
                if (ySrc > crY0)
                  bltSrcY   = ySrc - crY0;
                else
                  bltSrcY   = 0;
	        /*
                kprintf("bltSrcX: %d, bltSrcY: %d\n",bltSrcX,bltSrcY);
	        */
                srcBM     = srcCR->BitMap;
	      }
              else
	      {
                /* with superbitmap */
                if (xSrc >= crX0)
                  bltSrcX = xSrc + srcLayer->Scroll_X;
                else
                  bltSrcX = crX0 + srcLayer->Scroll_X;
            
                if (ySrc >= crY0)
                  bltSrcY   = ySrc + srcLayer->Scroll_Y;
                else
                  bltSrcY   = crY0 + srcLayer->Scroll_Y;
	        /*
                kprintf("bltSrcX: %d, bltSrcY: %d\n",bltSrcX,bltSrcY);
	        */
                srcBM     = srcCR->BitMap;
	      }

            }
            else
	    {
              /* this part of the layer is not hidden. */
              srcBM   = srcRP->BitMap;
              bltSrcX = xSrc + srcCR->bounds.MinX;
              bltSrcY = ySrc + srcCR->bounds.MinY;
	      /*
              kprintf("this cliprect is not hidden. I use the Rastport's bitmap\n");
              kprintf("bltSrcX: %d, bltSrcY: %d\n",bltSrcX,bltSrcY);
	      */
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
  	    /*
            kprintf("destRect: MinX: %d, MinY: %d,MaxX: %d, MaxY: %d\n",
                 destRect.MinX,destRect.MinY,destRect.MaxX,destRect.MaxY);
	    */
    
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
    } /* if() */
    else /* no layer in the source rastport */
    {
      srcBM = srcRP->BitMap;
      destRect.MinX = xDest;
      destRect.MinY = yDest;
      destRect.MaxX = xDest+xSize-1;
      destRect.MaxY = yDest+ySize-1;
      
      useminterm = minterm;
      /*
      kprintf("The source rastport has no layer!\n");
          kprintf("destRect: MinX: %d, MinY: %d,MaxX: %d, MaxY: %d\n",
               destRect.MinX,destRect.MinY,destRect.MaxX,destRect.MaxY);
      */ 
    }

    destCR = destLayer -> ClipRect;

    /* Does the destination have layers */
    if (NULL != destLayer)
    {
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
            ULONG bltSrcX_tmp = bltSrcX;
            ULONG bltSrcY_tmp = bltSrcY;        
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
                DestOffsetX = destCR->bounds.MinX & 0x0F;
                DestOffsetY = 0;
	      }
              else
	      {
                /* with superbitmap */
                destBM      = destLayer->SuperBitMap;
                DestOffsetX = destCR->bounds.MinX - destLayer->bounds.MinX +
                                                    destLayer->Scroll_X;
                DestOffsetY = destCR->bounds.MinY - destLayer->bounds.MinY +
                                                    destLayer->Scroll_Y;;
	      }
            }
            else  
            {
              destBM      = destRP->BitMap;
              
              DestOffsetX = destCR->bounds.MinX;
              DestOffsetY = destCR->bounds.MinY ;           
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
	  /*
          kprintf("bltSrcX:  %d, bltSrcY  : %d\n",bltSrcX_tmp,bltSrcY_tmp);
          kprintf("bltDstX:  %d, bltDstY  : %d\n",bltDstX,bltDstY);
          kprintf("bltWidth: %d, bltHeight: %d\n",bltWidth,bltHeight);
          kprintf("bltMask : %d\n",bltMask);
          kprintf("srcBM   : %x\n",srcBM);
	  */
	  
	      
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
      /*
      kprintf("Destination does not have a layer!\n");
      */
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
