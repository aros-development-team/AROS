/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Implementation of MoveLayer()
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH4(LONG, MoveLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),
	AROS_LHA(LONG          , dx   , D0),
	AROS_LHA(LONG          , dy   , D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 10, Layers)

/*  FUNCTION
        Move the layer to a specified position in the bitmap.
        Parts of simple layers that become visible are added to
        the damage list and a refresh is triggered.

    INPUTS
        dummy - unused
        l     - pointer to layer to be moved
        dx    - delta to add to current x position
        dy    - delta to add to current y position

    RESULT
        result - TRUE everyting went alright
                 FALSE an error occurred (out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer * l_tmp;
  struct ClipRect * CR;
  struct RastPort * RP;
  struct Layer_Info * LI = l->LayerInfo;
  struct BitMap * SimpleBackupBM = NULL;
  struct Rectangle Rect;  /* The area with the backed up data if it is a
                             simple layer */
  BOOL retVal;

  /* Check coordinates as there's no support for layers outside the displayed
     bitmap. I might add this feature later. */
  if (l->bounds.MinX+dx < 0 ||
      l->bounds.MinY+dy < 0 ||
      l->bounds.MaxX+dx > GetBitMapAttr(l->rp->BitMap, BMA_WIDTH) ||
      l->bounds.MaxY+dy > GetBitMapAttr(l->rp->BitMap, BMA_HEIGHT))
    return FALSE; 

  if (0 == dx && 0 == dy)
    return TRUE;

  /* Lock all other layers while I am moving this layer */
  LockLayers(LI);
  
  /* 
     Here's how I do it:
     I create a new layer on top of the given layer at the new position,
     copy the bitmaps from the old layer to the new layer via ClipBlit() 
     and delete the old layer. 
     In order to maintain the pointer of the layer I will create a Layer
     structure, link it into the list behind the new layer, copy important
     data to the newly created structure and connect the cliprects to it,
     of course.
     One problem, however: If the layer is a simple layer and it is
     moved to a new position such that parts are overlapping with the layer
     at the old position then the pixels in the overlapping area are lost
     as the new simple layer is on top of the old simple layer. Therefore
     this area has to be backed up prior to creating the layer at the
     new position.
   */
  
  /*
    Is it a simple layer and will areas overlap with the layer at the new
    position?
  */
  
  if (0 != (l->Flags & LAYERSIMPLE))
  {
    LONG abs_dx, abs_dy;
    abs_dx =((LONG)dx >= 0) ? dx : -dx;
    abs_dy =((LONG)dy >= 0) ? dy : -dy;
    if ((l->bounds.MaxX - l->bounds.MinX + 1) > abs_dx &&
        (l->bounds.MaxY - l->bounds.MinY + 1) > abs_dy   )
    {
      /* it will overlap! */
      struct ClipRect * _CR;
      /* and start backing up right here in case it is needed at all 
         (the whole area might be hidden)
         Determine the rectangle with the overlapping area. 
      */
      Rect.MinX = (dx >= 0) ? l->bounds.MinX + dx 
                            : l->bounds.MinX;   
      Rect.MinY = (dy >= 0) ? l->bounds.MinY + dy
                            : l->bounds.MinY;
      Rect.MaxX = (dx <= 0) ? l->bounds.MaxX + dx
                            : l->bounds.MaxX;
      Rect.MaxY = (dy <= 0) ? l->bounds.MaxY + dy
                            : l->bounds.MaxY; 
      /* Walk throught the layer's cliprects and copy bitmap data
         to the backup bitmap. The backup bitmap, however, is only
         allocated if really needed 
      */
      _CR = l->ClipRect;
      while (NULL != _CR)
      {
        struct BitMap * BM = l->rp->BitMap;
        /* Check whether this is a ClipRect to be considered */
        if (NULL == _CR->lobs &&
            !(Rect.MinX >= _CR->bounds.MaxX ||
              Rect.MaxX <= _CR->bounds.MinX ||
              Rect.MinY >= _CR->bounds.MaxY ||
              Rect.MaxY <= _CR->bounds.MinY   ))
        {
          struct Rectangle CopyRect;
          if (NULL == SimpleBackupBM)
          {
            /* Now get the bitmap as there is a real reason */
            SimpleBackupBM = AllocBitMap(Rect.MaxX - Rect.MinX + 1,
                                         Rect.MaxY - Rect.MinY + 1,
                                         GetBitMapAttr(BM, BMA_DEPTH),
                                         BMF_CLEAR,
                                         BM);
            if (NULL == SimpleBackupBM)
            {
              /* not enough memory!! */
              UnlockLayers(LI);
              return FALSE;
            }
          }
          /* Copy the correct part from the screen's rastport 
             to the backup bitmap
          */
          CopyRect.MinX = (Rect.MinX >= _CR->bounds.MinX) ? Rect.MinX 
                                                          : _CR->bounds.MinX;
          CopyRect.MinY = (Rect.MinY >= _CR->bounds.MinY) ? Rect.MinY
                                                          : _CR->bounds.MinY;
          CopyRect.MaxX = (Rect.MaxX <= _CR->bounds.MaxX) ? Rect.MaxX
                                                          : _CR->bounds.MaxX;
          CopyRect.MaxY = (Rect.MaxY <= _CR->bounds.MaxY) ? Rect.MaxY
                                                          : _CR->bounds.MaxY;
          
          BltBitMap(BM,
                    CopyRect.MinX,
                    CopyRect.MinY,
                    SimpleBackupBM,
                    CopyRect.MinX - Rect.MinX,
                    CopyRect.MinY - Rect.MinY,
                    CopyRect.MaxX - CopyRect.MinX + 1,
                    CopyRect.MaxY - CopyRect.MinY + 1,
                    0x0c0, /* copy */
                    ~0,
                    NULL);                    
        }
        _CR = _CR->Next;
      }
    } /* if (overlapping) */
  } /* if (simple layer) */
  
  l_tmp = (struct Layer *)AllocMem(sizeof(struct Layer), MEMF_CLEAR|MEMF_PUBLIC);
  CR = _AllocClipRect(l);
  RP = CreateRastPort();

  if (NULL != l_tmp && NULL != CR && NULL != RP)
  {
    struct CR_tmp;
    struct Layer * l_behind; 


    /* link the temporary layer behind the layer to move */
    l_tmp -> front = l;
    l_tmp -> back  = l->back;
    l     -> back  = l_tmp;

    if (NULL != l_tmp->back)
      l_tmp->back->front = l_tmp;

    /* copy important data to the temporary layer. this list might be 
       shrinkable depending on what data deletelayer() needs later on */
    l_tmp->ClipRect   = l->ClipRect;
    l_tmp->rp         = RP;
    l_tmp->bounds     = l->bounds;
    l_tmp->Flags      = l->Flags;
    l_tmp->LayerInfo  = LI;
    l_tmp->SuperBitMap= l->SuperBitMap;
    l_tmp->Scroll_X   = l->Scroll_X;
    l_tmp->Scroll_Y   = l->Scroll_Y;
    l_tmp->DamageList = l->DamageList;

    /* further init the rastport structure of the temporary layer */
    RP -> Layer  = l_tmp;
    RP -> BitMap = l->rp->BitMap;

    /* I have to go through all the cliprects of the layers that are 
       behind this layer and have an enty in lobs pointing to l. I
       have to change this pointer to l_tmp, so that everything still
       works fine later, especially the DeleteLayer() call. */

    l_behind = l_tmp->back;
    while (NULL != l_behind)
    {
      struct ClipRect * _CR = l_behind->ClipRect;
      while (NULL != _CR)
      {
        if (_CR->lobs == l)
	{
          _CR->lobs = l_tmp;
	}
        _CR = _CR->Next;
      } /* while */
      l_behind = l_behind ->back;
    } /* while */

   
    InitSemaphore(&l_tmp->Lock);
    LockLayer(0, l_tmp);

    /* modify the layer l's structure for the new position */
    l->bounds.MinX += dx;
    l->bounds.MaxX += dx;
    l->bounds.MinY += dy;
    l->bounds.MaxY += dy;

    l->ClipRect = CR;

    l->DamageList = NewRegion();
 
    /* Copy the bounds */
    CR->bounds = l->bounds;
    /* 
      Now create all ClipRects of all Layers correctly.   
      Comment: CreateClipRects is the only function that does the
               job correctly if you want to create a layer somewhere
               behind other layers.
    */

    CreateClipRects(LI, l); 

    /*
       Ok, all other layers were visited and pixels are backed up.
       Now we can draw the new layer by copying all parts of the
       temporary layer's cliprects to the new layer via ClipBlit.
    */
    ClipBlit(l_tmp->rp,
             0,
             0,
             l->rp,
             0,
             0,
             l->bounds.MaxX - l->bounds.MinX + 1,
             l->bounds.MaxY - l->bounds.MinY + 1,
             0x0c0);
    
    if (NULL != SimpleBackupBM)
    {
      /* adjust the Rectangle Rect to the position of the new layer */
      Rect.MinX += dx;
      Rect.MinY += dy;
      Rect.MaxX += dx;
      Rect.MaxY += dy;
      /* now walk through all the cliprects again and check whether they
         have a part of this rectangle and if yes then copy it into the 
         bitmap of the screen.
         -> simpler: copy bitmap to the rastport w/ BltBitMapRastPort
      */
      BltBitMapRastPort(SimpleBackupBM,
                        0,
                        0,
                        l->rp,
                        Rect.MinX - l->bounds.MinX,
                        Rect.MinY - l->bounds.MinY,
                        Rect.MaxX - Rect.MinX + 1,
                        Rect.MaxY - Rect.MinY + 1,
                        0x0c0 /* copy */
                        );
      FreeBitMap(SimpleBackupBM);
    }


    if (0 != (l_tmp->Flags & LAYERSIMPLE))
    {
      struct ClipRect * _CR;
      struct Region * R = NewRegion();
      /* Walk through all the old layer's cliprects and check whether they
         were in(!)visible. If a part was not visible then add it to the 
         new layers damagelist 
      */
      _CR = l_tmp->ClipRect;
      while (NULL != _CR)
      {
        if (NULL != _CR->lobs)
        {
          /* this part was hidden! */ 
          OrRectRegion(l->DamageList, &_CR->bounds);
        } 
        _CR = _CR->Next;
      }
      /* a necessary adjustment to the Damagelist's base coordinates! */
      l->DamageList->bounds.MinX += dx;
      l->DamageList->bounds.MinY += dy;
      l->DamageList->bounds.MaxX += dx;
      l->DamageList->bounds.MaxY += dy;
      l->Flags |= LAYERREFRESH;
      
      /* Comparison with layer at new position is absolutely necessary!! 
      **  Collect all visible(!) cliprects in the new layer and make
      **  a logical AND with both areas. The result will be the areas
      **  that need to be update in the new layer.
      */
      _CR = l->ClipRect;
      while (NULL != _CR)
      {
        /* is it visible at the new location? */
        if (NULL == _CR->lobs)
        {
          /* this part is visible! Collect it! */
          OrRectRegion(R, &_CR->bounds);
        }
        _CR = _CR->Next;
      }
      /* Determine the valid parts */
      AndRegionRegion(R, l->DamageList);
      DisposeRegion(R);

      /* If I was certain that the damasglist in the old layer is correct
      ** I wouldn't have to do all of the above!! This just be tried later.
      */
    }

    /* 
      The layer that was moved is totally visible now at its new position
      and also at its old position. I delete it now from its old position.
    */
    DeleteLayer(0, l_tmp);

    /* That's it folks! */
    CleanupLayers(LI);
    retVal = TRUE;
  } 
  else /* not enough memory */
  {
    if (NULL != CR   ) _FreeClipRect(CR, l);
    if (NULL != RP   ) FreeRastPort(RP);
    if (NULL != l_tmp) FreeMem(l_tmp, sizeof(struct Layer));
    if (NULL != SimpleBackupBM) FreeBitMap(SimpleBackupBM);
    retVal = FALSE;
  }

  /* Now everybody else may play with the layers again */
  UnlockLayers(LI);
  return retVal;

  AROS_LIBFUNC_EXIT
} /* MoveLayer */
