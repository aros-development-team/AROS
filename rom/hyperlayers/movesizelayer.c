/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include "layers_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH5(LONG, MoveSizeLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l , A0),
	AROS_LHA(LONG          , dx, D0),
	AROS_LHA(LONG          , dy, D1),
	AROS_LHA(LONG          , dw, D2),
	AROS_LHA(LONG          , dh, D3),

/*  LOCATION */
	struct LayersBase *, LayersBase, 30, Layers)

/*  FUNCTION
        Moves and resizes the layer in one step. Collects damage lists
        for those layers that become visible and are simple layers.
        If the layer to be moved is becoming larger the additional
        areas are added to a damagelist if it is a non-superbitmap
        layer. Refresh is also triggered for this layer.

    INPUTS
        l     - pointer to layer to be moved
        dx    - delta to add to current x position
        dy    - delta to add to current y position
        dw    - delta to add to current width
        dw    - delta to add to current height

    RESULT
        result - TRUE everyting went alright
                 FALSE an error occurred (out of memory)

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

  struct Layer * first, *_l, *lparent;
  struct Region * newshape = NewRegion(), * oldshape, r, rtmp, cutnewshape;
  struct Rectangle rectw, recth;

  rtmp.RegionRectangle = NULL; // min. initialization
  r.RegionRectangle = NULL; // min. initialization
  cutnewshape.RegionRectangle = NULL;
  
  LockLayers(l->LayerInfo);

#warning Write a function to copy a region.
  /*
   * First Create the new region of the layer:
   * adjust its size and position.
   */
  OrRegionRegion(l->shape,newshape);

  if (dw > 0)
  {
    rectw.MinX = l->shape->bounds.MaxX+1;
    rectw.MinY = l->shape->bounds.MinY;
    rectw.MaxX = rectw.MinX + dw - 1;
    rectw.MaxY = l->shape->bounds.MaxY+dh;
    OrRectRegion(newshape, &rectw); 
  }
  else
  if (dw < 0)
  {
    rectw.MinX = l->shape->bounds.MinX;
    rectw.MinY = l->shape->bounds.MinY;
    rectw.MaxX = l->shape->bounds.MaxX+dw;
    rectw.MaxY = l->shape->bounds.MaxY+dh;
    AndRectRegion(newshape, &rectw); 
  }

  if (dh > 0)
  {
    recth.MinX = l->shape->bounds.MinX;
    recth.MinY = l->shape->bounds.MaxY + 1;
    recth.MaxX = l->shape->bounds.MaxX+dw;
    recth.MaxY = recth.MinY + dh - 1;
    OrRectRegion(newshape, &recth); 
  }
  else
  if (dh < 0)
  {
    recth.MinX = l->shape->bounds.MinX;
    recth.MinY = l->shape->bounds.MinY;
    recth.MaxX = l->shape->bounds.MaxX+dw;
    recth.MaxY = l->shape->bounds.MaxY+dh;
    AndRectRegion(newshape, &recth); 
  }

  if (dx || dy)
  {
    _TranslateRect(&newshape->bounds, dx, dy);
    _TranslateRect(&rectw, dx, dy);
    _TranslateRect(&recth, dx, dy);
  }

  _SetRegion(newshape, &cutnewshape);
  AndRegionRegion(l->parent->shape, &cutnewshape);

  first = GetFirstFamilyMember(l);
  /*
   * Must make a copy of the VisibleRegion of l here
   * and NOT later!
   */
  _SetRegion(first->VisibleRegion, &r);
//kprintf("%s called for layer %p, first = %p!\n",__FUNCTION__,l,first);
  
  /*
   * First back up parts of layers that are behind the layer
   * family. Only need to do this if layer is moving or
   * getting bigger in size. Only need to visit those layers
   * that overlap with the new shape of the layer.
   */

        
  lparent = l->parent;
  _l = l->back;

#if 0
kprintf("\t\t%s: Backing up parts of layers that are behind the layer!\n",
        __FUNCTION__);
