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

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH2(LONG, UpfrontLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, L, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 8, Layers)

/*  FUNCTION

    INPUTS

    RESULT

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

  struct Layer_Info * LI = L->LayerInfo;

  
  /* see whether this is already the most upfront layer */
  if (LI->top_layer != L)
  {
    /* no, we actually have to move it to the front. */
    struct ClipRect * CR_old = L->ClipRect;
    /* no one else may interrupt me while I do this */
    LockLayers(LI);

    /* take layer out of the list of layers */
    L->front->back = L->back;
    if (NULL != L->back)
      L->back->front = L->front;
    
    /* and hang it in at the front */
    L->back = LI->top_layer;
    L->front = NULL;
    /* I am in front of the old top_layer */
    LI->top_layer->front = L;
    /* and now I am the top_layer */
    LI->top_layer = L;
    
      
    /* get a new cliprect structure */
    L->ClipRect = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_CLEAR|MEMF_PUBLIC);

    /* and init. it with the layer's bounds */
    L->ClipRect->bounds = L->bounds;
    
    /* and now we insert this layer at the top and split the
       layers behind it */

    CreateClipRectsBehindLayer(L);

    /* Cool! Now we process the ClipRectsList and restore the
       cliprects found there and free the bitmaps and the
       list itself while doing so. */
    while (NULL != CR_old)
    {
      struct ClipRect * _CR_old = CR_old->Next;
      if (NULL != CR_old->lobs)
      {
        /* this ClipRect was hidden before, but not any more, so let's
           show what we have there. */
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
      FreeMem(CR_old, sizeof(struct ClipRect));
      CR_old = _CR_old;
    }    

    /* Ok, I am done now. */
    UnlockLayers(LI);
  }
  

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* UpfrontLayer */
