/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/layers.h>
#include <graphics/regions.h>
#include "layers_intern.h"
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH2(LONG, DeleteLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, LD   , A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 15, Layers)

/*  FUNCTION
        Deletes the layer. Other layers that were hidden (partially)
        will become visible. If parts of a simple layer become
        visible those parts are added to the damagelist of the
        layer and the LAYERREFRESH flags is set.

    INPUTS
        dummy - nothing special
        LD    - layer to be deleted

    RESULT
        TRUE  - layer was successfully deleted
        FALSE - layer could not be delete (out of memory) 

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

  /*
    This is how  we do it:
      The layer to be deleted is called LD
      
    - All layers in front of the layer LD
      are touched in no way by this operation.

    - The ClipRects of the layer LD that are hidden by
      another layer can be deallocated immediately as
      no refresh has to be made there.
      These ClipRects can be recognized by lobs != NULL.
      A bitmap is also found there and must be deallocated. 

    - For the layer immediately behind the layer to be deleted:
      If a ClipRect->lobs of that layer is pointing to the
      layer to be destroyed then the bitmap found there can be 
      restored, the bitmap deallocated and the pointer lobs
      has to be zeroed. The restored ClipRect has to be cut
      out (splitting the rectangle) of the list of the 
      cliprects of the layer LD.
      
    - For all layers further behind:
      If a ClipRect is found with lobs pointing to layer LD
      ask in which layer (staring with the layer behind LD) the 
      upper left corner is located. If it is located in the layer
      the ClipRect belongs to, then restore the rectangle, cut
      it out of the ClipRect list of the layer LD. 
      Otherwise make lobs point to the result of the call to 
      internal_WhichLayer();
  
    - If we take out the very first layer then the new top layer
      doesn't have to have several cliprects. It just needs to have
      one single cliprect.

  Ok, here we go: 
  */
  
  struct Layer_Info * LI = LD->LayerInfo;
  struct ClipRect * CR, * _CR;
  struct Region * R;
  struct RegionRectangle * RR;

  /* no one may interrupt me */
  LockLayers(LI);

  /* take this Layer out of the list of layers */
  if (NULL != LD->front)
  {
    LD->front->back = LD->back;

    if (LD->back)
      LD->back->front = LD->front;
  }
  else /* take out the very first layer */
  {
    LI -> top_layer = LD -> back;

    if (NULL != LD->back)
      LD->back->front = NULL;
  }
 
  /*
     Let's delete the ClipRects of the layer LD that are
     hidden themselves. 
     The other ClipRects I add to the damage List for
     me to refresh (clear) through at the end.
  */

  UninstallClipRegionClipRects(LI);

  /* clear the region that is there */
  ClearRegion(LD->DamageList);

  CR = LD->ClipRect;
  while (NULL != CR)
  {
    BOOL status;
    struct ClipRect * _CR = CR->Next;
    
    if (NULL != CR->lobs)
    {
      /* 
         This ClipRect of the layer is hidden. So we can simply 
         free the bitmap data there, if it's not a superbitmap and
         not a simple layer.
       */
      if (0 == (LD->Flags & (LAYERSUPER|LAYERSIMPLE)))
        FreeBitMap(CR->BitMap);
    }
    else
    {
      /* 
         Add this rectangle to the region which we might have
         to clear later on. Parts that do not have to be cleared
         later on will be taken out further below with a call
         to ClearRectRegion(). These parts are restored from
         other layers which were hidden by that layer.
      */
      /* This damaglist is built relative to three screen!
         Usually the damage list has to be relative to the layer
      */
      status = OrRectRegion(LD->DamageList, &CR->bounds);
      if (FALSE == status)
      {
        /* 
           We ran out of memory. Hope this never happens, as
           some of the CR->BitMaps might already be gone. 
         */
        UnlockLayers(LI);
        return FALSE;
      }
      /* 
        If this layer has a superbitmap I must store the information
        of the visible area into the superbitmap. 
       */
      if (0 != (LD->Flags & LAYERSUPER))
      {
        BltBitMap(
          LD->rp->BitMap, /* visible bitmap */
          CR->bounds.MinX,
          CR->bounds.MinY,
          LD->SuperBitMap,  /* storage area */
          CR->bounds.MinX - LD->bounds.MinX + LD->Scroll_X,
          CR->bounds.MinY - LD->bounds.MinY + LD->Scroll_Y,
          CR->bounds.MaxX - CR->bounds.MinX + 1,
          CR->bounds.MaxY - CR->bounds.MinY + 1,
          0x0c0, /* copy */
          0xff,
          NULL
	);           
      }
    }
    /* 
       Is there still another layer connected to LI that might need 
       preallocated cliprects??
    */
    FreeMem(CR, sizeof(struct ClipRect));
    CR = _CR;
  }
  /* 
     just to make sure...
     Remember: The ClipRects list is now invalid!
  */

  /* there is a damagelist left and there is a layer behind */
  if (NULL != LD->DamageList->RegionRectangle && NULL != LD->back)
  {
    /* 
       Let's visit the layers that are behind this layer 
       start out with the one immediately behind the layer LD 
     */
     
    struct Layer * L_behind = LD->back;

    CR = L_behind -> ClipRect;

    while (NULL != L_behind)
    {
      CR = L_behind -> ClipRect;
      /* go through all ClipRects of the Layer L_behind. */
      while (NULL != CR)
      {
        /* 
           If the ClipRect of the layer L_behind was previously
           hidden by the deleted Layer LD then we might have to
           make it visible if it is not hidden by yet another
           layer.
         */
        if (LD == CR->lobs)
        {
          struct Layer * Ltmp = internal_WhichLayer(
            LD->back,
            CR->bounds.MinX,
            CR->bounds.MinY
          );
          /* 
              If this layer is now visible ... 
              (it was previously hidden by the layer ld, but not anymore) 
          */
          if (Ltmp == L_behind)
          {  
            /* treat simple layers separately */
            if (0 == (L_behind->Flags & LAYERSIMPLE))
            {
              /* ... restore the bitmap stuff found there */
              if (0 == (L_behind->Flags & LAYERSUPER))
	      {
                /* no SuperBitMap */
                BltBitMap(
                  CR->BitMap,
                  CR->bounds.MinX & 0x0f,
                  0,
                  LD->rp->BitMap,
                  CR->bounds.MinX,
                  CR->bounds.MinY,
                  CR->bounds.MaxX - CR->bounds.MinX + 1,
                  CR->bounds.MaxY - CR->bounds.MinY + 1,
                  0x0c0, /* copy */
                  0xff,
                  NULL
                );
                /*
                  Also free the bitmap as it's useless now.
   	        */
                FreeBitMap(CR->BitMap);
	      }
              else
	      {
                /* with SuperBitMap */
                BltBitMap(
                  CR->BitMap,
                  CR->bounds.MinX - L_behind->bounds.MinX + L_behind->Scroll_X,
                  CR->bounds.MinY - L_behind->bounds.MinY + L_behind->Scroll_Y,
                  LD->rp->BitMap,
                  CR->bounds.MinX,
                  CR->bounds.MinY,
                  CR->bounds.MaxX - CR->bounds.MinX + 1,
                  CR->bounds.MaxY - CR->bounds.MinY + 1,
                  0x0c0, /* copy */
                  0xff,
                  NULL);

	      }
	    }
	    else
	    {
	      struct Rectangle Rect = CR->bounds;
	      Rect.MinX -= L_behind->bounds.MinX;
	      Rect.MinY -= L_behind->bounds.MinY;
	      Rect.MaxX -= L_behind->bounds.MinX;
	      Rect.MaxY -= L_behind->bounds.MinY;
	      
	      OrRectRegion(L_behind->DamageList, &Rect);
	      /* this layer needs a refresh in that area */
	      L_behind->Flags |= LAYERREFRESH;
	      
	      /* I call the hook for this area */
	      _CallLayerHook(L_behind->BackFill,
	                     L_behind->rp,
	                     L_behind,
	                     &CR->bounds,
	                     CR->bounds.MinX,
	                     CR->bounds.MinY);
	    }
            /* 
               clear the lobs entry and BitMap entry 
            */
            CR->BitMap = NULL;
            CR->lobs   = NULL;
            /*
               Take this ClipRect out of the damagelist so that
               this part will not be cleared later on. 
             */
            /* This damagelist is relative to the screen!!! */
            ClearRectRegion(LD->DamageList, &CR->bounds);
          } /* if */
          else /* Ltmp != L_behind */
          {
            /* 
               The entry in lobs of the current ClipRect says that this
               ClipRect was previously hidden by the deleted layer, but
               it is still hidden by yet another layer. So I have to change
               the entry in lobs to the layer that is currently hiding this
               part.
             */
            CR -> lobs = Ltmp;
            
            if (0 != (L_behind->Flags & LAYERSIMPLE))
            {
              struct Rectangle Rect = CR->bounds;
              Rect.MinX -= CR->bounds.MinX;
              Rect.MinY -= CR->bounds.MinY;
              Rect.MaxX -= CR->bounds.MinX;
              Rect.MaxY -= CR->bounds.MinY;
              ClearRectRegion(L_behind->DamageList, &Rect);
            }
          }  
        } /* if */
        CR = CR->Next;
      } /* while */
      L_behind = L_behind -> back;
    }
  }
    
  /* 
     The List of the ClipRects of the layer LD should
     now only contain these parts that have to be cleared
     in the bitmap. 
  */
  /*
      The last thing we have to do now is clear those parts of
      the deleted layer that were not hiding anything.
  */
  R = LD->DamageList;

  RR = R->RegionRectangle;
  /* check if a region is empty */
  while (NULL != RR)
  {
    RR->bounds.MinX += R->bounds.MinX;
    RR->bounds.MinY += R->bounds.MinY;
    RR->bounds.MaxX += R->bounds.MinX;
    RR->bounds.MaxY += R->bounds.MinY;
    _CallLayerHook(LI->BlankHook,
                   LD->rp,
                   LD,
                   &RR->bounds,
                   RR->bounds.MinX,
                   RR->bounds.MinY);

    RR = RR->Next;
  } /* while */
  
  /*
     Free 
     - rastport
     - damagelist
     - preallocated cliprects 
     - layer structure itself
     
   */

  FreeRastPort(LD->rp);
  DisposeRegion(LD->DamageList);

  /* 
     Does the Layer structure have any preallocated cliprects? If yes then free
     all allocated ClipRect structures here.
  */
  
  if (NULL != (CR = LD->SuperSaveClipRects))
  {
    /* Free the list of allocated ClipRects */
    do
    {
      _CR = CR->Next;
      FreeMem(CR, sizeof(struct ClipRect));
      CR = _CR;
    }
    while (NULL != CR);
  }

  /*
    Now as this layer is gone other layers that used to be split by it
    can have their cliprects recombined. As MoveLayer(), SizeLayer() &
    MoveSizeLayer() are the critical functions and they all call 
    DeleteLayer() this is the best place to do this. 
  */
  UnsplitLayers(LI, &LD->bounds);

  FreeMem(LD, sizeof(struct Layer));

  InstallClipRegionClipRects(LI);
  /* ok, I'm done */
  UnlockLayers(LI);
  return TRUE;
  
  AROS_LIBFUNC_EXIT
} /* DeleteLayer */
