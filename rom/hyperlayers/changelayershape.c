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
#include <proto/utility.h>
#include <utility/hooks.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH3(struct Region *, ChangeLayerShape,

/*  SYNOPSIS */
	AROS_LHA(struct Layer  *, l          , A0),
	AROS_LHA(struct Region *, newshape   , A1),
	AROS_LHA(struct Hook   *, callback   , A2),
/*  LOCATION */
	struct LayersBase *, LayersBase, 41, Layers)

/*  FUNCTION
       Changes the shape of the layer on the fly.
       When the shape of a layer is changed the current pixel content
       is copied into its ClipRects so no information is lost.
       The user can provide a callback hook that will be 
       called when the current layer's information is all backed up
       in ClipRects. The signature of the callback should look as follows:
           AROS_UFC4(struct Region *, callback,
               AROS_UFCA(struct Hook   *          , hook       , A0),
               AROS_UFCA(struct Layer  *          , l          , A2),
               AROS_UFCA(struct ScaleLayerParam * , arg        , A1));


    INPUTS
       L        - pointer to layer 
       newshape - pointer to a region that comprises the new shape
                  of the layer. May be NULL if callback is provided. 
       callback - pointer to a callback hook. May be NULL if newshape
                  is given.

    RESULT
       Pointer to the previously installed region.
  
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

  struct Region * returnshape = NULL;

  LockLayers(l->LayerInfo);

  {
    struct Region r, cutoldshape, rtmp, cutnewshape;
    struct Layer * lparent, * _l, * lfirst;
    int behind_l = FALSE;
    r.RegionRectangle = NULL;
    rtmp.RegionRectangle = NULL;
    cutoldshape.RegionRectangle = NULL;
    cutnewshape.RegionRectangle = NULL;

    if (IS_VISIBLE(l))
    {
      /*
       * Backup everything that is visible right now into 
       * cliprects.
       */
      _SetRegion(l->shape, &cutoldshape);
      AndRegionRegion(l->parent->shape, &cutoldshape);
      _BackupPartsOfLayer(l, &cutoldshape, 0, TRUE, LayersBase);
    }

    /*
     * Get the current layer's shape as the paramter to return
     * from this function.
     */
    returnshape = l->shaperegion;

    if (NULL != callback)
    {
      struct ChangeLayerShapeMsg clsm;
      clsm.newshape = newshape;
      clsm.cliprect = l->ClipRect;
      clsm.shape    = l->shape;
      /*
       * call the callback to the user to give me a new shape
       * The user can manipulate the cliprects of the layer
       * l and can have a look at the current shape.
       */
      l->shaperegion = CallHookPkt(callback, l, &clsm);
    }
    else
    {
      /*
       * The user passed me the new shape but no callback.
       */
      l->shaperegion = newshape;
    }
    
    ClearRegion(l->shape);
    /*
     * At this point l->shaperegion holds the layer that is to be 
     * installed. Let's cut it down to the actually visible part.
     */
    
    l->shape = NewRectRegion(0,0,l->Width-1,l->Height-1);
    
    if (l->shaperegion)
      AndRegionRegion(l->shaperegion, l->shape);

    _TranslateRect(&l->shape->bounds, l->bounds.MinX, l->bounds.MinY);      
      
    _SetRegion(l->shape, &cutnewshape);
    AndRegionRegion(l->parent->shape, &cutnewshape);


    if (IS_VISIBLE(l))
    {
      /*
       * Let me backup parts of the layers that are behind the
       * layer l and in front of it.
       */
      lparent = l->parent;
      lfirst = GetFirstFamilyMember(l);
      _l = lfirst;
    
      while (1)
      {
        if ((l != _l) && IS_VISIBLE(_l) && DO_OVERLAP(&cutnewshape.bounds, &l->shape->bounds))
          _BackupPartsOfLayer(_l, &cutnewshape, 0, FALSE, LayersBase);
        else
          ClearRegionRegion(&cutnewshape, _l->VisibleRegion);
         
        if (_l == lparent)
        {
          if (IS_VISIBLE(_l) || (NULL == lparent->parent))
            break;
          else
            lparent = lparent->parent;
        }
      
        _l = _l->back;
      } /* while (1) */

      
      if (lfirst->front)
      {
        _SetRegion(lfirst->front->VisibleRegion, &r);
        _SetRegion(lfirst->front->shape, &rtmp);
        AndRegionRegion(lfirst->front->parent->shape, &rtmp);
        ClearRegionRegion(&rtmp, &r);
      }
      else
        _SetRegion(l->LayerInfo->check_lp->shape, &r);

      /*
       * Make the new layer and its family visible
       * Since the parent might have become bigger more
       * of the children might become visible...
       */    
      _l = lfirst;
      lparent = l->parent;
      
      while (1)
      {
        if (IS_VISIBLE(_l) && 
            (DO_OVERLAP(&cutnewshape.bounds, &_l->shape->bounds) ||
             DO_OVERLAP(&cutoldshape.bounds, &_l->shape->bounds)))
        {
          ClearRegion(l->VisibleRegion);
          _ShowPartsOfLayer(l, &r, LayersBase);
        }
        else
          _SetRegion(&r, _l->VisibleRegion);
          

        if ((TRUE == behind_l) && (IS_VISIBLE(_l) || IS_ROOTLAYER(_l)))
          AndRegionRegion(_l->VisibleRegion, &cutoldshape);

        if (_l == lparent)
        {
          if (IS_VISIBLE(_l) || (NULL == lparent->parent))
            break;
          else
            lparent = lparent->parent;
        }
          
        if (IS_VISIBLE(_l))
        {
          _SetRegion(_l->shape, &rtmp);
          AndRegionRegion(_l->parent->shape, &rtmp);
          ClearRegionRegion(&rtmp, &r);
        }

        if (l == _l)
          behind_l = TRUE;
      
        _l = _l->back;
      } /* while(1) */

      ClearRegion(&rtmp);
      ClearRegion(&r);

      if (!IS_EMPTYREGION(&cutoldshape))
      {
        if (lparent &&
            (IS_SIMPLEREFRESH(lparent) || IS_ROOTLAYER(lparent)))
              _BackFillRegion(l->parent, &cutoldshape, TRUE);
      }

      ClearRegion(&cutoldshape);

    } /* if (IS_VISIBLE(l)) */
    
    ClearRegion(&cutnewshape);
  
  }
  
  UnlockLayers(l->LayerInfo);
  
  return returnshape;

  AROS_LIBFUNC_EXIT
} /* ChangeLayerShape */
