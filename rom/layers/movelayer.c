/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
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
        No support for simple layers.

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

  /* Check coordinates as there's no suport for layers outside the displayed
     bitmap. I might add this feature later. */
  if (l->bounds.MinX+dx < 0 ||
      l->bounds.MinY+dy < 0 ||
      l->bounds.MaxX+dx > GetBitMapAttr(l->rp->BitMap, BMA_WIDTH) ||
      l->bounds.MaxY+dy > GetBitMapAttr(l->rp->BitMap, BMA_HEIGHT))
    return FALSE; 
  

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
   */
  
  l_tmp = (struct Layer *)AllocMem(sizeof(struct Layer)  , MEMF_CLEAR|MEMF_PUBLIC);
  CR = (struct ClipRect *)AllocMem(sizeof(struct ClipRect), MEMF_CLEAR|MEMF_PUBLIC);
  RP = (struct RastPort *)AllocMem(sizeof(struct RastPort), MEMF_CLEAR|MEMF_PUBLIC);

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
       shrinkable
       depending on what data deletelayer() needs later on */
    l_tmp->ClipRect   = l->ClipRect;
    l_tmp->rp         = RP;
    l_tmp->bounds     = l->bounds;
    l_tmp->Flags      = l->Flags;
    l_tmp->LayerInfo  = LI;
    l_tmp->DamageList = l->DamageList;
    l_tmp->SuperBitMap= l->SuperBitMap;
    l_tmp->Scroll_X   = l->Scroll_X;
    l_tmp->Scroll_Y   = l->Scroll_Y;

    /* init the rastport structure of the temporary layer */
    InitRastPort(RP);
    RP -> Layer  = l_tmp;
    RP -> BitMap = l->rp->BitMap;

    /* I have to go through all the cliprects of the layers that are 
       behind this layer and have an enty in lobs pointing to l. I
       have to change this pointer to l_tmp, so that everything still
       works fine later, especially the DeleteLayer() */

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

    /* 
      The layer that was moved is totally visible now at its new position
      and also at its old position. I delete it now from its old position.
    */
    DeleteLayer(0, l_tmp);

    /* That's it folks! */

    /* Now everybody else may play with the layers again */
    UnlockLayers(l->LayerInfo);
    return TRUE;
  } 
  else /* not enough memory */
  {
    if (NULL != CR   ) FreeMem(CR, sizeof(struct ClipRect));
    if (NULL != RP   ) FreeMem(RP, sizeof(struct RastPort));
    if (NULL != l_tmp) FreeMem(l_tmp, sizeof(struct Layer));
  }



  return FALSE;

  AROS_LIBFUNC_EXIT
} /* MoveLayer */
