/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <proto/graphics.h>

#include "layers_intern.h"
#include "basicfuncs.h"



/* 
 *  From a given list of cliprects make a 1:1 copy for all those
 *  parts that are within a region. Also copy the BitMaps if 
 *  necessary.
 */
 
struct ClipRect * CopyClipRectsInRegion(struct Layer * L,
                                        struct ClipRect * CR,
                                        struct Region * ClipRegion)
{
  struct ClipRect * CR_new = NULL, * _CR;
  BOOL isSmart;

  if (LAYERSMART == (L->Flags & (LAYERSMART | LAYERSUPER)))
    isSmart = TRUE;
  else
    isSmart = FALSE;
  
  /* walk through all ClipRects */
  while (NULL != CR)
  {
    /* 
    ** if this is a simple layer and the cliprect is hidden then I
    ** don't even bother with that cliprect 
    */
    if (!(   (0 != (L ->Flags & LAYERSIMPLE)) && NULL != CR->lobs))
    { 
      struct RegionRectangle * RR = ClipRegion->RegionRectangle;
      struct Rectangle Rect = CR->bounds;
      int area;
      /*
      ** Usually I would have to add the ClipRegion's coordinate
      ** to all its RegionRectangles. To prevent this I subtract it
      ** from the Rect's coordinates. 
      */
      Rect.MinX -= (L->bounds.MinX + ClipRegion->bounds.MinX);
      Rect.MinY -= (L->bounds.MinY + ClipRegion->bounds.MinY);
      Rect.MaxX -= (L->bounds.MinX + ClipRegion->bounds.MinX);
      Rect.MaxY -= (L->bounds.MinY + ClipRegion->bounds.MinY);
      area = (Rect.MaxX - Rect.MinX + 1) *
             (Rect.MaxY - Rect.MinY + 1);
    
      /* compare it with all RegionRectangles */
    
      while (NULL != RR && area > 0)
      {
        if (!(Rect.MinX > RR->bounds.MaxX ||
              Rect.MinY > RR->bounds.MaxY ||
              Rect.MaxX < RR->bounds.MinX ||
              Rect.MaxY < RR->bounds.MinY   ))
        {
          /* this is at least partly matching */
          if (NULL == CR_new)
          {
            CR_new = _AllocClipRect(L);
            _CR    = CR_new;
          }
          else
          {
            _CR->Next  = _AllocClipRect(L);
            _CR        = _CR->Next; 
          }

          /* That's what we need in any case */
          _CR->bounds.MinX = ClipRegion->bounds.MinX;
          _CR->bounds.MinY = ClipRegion->bounds.MinY;
          _CR->bounds.MaxX = ClipRegion->bounds.MinX;
          _CR->bounds.MaxY = ClipRegion->bounds.MinY;

          if (RR->bounds.MinX > Rect.MinX)
            _CR->bounds.MinX += RR->bounds.MinX + L->bounds.MinX;
          else
            _CR->bounds.MinX +=       Rect.MinX + L->bounds.MinX;

          if (RR->bounds.MinY > Rect.MinY)
            _CR->bounds.MinY += RR->bounds.MinY + L->bounds.MinY;
          else
            _CR->bounds.MinY +=       Rect.MinY + L->bounds.MinY;

          if (RR->bounds.MaxX < Rect.MaxX)
            _CR->bounds.MaxX += RR->bounds.MaxX + L->bounds.MinX;
          else
            _CR->bounds.MaxX +=       Rect.MaxX + L->bounds.MinX;

          if (RR->bounds.MaxY < Rect.MaxY)
            _CR->bounds.MaxY += RR->bounds.MaxY + L->bounds.MinY;
          else
            _CR->bounds.MaxY +=       Rect.MaxY + L->bounds.MinY;

          area -= (_CR->bounds.MaxX - _CR->bounds.MinX + 1) *
                  (_CR->bounds.MaxY - _CR->bounds.MinY + 1);

          /* copy important data from the original */
          _CR -> lobs   = CR -> lobs;
        
          /* copy parts of/ the whole bitmap in case of a SMART LAYER */
          if (TRUE == isSmart && NULL != CR->BitMap)
          {
            _CR->BitMap = AllocBitMap(_CR->bounds.MaxX - _CR->bounds.MinX + 1 + 15,
                                      _CR->bounds.MaxY - _CR->bounds.MinY + 1,
                                      GetBitMapAttr(CR->BitMap, BMA_DEPTH),
                                      0,
                                      CR->BitMap);
            if (NULL == _CR->BitMap)
              goto failexit;

            BltBitMap(CR->BitMap,
                      _CR->bounds.MinX - CR->bounds.MinX + ALIGN_OFFSET( CR->bounds.MinX),
                      _CR->bounds.MinY - CR->bounds.MinY,
                      _CR->BitMap,
                      ALIGN_OFFSET(_CR->bounds.MinX),
                      0,
                      _CR->bounds.MaxX - _CR->bounds.MinX + 1,
                      _CR->bounds.MaxY - _CR->bounds.MinY + 1,
                      0x0c0,/* copy */
                      0xff,
                      NULL);
          }
          else
          {
            _CR -> BitMap = CR -> BitMap;
          }
        }
        /* visit next RegionRectanlge */
        RR = RR->Next;
      }
    }
    
    /* check next layer */
    CR = CR->Next;
  }
  
  return CR_new;
  



/* out of memory failure */
failexit:
  
  if (NULL == CR_new)
    return NULL;

  CR = CR_new;

  while (TRUE)
  {
    if (TRUE == isSmart)
      FreeBitMap(CR->BitMap);
    
    if (NULL == CR->Next)
      break;
      
    CR = CR->Next;
    
  }

  /* concat the two lists of cliprects */
  CR->Next = L->SuperSaveClipRects;
  L->SuperSaveClipRects = CR_new;
  
  return NULL;
}

