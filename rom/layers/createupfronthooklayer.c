/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include "layers_intern.h"
#include "basicfuncs.h"


/*****************************************************************************

    NAME */
#include <proto/layers.h>

        AROS_LH9(struct Layer *, CreateUpfrontHookLayer,

/*  SYNOPSIS */
        AROS_LHA(struct Layer_Info *, li, A0),
        AROS_LHA(struct BitMap     *, bm, A1),
        AROS_LHA(LONG               , x0, D0),
        AROS_LHA(LONG               , y0, D1),
        AROS_LHA(LONG               , x1, D2),
        AROS_LHA(LONG               , y1, D3),
        AROS_LHA(LONG               , flags, D4),
        AROS_LHA(struct Hook       *, hook, A3),
        AROS_LHA(struct BitMap     *, bm2, A2),

/*  LOCATION */
        struct LayersBase *, LayersBase, 31, Layers)

/*  FUNCTION
        Create a new layer at the given position and with the
        given size. The new layer will be in front of all other
        layers. If it is a backdrop layer it will be created
        in front of all other backdrop layers and behind all
        non backdrop layers.
        Install the given hook as a backfill hook. This hook will
        be called whenever a part of the layer is supposed to be
        filled with a certain pattern. The backfill hook has to
        do that.
        If a super bitmap layer is wanted the flags LAYERSUPER and
        the flag LAYERSMART have to be set and a pointer to a 
        bitmap must also be passed to this function. 

    INPUTS
        li    - pointer to LayerInfo structure
        bm    - pointer to common bitmap
        x0, y0- upper left corner of the layer
        x1, y1- lower right corner of the layer
        flags - choose the type of layer by setting some flags
        hook  - pointer to the backfill hook of this layer
                The backfill hook will be called with
                     object = (struct RastPort *) result->RastPort
                and message = [ (struct Layer *) layer,
                                (struct Rectangle) bounds,
                                (WORD) offsetx,
                                (WORD) offsety ]
        bm2   - pointer to optional super bitmap. 

    RESULT
        pointer to layer if successful, NULL otherwise

    NOTES
        Does not allow to create layers that are partially outside
        the given bitmap (, yet).
 
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

  struct Layer * L;
  struct ClipRect * CR;
  struct RastPort * RP;

  /* 
  **  First check whether the layer will totally fit into the given
  **  bitmap
   */
  if (x0 < 0 
      || y0 < 0 
      || x1 > GetBitMapAttr(bm, BMA_WIDTH) 
      || y1 > GetBitMapAttr(bm, BMA_HEIGHT))
    return NULL;

  L  = (struct Layer    *) AllocMem(sizeof(struct Layer)   , MEMF_CLEAR|MEMF_PUBLIC);
  CR = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_CLEAR|MEMF_PUBLIC);
  RP = CreateRastPort();

  /* is everything there that I need?  */
  if (NULL != L && NULL != CR && NULL != RP)
  {
    struct Layer * L_behind;
    /* first of all we init. the layers structure */
    L->ClipRect = CR;
    L->rp       = RP;

    L->bounds.MinX = x0;
    L->bounds.MinY = y0;
    L->bounds.MaxX = x1;
    L->bounds.MaxY = y1;

    L->Flags = (WORD) flags;
    L->LayerInfo = li;
    L->Width  = x1-x0+1;
    L->Height = y1-y0+1;
    L->SuperBitMap = bm2;
    L->BackFill = hook;
    
    InitSemaphore(&L->Lock);
    LockLayer(0, L);

    L->DamageList = NewRegion();

    /*
      L->priority = ;
    */

    /* and now init the ClipRect structure */
    CR->bounds.MinX = x0;
    CR->bounds.MinY = y0;
    CR->bounds.MaxX = x1;
    CR->bounds.MaxY = y1;

    /* and now further init the RastPort structure */

    RP->Layer  = L;
    RP->BitMap = bm;

    /* add the layer at the correct position in the linked list of layers
       non-BACKDROP layer: insert it as the very first layer
           BACKDROP layer: insert if befor the first BACKDROP layer
                           found in the list.
			   */
   
    /* no one else may fool around with the layers I will be working with */
    /* don't forget to add UnlockLayers(li) before any return-statement! */

    LockLayers(li);
    
    /* non BACKDROP? or layer is the very first one in the list */
    L_behind = li->top_layer;
    
    if ((flags & LAYERBACKDROP) == 0 ||
        NULL == L_behind || 
        (L_behind->Flags & LAYERBACKDROP)  != 0)
    {
      if (NULL != L_behind)
      {
        L_behind -> front = L;
        L        -> back  = L_behind;
      }
      /* make the new layer the top layer.*/
      li->top_layer     = L;
    }
    else
    {
      /* we have a BACKDROP layer and there a some non-BACKDROP layers */
      while ( L_behind->back != NULL &&
             (L_behind->back->Flags & LAYERBACKDROP) == 0)
        L_behind = L_behind->back;
      /* 
         L_behind now points to the layer that the new layer has
         to go behind. 
      */
      if (NULL != L_behind->back)
      {
        L_behind->back->front = L;
      }
      L       ->front = L_behind;
      L       ->back  = L_behind->back;
      L_behind->back  = L;
    }

    /* 
        Now create all ClipRects of all Layers correctly.
        Comment: The purpose here has to be to backup display data
                 of layers that are to be hidden now. 
     */
    SetLayerPriorities(li);
    UninstallClipRegionClipRects(li);
    
    CreateClipRectsOther(L);
    /*
       And to keep consistency of the layers I have to split the layer 
       that is behind the new top layer, if I generated the top-layer,
       otherwise (for a BACKDROP layer) I have to split the BACKDROP
       layer itself.
       Comment: This function call might be *unnecessary*, depending on
                how moving, resizing etc. layers is handled.
                Other functions are depending on this call however,
                if they depend on the consistency of the cliprects.
     */
 
    if (li->top_layer == L && NULL != L->back)
      CreateClipRectsSelf(L->back, FALSE);
    else
      CreateClipRectsSelf(L, FALSE);
 
 
    /*
       Ok, all other layers were visited and pixels are backed up.
       Now we can draw the new layer by clearing these parts of the
       displaybitmap for which the ClipRects of this layer have no
       entry in lobs (these are not hidden, but might be hiding other
       layers behind them, but those parts are backed up now)
    */

    CR = L->ClipRect;
    while (CR != NULL)
    {
      if (NULL == CR->lobs)
      {
        if (0 == (L->Flags & LAYERSUPER))
	{
          /* no superbitmap */
          
          /* clear the visible area with the given hook */
          _CallLayerHook(hook,
                         L->rp,
                         L,
                         &CR->bounds,
                         0,
                         0);
	}
        else
	{
          /* with superbitmap */
          BltBitMap(
            bm2 /* Source Bitmap = superbitmap  */,
            CR->bounds.MinX - L->bounds.MinX /* + L->Scroll_X = 0! */,
            CR->bounds.MinY - L->bounds.MinY /* + L->Scroll_Y = 0! */,
            bm, /* Destination Bitmap - */
            CR->bounds.MinX,
            CR->bounds.MinY,
            CR->bounds.MaxX-CR->bounds.MinX+1,
            CR->bounds.MaxY-CR->bounds.MinY+1,
            0x0c0, /* copy  */
            0xff,
            NULL
          );
	}
      }
      CR = CR->Next;
    }

    InstallClipRegionClipRects(li);

    UnlockLayers(li);    
  }
  else /* not enough memory */
  {
    if (NULL != L ) FreeMem(L , sizeof(struct Layer));
    if (NULL != RP) FreeRastPort(RP);
    if (NULL != CR) FreeMem(CR, sizeof(struct ClipRect));
    L = NULL;
  }

  return L;

  AROS_LIBFUNC_EXIT
} /* CreateUpfrontHookLayer */
