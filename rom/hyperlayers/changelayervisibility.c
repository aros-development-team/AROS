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
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(LONG, ChangeLayerVisibility,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l      , A0),
	AROS_LHA(int           , visible, D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 39, Layers)

/*  FUNCTION
       Makes the given layer visible or invisible.
       If it is a simple refresh layer it will loose all its
       cliprects and therefore rendering will go into the void.

    INPUTS
       L       - pointer to layer 
       visible - TRUE or FALSE

    RESULT
       TRUE  - layer was changed to new state
       FALSE - layer could not be changed to new state
  
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer * _l, * lparent;
  struct Region rtmp, r;
  rtmp.RegionRectangle = NULL;
  r.RegionRectangle = NULL;

kprintf("%s called!\n",__FUNCTION__);
  if (l->visible == visible)
    return TRUE;

kprintf("%s called 2nd!\n",__FUNCTION__);

  LockLayers(l->LayerInfo);

  l->visible = visible;
  
  _SetRegion(l->shape, &rtmp);
  AndRegionRegion(l->parent->shape, &rtmp);
  
  if (TRUE == visible)
  {
    /*
     * Make the layer visible
     * Back up all layers behind this layer.
     */
    lparent = l->parent;
    _l = l->back;
    while (1)
    {
      if (IS_VISIBLE(_l) && DO_OVERLAP(&rtmp.bounds, &_l->shape->bounds))
        _BackupPartsOfLayer(_l, &rtmp, 0, FALSE, LayersBase);
      
      if (_l == lparent)
      {
        if (IS_VISIBLE(_l) || (NULL == lparent->parent))
          break;
        else
          lparent = lparent->parent;
      }
      _l = _l->back;
    }
    
    /*
     * For the layer to become visible I must recalculate its
     * visible area.
     */
    ClearRegion(l->VisibleRegion);
    if (l->front)
    {
      _SetRegion(l->front->VisibleRegion, &r);
      _SetRegion(l->front->shape, &rtmp);
      AndRegionRegion(l->front->parent->shape, &rtmp);
      ClearRegionRegion(&rtmp, &r);
    }
    else
    {
      /*
       * This is the frontmost layer...
       */
      _SetRegion(l->LayerInfo->check_lp->shape, &r);
    }
    /*
     * Let me show the layer in its full beauty...
     */
    _ShowPartsOfLayer(l, &r, LayersBase);
  
    if (IS_SIMPLEREFRESH(l))
    {
      /*
       * Add the whole visible area of the layer to the
       * damage list since for those kind of layers
       * nothing was backed up.
       */
      _SetRegion(l->shape, l->DamageList);
      AndRegionRegion(l->VisibleRegion, l->DamageList);
      /*
       * Since the Damagelist is relative to the layer I have to make
       * some adjustments to the coordinates here.
       */
      _TranslateRect(&l->DamageList->bounds,-l->bounds.MinX,-l->bounds.MinY);
      l->Flags |= LAYERREFRESH;
    }
  }
  else
  {
    struct Region r, clearr;
    r.RegionRectangle = NULL; // min. initialization
    _SetRegion(l->VisibleRegion, &r);
    
    clearr.RegionRectangle = NULL; // min. initialization
    _SetRegion(l->shape, &clearr);
    /*
     * Make the layer invisible
     */
    _BackupPartsOfLayer(l, l->shape, 0, FALSE, LayersBase);
    
    /*
     * Walk through all the layers behind this layer and
     * make them (more) visible...
     */
    lparent = l->parent;
    _l = l->back;
    while (1)
    {
      if (IS_VISIBLE(_l) && DO_OVERLAP(&l->shape->bounds, &_l->shape->bounds))
      {
        ClearRegion(_l->VisibleRegion);
        _ShowPartsOfLayer(_l, &r, LayersBase);
      }
      else
        _SetRegion(&r, _l->VisibleRegion);

      if (IS_VISIBLE(_l) || IS_ROOTLAYER(_l))
        AndRegionRegion(_l->VisibleRegion, &clearr);

      if (_l == lparent)
      {
        if (IS_VISIBLE(_l) || (NULL == lparent->parent))
          break;
        else
          lparent = lparent->parent;
      }
      
      /*
       * Take the shape of the current layer out of
       * the visible region that will be applied to the
       * layer behind this one.
       */
      if (IS_VISIBLE(_l))
      {
        _SetRegion(_l->shape, &rtmp);
        AndRegionRegion(_l->parent->shape, &rtmp);
        ClearRegionRegion(&rtmp, &r);
      }
      _l = _l->back;
    }
    ClearRegion(&r);
  
    if (!IS_EMPTYREGION(&clearr))
    {
      if (lparent &&
          (IS_SIMPLEREFRESH(lparent) || IS_ROOTLAYER(lparent)))
        _BackFillRegion(lparent, &clearr, FALSE);
    }

    l->Flags &= ~LAYERREFRESH;
    ClearRegion(l->DamageList);
    ClearRegion(&clearr);
  }

  ClearRegion(&rtmp);
  UnlockLayers(l->LayerInfo);

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* ChangeLayerVisibility */
