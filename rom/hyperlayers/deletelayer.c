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

  struct Layer * _l, * lparent;
  struct ClipRect * cr, *_cr;
  /*
   * all children must have been destroyed before.
   */
  LockLayers(l->LayerInfo);
  
  if (l != GetFirstFamilyMember(l))
  {
    kprintf("%s: There are still children around! Cannot destroy layer %p\n",
            __FUNCTION__,
            l);
    UnlockLayers(l->LayerInfo);
kprintf("%s %d\n",__FUNCTION__,__LINE__);  
    return FALSE;
  }
kprintf("%s %d\n",__FUNCTION__,__LINE__);  
    
  if (IS_VISIBLE(l))
  { 
    struct Region * r = NewRegion();
kprintf("%s %d\n",__FUNCTION__,__LINE__);  

    OrRegionRegion(l->VisibleRegion, r);
    _l = l->back;
    lparent = l->parent;

    /*
     * Visit all layers behind this layer until my parent comes
     * I also visit my parent. If my parent is invisible I must
     * go further to the parent of that parent etc.
     */
    while (1)
    {
      ClearRegion(_l->VisibleRegion);
      if (IS_VISIBLE(_l)  && DO_OVERLAP(&r->bounds, &_l->shape->bounds))
        _ShowPartsOfLayer(_l, r);
      else
        OrRegionRegion(r,_l->VisibleRegion);

      if (IS_VISIBLE(_l) || IS_ROOTLAYER(_l))
        AndRegionRegion(_l->VisibleRegion, l->shape);
      
      if (_l == lparent)
      {
        if (IS_VISIBLE(_l) || (NULL == lparent->parent))
          break;
        else
          lparent = lparent->parent;
      }
      /*
       * The part that this layer is hiding I cannot make 
       * visible on the layers behind it. Therefore I
       * have to take it out.
       */
      if (IS_VISIBLE(_l))
        ClearRegionRegion(_l->shape, r);

      _l = _l->back;
    }

    DisposeRegion(r);

    if (!IS_EMPTYREGION(l->shape))
    {
kprintf("lparent: %p, l->parent: %p\n",lparent,l->parent);
      if (lparent && 
          (IS_SIMPLEREFRESH(lparent) || (lparent==l->LayerInfo->check_lp)))
        _BackFillRegion(lparent, l->shape, FALSE);
    }
    else
      kprintf("NOTHING TO CLEAR!\n");
  }
  
  /*
   * Free all cliprects.
   */
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
  
  UnlockLayers(l->LayerInfo);

  DisposeRegion(l->DamageList);
  DisposeRegion(l->VisibleRegion);
  DisposeRegion(l->shape);

  FreeMem(l, sizeof(struct Layer));

  return TRUE;
  
  AROS_LIBFUNC_EXIT
} /* DeleteLayer */