#endif
  while (1)
  {
    if (IS_VISIBLE(_l) && DO_OVERLAP(&cutnewshape.bounds, &_l->shape->bounds))
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

  /*
   * Now I need to move the layer and all its familiy to the new 
   * location.
   */
  _l = first;
  oldshape = l->shape;
 
  while (1)
  {
    struct ClipRect * cr;

#if 0
kprintf("\t\t%s: BACKING up parts of THE LAYER TO BE MOVED!\n",
        __FUNCTION__);
#endif    

    if (IS_VISIBLE(_l))
    {
      ClearRegion(_l->VisibleRegion);
      _BackupPartsOfLayer(_l, _l->shape, dx, TRUE, LayersBase);
    }
    /*
     * Effectively move the layer...
     */
     
    _TranslateRect(&_l->bounds, dx, dy);

    /*
     * ...and also its cliprects.
     */
    cr = _l->ClipRect;
    while (cr)
    {
      _TranslateRect(&cr->bounds, dx, dy);
      cr = cr->Next;
    }

    cr = _l->_cliprects;
    while (cr)
    {
      _TranslateRect(&cr->bounds, dx, dy);
      cr = cr->Next;
    }
    
    if (_l == l)
    {
      /*
       * Resize the bounds!
       */
      l->bounds.MaxX += dw;
      l->bounds.MaxY += dh;
      l->Width  += dw;
      l->Height += dh;
      break;
    }
    
    _TranslateRect(&_l->shape->bounds, dx, dy);
      
    _l = _l->back;
  }

  /* 
   * Now make them visible again.
   */
  _l = first;
  l->shape = newshape;
  while (1)
  {
    if (_l == l)
    {
      if (IS_VISIBLE(l))
      {
#if 0
kprintf("\t\t%s: SHOWING parts of THE LAYER TO BE MOVED!\n",
        __FUNCTION__);
#endif
        ClearRegion(l->VisibleRegion);
        _ShowPartsOfLayer(l, &r, LayersBase);
      }
      break;
    }

    if (IS_VISIBLE(_l))
    {
#if 0
kprintf("\t\t%s: SHOWING parts of THE LAYER TO BE MOVED (children)!\n",
        __FUNCTION__);
#endif    
      ClearRegion(l->VisibleRegion);
      _ShowPartsOfLayer(_l, &r, LayersBase);
      
      _SetRegion(_l->shape, &rtmp);
      AndRegionRegion(_l->parent->shape, &rtmp);
      ClearRegionRegion(&rtmp, &r);
      
    }
      
    _l = _l->back;
  }


  /*
   * Now make those parts of the layers after l up to and including
   * its parent visible.
   */
  _SetRegion(l->VisibleRegion, &r);
  ClearRegionRegion(&cutnewshape, &r);
  _l = l->back;
  lparent = l->parent;
    
  while (1)
  {
#if 0
kprintf("\t\t%s: SHOWING parts of the layers behind the layer to be moved!\n",
        __FUNCTION__);
#endif
    if (IS_VISIBLE(_l) && 
         (DO_OVERLAP(&cutnewshape.bounds, &_l->shape->bounds) ||
          DO_OVERLAP(   &oldshape->bounds, &_l->shape->bounds) ))
    {
      ClearRegion(_l->VisibleRegion);
      _ShowPartsOfLayer(_l, &r, LayersBase);
    }
    else
      _SetRegion(&r, _l->VisibleRegion);

    if (IS_VISIBLE(_l) || IS_ROOTLAYER(_l))
      AndRegionRegion(_l->VisibleRegion, oldshape);

#if 0
    if (IS_ROOTLAYER(_l))
      kprintf("root reached! %p\n",_l);
#endif
      
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
    _l = _l->back;
  }

  ClearRegion(&cutnewshape);
  ClearRegion(&rtmp);
    
  /*  
   * Now I need to clear the old layer at its previous place..
   * But I may only clear those parts where no layer has become
   * visible in the meantime.
   */
  if (!IS_EMPTYREGION(oldshape))
  {
    if (lparent &&
        (IS_SIMPLEREFRESH(lparent) || IS_ROOTLAYER(lparent)))
      _BackFillRegion(l->parent, oldshape, TRUE);
  }

  DisposeRegion(oldshape);

  /*
   * If the size of the layer became larger clear the
   * new area where it is visible.
   */
  if ((dw > 0 || dh > 0) && !IS_SUPERREFRESH(l))
  {
    ClearRegion(&r);
    if (dw > 0)
      OrRectRegion(&r, &rectw);
    if (dh > 0)
      OrRectRegion(&r, &recth);
    _BackFillRegion(l, &r, TRUE);
  }

  ClearRegion(&r);

  UnlockLayers(l->LayerInfo);

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* MoveSizeLayer */
