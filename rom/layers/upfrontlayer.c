/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <layers_intern.h>
#include <aros/libcall.h>
#include <proto/graphics.h>


/*****************************************************************************

    NAME */

	AROS_LH2(LONG, UpfrontLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, L, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 8, Layers)

/*  FUNCTION
        Brings a layer to the front. If this layer is a backdrop layer
        it is brought in front of all backdrop layers and behind the
        last non-backdrop layer. By clearing the BACKDROP flag of a layer
        a backdrop layer can be brought in front of all other layers.
        Parts of a simple layer that become visible are added to the 
        damage list and the REFRESH flag is set.

    INPUTS
        dummy - unused
        L     - pointer to layer

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved (probably out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateUpfrontLayer() CreateUpfrontHookLayer() BehindLayer()
        CreateBehindLayer() CreateBehindHookLayer()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer_Info * LI = L->LayerInfo;
  struct ClipRect * CR_old = L->ClipRect;

  /* see whether this is already the most upfront layer of its kind */
  if (LI->top_layer == L || 
       ( (L->Flags & LAYERBACKDROP) != 0 &&  
         (L->front->Flags & LAYERBACKDROP) == 0))
    return TRUE;

  /*
     If there are only BACKDROP layers in the list I will also
     treat it here, otherwise I will simply call MoveLayerInfrontOf
     for the BACKDROP layer.
   */

  if ((L->Flags & LAYERBACKDROP) != 0 &&
      (LI->top_layer -> Flags & LAYERBACKDROP) == 0)
  {
    /* L is a BACKDROP layer and there are non-BACKDROP layers
       in front of it, so I will call MoveLayerInfrontOf() and
       put L in front of the first BACKDROP layer.
    */
    struct Layer * L_behind = L->front;
    while ((L_behind->front->Flags & LAYERBACKDROP) != 0)
      L_behind =  L_behind->front;
    return MoveLayerInFrontOf(L, L_behind);
  }  
  
  /* I actually have to move it to the very front of the layers. */
  /* no one else may interrupt me while I do this */
  LockLayers(LI);

  /* take layer out of the list of layers */
  L->front->back = L->back;
  if (NULL != L->back)
    L->back->front = L->front;
    
  /* and hang it in at the position where it belongs */

  /* as it's a non-BACKDROP layer I put it in the front */
  L->back  = LI->top_layer;
  L->front = NULL;
  /* I am in front of the old top_layer */

  LI->top_layer->front = L;
  /* and now I am the top_layer */
  LI->top_layer = L;
      
  /* get a new cliprect structure */
  L->ClipRect = _AllocClipRect(L);

  /* and initilize it with the layer's bounds */
  L->ClipRect->bounds = L->bounds;

  /*
     And split all ClipRects that are further behind 
     Comment: This seems to be the only way for backing up
              the bitmaps of the layers behind it and the
              amount of cliprects is also at a minimum.
   */
  CreateClipRects(LI, L);

  /* 
     Cool! Now we process the ClipRectsList and restore the
     cliprects found there and free the bitmaps and the
     list itself while doing so. 
   */
  while (NULL != CR_old)
  {
    struct ClipRect * _CR_old = CR_old->Next;
    /* treat simple layer separately */
    if (NULL != CR_old->lobs)
    {
      if (0 == (L->Flags & LAYERSIMPLE))
      {
        /* 
           This ClipRect was hidden before, but not any more, so let's
           show what we have there. 
        */
        if (0 == (L->Flags & LAYERSUPER))
        {
          /* no superbitmap */
          BltBitMap(
            CR_old->BitMap,
            CR_old->bounds.MinX & 0x0f,
            0,
            L->rp->BitMap,
            CR_old->bounds.MinX,
            CR_old->bounds.MinY, 
            CR_old->bounds.MaxX - CR_old->bounds.MinX + 1,
            CR_old->bounds.MaxY - CR_old->bounds.MinY + 1,
            0x0c0, /* copy */
            0xff,
            NULL
          );
          FreeBitMap(CR_old->BitMap);
        }
        else
        {
          /* with superbitmap */
          BltBitMap(
            L->SuperBitMap,
            CR_old->bounds.MinX - L->bounds.MinX + L->Scroll_X,
            CR_old->bounds.MinY - L->bounds.MinY + L->Scroll_Y,
            L->rp->BitMap,
            CR_old->bounds.MinX,
            CR_old->bounds.MinY, 
            CR_old->bounds.MaxX - CR_old->bounds.MinX + 1,
            CR_old->bounds.MaxY - CR_old->bounds.MinY + 1,
            0x0c0, /* copy */
            0xff,
            NULL
          );
        } /* else (superbitmapped layer) */
      }
      else
      {
        /* a simple layer */
        OrRectRegion(L->DamageList, &CR_old->bounds);
        L->Flags |= LAYERREFRESH;
      }
    }
    _FreeClipRect(CR_old, L);
    CR_old = _CR_old;
  } /* while () */    

  /*
     To keep consistency of the layers' cliprects I have to split the
     layer that has previously been the front layer.
   */

  if (NULL != L->back)
    CreateClipRectsSelf(L->back, FALSE);

  CleanupLayers(LI);

  /* Ok, I am done now. */
  UnlockLayers(LI);

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* UpfrontLayer */
