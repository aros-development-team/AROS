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

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf



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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
      No support of backdrop layers.
      No support of simple layers. (the easier part :-)) )
      No support of superbitmaps.

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

  /* no one else may screw around with the layers I will be working with */
  /* don't forget to add UnlockLayers(li) before any return-statement! */
  LockLayers(li);

  L  = (struct Layer    *) AllocMem(sizeof(struct Layer)   , MEMF_CLEAR|MEMF_PUBLIC);
  CR = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_CLEAR|MEMF_PUBLIC);
  RP = (struct RastPort *) AllocMem(sizeof(struct RastPort), MEMF_CLEAR|MEMF_PUBLIC);

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

    InitSemaphore(&L->Lock);
    /* also lock this one layer, because I will insert it into the
       linked list of layers and then UnLockLayers will be called and
       Unlock this layer.
    */
    LockLayer(NULL, L);

    L->DamageList = NewRegion();

    /*
      L->priority = ;
    */

    /* and now init the ClipRect structure */
    CR->bounds.MinX = x0;
    CR->bounds.MinY = y0;
    CR->bounds.MaxX = x1;
    CR->bounds.MaxY = y1;

    /* and now init the RastPort structure */
    InitRastPort(RP);

    RP->Layer  = L;
    RP->BitMap = bm;

    /* add the layer to the list of layers. It becomes the first one
       in the doubly linked list */
    L_behind          = li->top_layer;
    if (NULL != L_behind)
    {
      L_behind->front = L;
      L->back         = L_behind;

      /*
        L->_cliprects   = ;
      */
    }
    /* make the new layer the top layer.*/
    li->top_layer     = L;

    CreateClipRectsBehindLayer(L);

    /*
       Ok, all other layers were visited and pixels are backed up.
       Now we can draw the new layer by clearing the part of the
       displaybitmap.
    */
    BltBitMap(
      bm /* Source Bitmap - we don't need one for clearing, but this
            one will also do :-) */,
      0,
      0,
      bm /* Destination Bitmap - */,
      x0,
      y0,
      x1-x0+1,
      y1-y0+1,
      0x000 /* supposed to clear the destination */,
      0xff,
      NULL
    );

  }
  else /* not enough memory */
  {
    if (NULL != L ) FreeMem(L , sizeof(struct Layer));
    if (NULL != RP) FreeMem(RP, sizeof(struct RastPort));
    if (NULL != CR) FreeMem(CR, sizeof(struct ClipRect));
    L = NULL;
  }

  /* let anybody else play with these layers - I'm done. */
  UnlockLayers(li);
  return L;

  AROS_LIBFUNC_EXIT
} /* CreateUpfrontHookLayer */

