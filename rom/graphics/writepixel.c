/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$	 $Log

    Desc: Graphics function WritePixel()
    Lang: english
*/

#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <proto/graphics.h>

/*****************************************************************************

    NAME */

	AROS_LH3(LONG, WritePixel,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , x, D0),
	AROS_LHA(LONG             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 54, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        This function takes layers into account. Some pixel that is
        being read is not found on the display-bitmap but in some
        clipped rectangle (cliprect) in a layer structure.
        There is no support of anything else than bitplanes now.
        (No chunky pixels)
        This function resembles very much the function WritePixel()!!

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct Layer * L = rp -> Layer;
  ULONG pen;
  BYTE Mask, pen_Mask, CLR_Mask;
  LONG count;
  struct BitMap * bm = rp->BitMap;
  ULONG Width, Height;
  ULONG i;
  BYTE * Plane; 

  Width = GetBitMapAttr(bm, BMA_WIDTH);  
  Height = GetBitMapAttr(bm, BMA_HEIGHT);
  /* 
     Is there a layer. If not then it cannot be a regular window!!
  */
  if (NULL == L)
  {  
    /* is this pixel inside the rastport? */
    if (x < 0 || x  > Width || 
        y < 0 || y  > Height)
    {
      /* no, it's not ! I refuse to do anything :-))  */
      return -1;
    }
  }

  if (0 != (Width & 0x07))
    Width = (Width >> 3) + 1;
  else
    Width = (Width >> 3);

  /* does this rastport have a layer? */
  if (NULL != L)
  {
    /* more difficult part here as the layer might be partially 
       hidden.
       The coordinate x,y is relative to the layer.
    */
    struct ClipRect * CR = L -> ClipRect;
    WORD YRel = L->bounds.MinY;
    WORD XRel = L->bounds.MinX;

    /* Is this pixel inside the layer ?? */
    if (x > (L->bounds.MaxX - XRel + 1) ||
        y > (L->bounds.MaxY - YRel + 1)   )
    {
      /* ah, no it is not. So we exit */
      return -1;
    }
    
    /* No one may interrupt me while I'm working with this layer */
    /* But there is a problem: if this is called from a routine that 
       already  locked  the layer is already I am stuck. 
    */
/* !!!
    LockLayer(L);
*/
    /* search the list of ClipRects. If the cliprect where the pixel
       goes into does not have an entry in lobs, we can directly
       draw it to the bitmap, otherwise we have to draw it into the
       bitmap of the cliprect. 
    */
    while (NULL != CR)
    {
      if (x >= (CR->bounds.MinX - XRel) &&
          x <= (CR->bounds.MaxX - XRel) &&
          y >= (CR->bounds.MinY - YRel) &&  
          y <= (CR->bounds.MaxY - YRel)    )
      {
        /* that's our cliprect!! */
        /* if it is not hidden, then we treat it as if we were
           directly drawing to the BitMap  
        */
        LONG Offset;
        if (NULL == CR->lobs)
        {
          i = (y + YRel) * Width + 
             ((x + XRel) >> 3);
          Mask = 1 << (7-((x + XRel) & 0x07));

          /* and let the driver set the pixel to the X-Window also,
             but this Pixel has a relative position!! */
          if (bm->Flags & BMF_AROS_DISPLAYED)
            driver_WritePixel (rp, x+XRel, y+YRel, GfxBase);
        } 
        else
        {
          /* we have to draw into the BitMap of the ClipRect, which
             will be shown once the layer moves... 
           */
          bm = CR -> BitMap;
          Width = GetBitMapAttr(bm, BMA_WIDTH);
          /* Calculate the Width of the bitplane in bytes */
          if (Width & 0x07)
            Width = (Width >> 3) + 1;
          else
            Width = (Width >> 3);
           
          Offset = CR->bounds.MinX & 0x0f;
          
          i = (y - (CR->bounds.MinY - YRel)) * Width + 
             ((x - (CR->bounds.MinX - XRel) + Offset) >> 3);   
                /* Offset: optimization for blitting!! */
          Mask = (1 << ( 7 - ((Offset + x - (CR->bounds.MinX - XRel) ) & 0x07)));
          /* no pixel into the X window */
        }       
        break;
        
      } /* if */      
      /* examine the next cliprect */
      CR = CR->Next;
    } /* while */
       
  } /* if */
  else
  { /* this is probably something like a screen */

    /* if it is an old window... */
    if (bm->Flags & BMF_AROS_OLDWINDOW)
         return driver_WritePixel (rp, x, y, GfxBase);

    i = y * Width + (x >> 3);
    Mask = (1 << (7-(x & 0x07)));

    /* and let the driver set the pixel to the X-Window also */
    if (bm->Flags & BMF_AROS_DISPLAYED)
      driver_WritePixel (rp, x, y, GfxBase);
  }

  /* get the pen for this rastport */
  pen = GetAPen(rp);

  pen_Mask = 1;
  CLR_Mask = ~Mask;

  /* we use brute force and write the pixel to
     all bitplane, setting the bitplanes where the pen is
     '1' and clearing the other ones */
  for (count = 0; count < GetBitMapAttr(bm, BMA_DEPTH); count++)
  {
    Plane = bm->Planes[count];

    /* are we supposed to clear this pixel or set it in this bitplane */
    if (0 != (pen_Mask & pen))
    {
      /* in this bitplane we're setting it */
      Plane[i] |= Mask;          
    }
    else
    {
      /* and here we clear it */
      Plane[i] &= CLR_Mask;
    }
    pen_Mask = pen_Mask << 1;
  } /* for */

  /* if there was a layer I have to unlock it now */
/*!!!
  if (NULL != L) 
    UnlockLayer(L);
*/

  return 0;
  AROS_LIBFUNC_EXIT
} /* WritePixel */
