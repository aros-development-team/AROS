/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>

/*****************************************************************************

    NAME */

	AROS_LH9(struct Layer *, CreateBehindHookLayer,

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
	struct LayersBase *, LayersBase, 32, Layers)

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
/*
  D(bug("CreateBehindHookLayer(li@$lx, bm@$lx, x0 %ld, y0 %ld, x1 %ld, y1 %ld, flags %ld, hook@$%lx, bm2@$lx)\n", li, bm, x0, y0, x1, y1, flags, hook, bm2));
*/

  struct Layer * L;
  struct ClipRect * CR;
  struct RastPort * RP;

  L  = (struct Layer    *) AllocMem(sizeof(struct Layer)   , MEMF_CLEAR|MEMF_PUBLIC);
  CR = (struct ClipRect *) AllocMem(sizeof(struct ClipRect), MEMF_CLEAR|MEMF_PUBLIC);
  RP = (struct RastPort *) AllocMem(sizeof(struct RastPort), MEMF_CLEAR|MEMF_PUBLIC);

  /* is everything there that I need?  */
  if (NULL != L && NULL != CR && NULL != RP)
  {
    struct Layer * L_front;
    /* first of all we init. the layers structure */
    L->ClipRect = CR;
    L->rp       = RP;

    L->bounds.MinX = x0;
    L->bounds.MinY = y0;
    L->bounds.MaxX = x1;
    L->bounds.MaxY = y1;

    L->Flags     = (WORD) flags;
    L->LayerInfo = li;
    L->Width     = x1-x0+1;
    L->Height    = y1-y0+1;
    L->SuperBitMap = bm2;

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

    /* and now init the RastPort structure */
    InitRastPort(RP);

    RP->Layer  = L;
    RP->BitMap = bm;

    /* 
       add the layer at the correct position in the linked list of layers
       non-BACKDROP layer: insert it before the first non-BACKDROP layer
           BACKDROP layer: insert it at the very end of the list  
    */
   
    L_front = li->top_layer;

    /* Now nobody else may play around with the layers while I am
     * doing this here 
     */
    LockLayers(li);

    if ((NULL == L_front) ||
        ( (0 != (L_front->Flags & LAYERBACKDROP)) && 
          (0 == (flags & LAYERBACKDROP) ) 
        ))
    {
      /* the new one is the very first one in the list */
      if (NULL != li->top_layer)
      {
        L ->back             = li->top_layer;
        li->top_layer->front = L;
      }
      /* make the new layer the top layer */
      li->top_layer          = L; 
    }
    else
    {
      /* search for the exact place in the list */
      if (0 != (flags & LAYERBACKDROP))
      {
        /* 
          for a BACKDROP layer we have to go to the very end of the list
         */
        while (NULL != L_front->back)
          L_front = L_front ->back;
        L_front->back = L;
        L -> front    = L_front;
      }
      else
      {
         /* 
           for a non BACKDROP layer we have to go to the end of the non-
           BACKDROP layers list
         */
         while  (NULL != L_front->back && 
               ((L_front->back->Flags & LAYERBACKDROP) == 0) )
           L_front = L_front->back;
         /* insert it behind L_front */
         if (L_front->back != NULL)
	 {
           L_front->back->front = L;
           L->back              = L_front->back;
         }
         L_front->back = L;
         L->front      = L_front;
      }
    }
    
    /* 
      Now create all ClipRects of all Layers correctly.   
      Comment: CreateClipRects is the only function that does the
               job correctly if you want to create a layer somewhere
               behind other layers.
    */

    CreateClipRects(li, L);

    if (li->top_layer == L && NULL != L->back)
      CreateClipRectsSelf(L->back, FALSE); 

    /*
       Ok, all other layers were visited and pixels are backed up.
       Now we can draw the new layer by clearing these parts of the
       displaybitmap for which the ClipRects of this layer have no
       entry in lobs (these are not hidden, but might be hiding other
       layers behind them, but those parts are backed up now)
    */

    
    /* the whole part from here ... */

    CR = L->ClipRect;
    while (CR != NULL)
    {
      /* only show thos cliprects that are visible, of course. */
      if (NULL == CR->lobs)
      {
        if (0 == (L->Flags & LAYERSUPER))
	{
          /* no superbitmap */
          BltBitMap(
            bm /* Source Bitmap - we don't need one for clearing, but this
                  one will also do :-) */,
            0,
            0,
            bm /* Destination Bitmap - */,
            CR->bounds.MinX,
            CR->bounds.MinY,
            CR->bounds.MaxX-CR->bounds.MinX+1,
            CR->bounds.MaxY-CR->bounds.MinY+1,
            0x000 /* supposed to clear the destination */,
            0xff,
            NULL
          );
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
 
    /* ... to here will be replaced by : */
    /*
    DoHookClipRects(hook, RP, L->bounds);
     */
    /*
       once DoHookClipRects is there and a default backfill is implemented. 
       (basically the same code as above) 
     */

    UnlockLayers(li);
  } /* if (all memory is there) */  
  else /* not enough memory */
  {
    if (NULL != L ) FreeMem(L , sizeof(struct Layer));
    if (NULL != RP) FreeMem(RP, sizeof(struct RastPort));
    if (NULL != CR) FreeMem(CR, sizeof(struct ClipRect));
    L = NULL;
  }

  return L;
  
  AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
