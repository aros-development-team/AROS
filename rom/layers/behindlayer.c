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

	AROS_LH2(LONG, BehindLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, L, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 9, Layers)

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
  struct Layer * _L;
  struct ClipRect * CR, * _CR;

  /* 
     If it's a non-BACKDROP layer and behind it is either no
     layer or a BACKDROP layer then we're done already
     If it's a BACKDROP layer and there's no layer behind it
     then we're also done.
   */
  if (   L->back == NULL ||
      ( (L->back->Flags & LAYERBACKDROP) != 0 && 
        (L      ->Flags & LAYERBACKDROP) == 0    )  )
    return TRUE;

  /* 
     At this point I am sure that something has to be moved.
     As I am moving a layer further behind I can be sure that
     none of its parts will be displayed that are hidden now, but
     some of its parts might be hidden that are visible now. 
   */ 
  LockLayers(LI);

  /*
     Make sure that all ClipRects are split correctly so that parts
     that are visible now and must be hidden later can go behind
     the right ClipRects. 
   */

  SetLayerPriorities(LI);

  if (LI->top_layer == L)
    CreateClipRectsTop(LI, FALSE);
  else
    CreateClipRectsSelf(L, FALSE);    

  /*
      Take the layer out of the list and put it in at the approriate
      place in the new layer.
   */

  if (LI->top_layer == L)
    LI -> top_layer = L -> back;
  else
    L -> front -> back = L -> back;

  if (NULL != L->back)
    L -> back -> front = L -> front;

  /*  
     Now look for the new position to hang the layer into the list
     I start out with the layer itself and look for the layer before 
     where I want to insert the layer.
   */

  _L = L;

  if ((L -> Flags & LAYERBACKDROP) == 0)
  {
    while (_L -> back != NULL)
    {
      if ((_L -> back -> Flags & LAYERBACKDROP) == 0)
        _L = _L -> back;
      else
        break;    
    }
  }
  else
  {
    while (NULL != _L -> back)
      _L = _L -> back;
  }

  /* _L points to the layer where i have to insert the layer *AFTER* */

  if (NULL != _L->back)
    _L->back->front = L;

   L -> back = _L->back;
  _L -> back = L;
   L -> front = _L;

  /* The layer is linked into its new position. */

  /* 
   * I have to visit all the ClipRects of the layer and see whether
   * they are still visible and I might have to hide them now.
   */

  CR = L->ClipRect;

  while (NULL != CR)
  {
kprintf("This Cliprect: MinX: %d, MaxX: %d, MinY: %d, MaxY: %d\n",
          CR->bounds.MinX,CR->bounds.MaxX,
          CR->bounds.MinY,CR->bounds.MaxY);
    /* Was this ClipRect visible before ??? */
    if (NULL == CR->lobs)
    {
kprintf("Is visible!!!\n");
      /* 
         Check which layer is now visible at this point. If its not
         the layer L then I will have to hide that part of the ClipRect.
       */
      _L = WhichLayer(LI, CR->bounds.MinX, CR->bounds.MinY);
      if (_L != L)
      {
        struct Layer * L_tmp = _L;
        /* 
           Another ClipRect is now visible in this area.
           Copy the ClipRects bitmap of the newly visible ClipRect
           to the Display BitMap and backup the Display BitMap into 
           that ClipRect.
	 */
kprintf("Priority: %d\n",L_tmp->priority);
//        CreateClipRectsSelf(L_tmp, FALSE);
        _CR = internal_WhichClipRect(_L, CR->bounds.MinX, CR->bounds.MinY);

        /*
            _CR [_L] is now visible and 
             CR [ L] is now hidden
	 */

if (CR->bounds.MinX != _CR->bounds.MinX ||
    CR->bounds.MaxX != _CR->bounds.MaxX ||
    CR->bounds.MinY != _CR->bounds.MinY ||
    CR->bounds.MaxY != _CR->bounds.MaxY)
  kprintf("!!!!!!!!!!!!! ERROR !!!!!!!!!\n");


kprintf("     Cliprect: MinX: %d, MaxX: %d, MinY: %d, MaxY: %d  (Pri: %d)\n",
          _CR->bounds.MinX,_CR->bounds.MaxX,
          _CR->bounds.MinY,_CR->bounds.MaxY,
          _L->priority);
kprintf("bl: A L->rp: %x, _CR: %x\n",L->rp,_CR);

        SwapBitsRastPortClipRect(L->rp, _CR);

kprintf("bl: B\n");

         CR -> lobs   = _L;
        _CR -> lobs   = NULL;
         CR -> BitMap = _CR -> BitMap;
        _CR -> BitMap = NULL;

kprintf("bl: C\n");
        /*
           Now I have to change all lobs-entries in the layers
           behind the layer that became visible (_L) so they are
           pointing to the correct layer
         */
        while (NULL != L_tmp -> back)
        {

if (CR->bounds.MinX != _CR->bounds.MinX ||
    CR->bounds.MaxX != _CR->bounds.MaxX ||
    CR->bounds.MinY != _CR->bounds.MinY ||
    CR->bounds.MaxY != _CR->bounds.MaxY)
  kprintf("!!!!!!!!!!!!! ERROR !!!!!!!!!\n");
if (CR->bounds.MinX >= _CR->bounds.MinX ||
    CR->bounds.MaxX <= _CR->bounds.MaxX ||
    CR->bounds.MinY >= _CR->bounds.MinY ||
    CR->bounds.MaxY <= _CR->bounds.MaxY)
  kprintf("Mine is smaller than the one to become visible!!\n");
else
  kprintf("Something is very wrong!!!!!!\n");

kprintf("     Cliprect: MinX: %d, MaxX: %d, MinY: %d, MaxY: %d\n",
          _CR->bounds.MinX,_CR->bounds.MaxX,
          _CR->bounds.MinY,_CR->bounds.MaxY);

          L_tmp = internal_WhichLayer(L_tmp->back, 
                                      CR->bounds.MinX, 
                                      CR->bounds.MinY);
          if (NULL == L_tmp)
            break;

//          CreateClipRectsSelf(L_tmp, FALSE);

          _CR = internal_WhichClipRect(L_tmp, 
                                       CR->bounds.MinX, 
                                       CR->bounds.MinY);
          _CR -> lobs = _L;
	} /* while */
      } /* if */

else kprintf("\t\tstays visible!!!\n");

    } /* if */
    CR = CR -> Next;
  } /* while */

  UnlockLayers(LI);

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BehindLayer */
