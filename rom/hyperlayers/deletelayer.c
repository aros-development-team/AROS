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
	AROS_LHA(struct Layer *, l    , A1),

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

  struct Layer * _l = l->back;
  struct ClipRect * cr, *_cr;
  /*
   * all children must have been destroyed before.
   */
  if (l != _FindFirstFamilyMember(l))
    return FALSE;

 
  if (IS_VISIBLE(l))
  { 
    struct Region * show_region = NewRegion();

    if (l->back)
    {
      /*
       * The layer to be deleted has the VisibleRegion
       * excluding the ones in front of him.
       * The layer beihind it as the VisibleRegion
       * excluding the ones in front of him and the layer
       * that is to be deleted.
       * If I calc the VisibleRegion of the current layer
       * minus the VisibleRegion of the layer behind it
       * I get the area that the current layer is hiding and
       * that can now be made visible.
       */ 
      OrRegionRegion(l->VisibleRegion, show_region);
      ClearRegionRegion(l->back->VisibleRegion, show_region);
    }
    else
    {
      /*
       * This is the front most layer.
       */
      OrRegionRegion(l->shape, show_region);
    }
    /*
     * Visit all layers behind this layer until my parent comes
     * I also visit my parent.
     */
    while ((NULL != _l) && !IS_EMPTYREGION(show_region))
    {
      _ShowPartsOfLayer(_l, show_region, _l->rp->BitMap);
      /*
       * The part that this layer is hiding I cannot make 
       * visible on the layers behind it. Therefore I
       * have to take it out.
       */
      ClearRegionRegion(_l->shape, show_region);
      
      if (_l == l->parent)
        break;
        
      _l = _l->back;
    }
  }
  
  cr = l->ClipRect;
  while (cr)
  {
    if (cr->BitMap)
      FreeBitMap(cr->BitMap);
    _cr =cr->Next;
    FreeMem(cr, sizeof(struct ClipRect));
    cr = _cr;
  }
  
  if (l->front)
    l->front->back = l->back;
  else
    l->LayerInfo->top_layer = l->back;
  
  if (l->back)
    l->back->front = l->front;
  
  FreeMem(l, sizeof(struct Layer));
  
  AROS_LIBFUNC_EXIT
} /* DeleteLayer */
