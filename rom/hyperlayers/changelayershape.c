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
	AROS_LH3(struct Region *, ChangeLayerShape,

/*  SYNOPSIS */
	AROS_LHA(struct Layer  *, l          , A0),
	AROS_LHA(struct Region *, newshape   , A1),
	AROS_LHA(void          *, callback   , A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 41, Layers)

/*  FUNCTION
       Changes the shape of the layer. 
       When the shape of a layer is changed the pixel content
       is copied into its ClipRects so no information is lost.

    INPUTS
       L        - pointer to layer 
       newshape - pointer to a region that comprises the new shape
                  of the layer. 

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

  if (!(NULL == newshape || NULL == callback))
  {
    struct Region r, cutoldshape, rtmp, cutnewshape;
    struct Layer * lparent, *_l, *lfirst;
    int behind_l = FALSE;
    r.RegionRectangle = NULL;
    rtmp.RegionRectangle = NULL;
    cutoldshape.RegionRectangle = NULL;
    cutnewshape.RegionRectangle = NULL;


    /*
     * Backup everything that is visible right now into 
     * cliprects.
     */
    _SetRegion(l->shape, &cutoldshape);
    AndRegionRegion(l->parent->shape, &cutoldshape);
    _BackupPartsOfLayer(l, &cutoldshape, 0, TRUE, LayersBase);

    /*
     * Get the current layer's shape as the paramter to return
     * from this function.
     */
    returnshape = l->shaperegion;

    if (NULL != callback)
    {
      /*
       * call the callback to the user to give me a new shape
       * give me a shape.
       */
      l->shaperegion = AROS_UFC4(struct Region *, callback,
                        AROS_UFCA(struct Region *   , newshape  , A0),
                        AROS_UFCA(struct Layer  *  , l          , A1),
                        AROS_UFCA(struct ClipRect *, l->ClipRect, A2),
                        AROS_UFCA(struct Region *  , l->shape   , A3));
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
      
      
    _SetRegion(l->shape, &cutnewshape);
    AndRegionRegion(l->parent->shape, &cutnewshape);


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
    }

      
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
    }

    ClearRegion(&rtmp);
    ClearRegion(&cutnewshape);
    ClearRegion(&r);
  
    if (!IS_EMPTYREGION(&cutoldshape))
    {
      if (lparent &&
          (IS_SIMPLEREFRESH(lparent) || IS_ROOTLAYER(lparent)))
            _BackFillRegion(l->parent, &cutoldshape, TRUE);
    }
  
    ClearRegion(&cutoldshape);
  }
  UnlockLayers(l->LayerInfo);
  
  return returnshape;

  AROS_LIBFUNC_EXIT
} /* ChangeLayerShape */
