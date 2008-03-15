/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include "layers_intern.h"
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

  struct Layer * _l, * lparent;
  struct Region r;
  InitRegion(&r);

  if (l->visible == visible)
    return TRUE;

  LockLayers(l->LayerInfo);

  l->visible = visible;
  
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
      if (IS_VISIBLE(_l) && DO_OVERLAP(&l->visibleshape->bounds, &_l->shape->bounds))
        _BackupPartsOfLayer(_l, l->visibleshape, 0, FALSE, LayersBase);
      else
        ClearRegionRegion(l->visibleshape, _l->VisibleRegion);
        
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
     * visible area. Search the first visible layer in front of
     * it and used that one's VisbleRegion minus its visibleshape.
     */
    ClearRegion(l->VisibleRegion);
    _l = l->front;
    while (1)
    {
      if (NULL == _l)
      {
        /*
         * It's like the top layer since all others are invisible
         */
        SetRegion(l->LayerInfo->check_lp->shape, &r);
        break;
      }

      if (IS_VISIBLE(_l))
      {
        SetRegion(_l->VisibleRegion, &r);
        ClearRegionRegion(_l->visibleshape, &r);
        break;
      }
        
      _l = _l->front;
        
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
      SetRegion(l->shape, l->DamageList);
      AndRegionRegion(l->VisibleRegion, l->DamageList);
      /*
       * Since the Damagelist is relative to the layer I have to make
       * some adjustments to the coordinates here.
       */
      _TranslateRect(&l->DamageList->bounds,
                     -l->bounds.MinX,
                     -l->bounds.MinY);
      l->Flags |= LAYERREFRESH;
    }
  }
  else
  {
    /*
     * Make the layer invisible
     */
    struct Region clearr;
    InitRegion(&clearr);

    l->Flags &= ~LAYERREFRESH;
    ClearRegion(l->DamageList);

    SetRegion(l->VisibleRegion, &r);

    SetRegion(l->visibleshape, &clearr);
    _BackupPartsOfLayer(l, &clearr, 0, FALSE, LayersBase);

    /*
     * Walk through all the layers behind this layer and
     * make them (more) visible...
     */
    lparent = l->parent;
    _l = l->back;
    while (1)
    {
      if (IS_VISIBLE(_l) && DO_OVERLAP(&l->visibleshape->bounds, &_l->visibleshape->bounds))
      {
        ClearRegion(_l->VisibleRegion);
        _ShowPartsOfLayer(_l, &r, LayersBase);
      }
      else
        SetRegion(&r, _l->VisibleRegion);

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
        ClearRegionRegion(_l->visibleshape, &r);

      _l = _l->back;
    }

    if (!IS_EMPTYREGION(&clearr))
    {
      if (lparent &&
          (IS_SIMPLEREFRESH(lparent) || IS_ROOTLAYER(lparent)))
        _BackFillRegion(lparent, &clearr, FALSE, LayersBase);
    }


    ClearRegion(&clearr);
  }

  ClearRegion(&r);
  UnlockLayers(l->LayerInfo);

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* ChangeLayerVisibility */
