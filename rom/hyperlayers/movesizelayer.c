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

  struct Layer * first, *_l;
  struct Region * newshape = NewRegion(), * oldshape, * r, *br;
  struct Rectangle rect;
  
  LockLayers(l->LayerInfo);
#warning Write a function to copy a region.
  /*
   * First Create the new region of the layer:
   * adjust its size and position.
   */
  OrRegionRegion(l->shape,newshape);

  if (dw > 0)
  {
    rect.MinX = l->shape->bounds.MaxX+1;
    rect.MinY = l->shape->bounds.MinY;
    rect.MaxX = rect.MinX + dw - 1;
    rect.MaxY = l->shape->bounds.MaxY+dh;
    OrRectRegion(newshape, &rect); 
  }
  else
  if (dw < 0)
  {
    rect.MinX = l->shape->bounds.MinX;
    rect.MinY = l->shape->bounds.MinY;
    rect.MaxX = l->shape->bounds.MaxX+dw;
    rect.MaxY = l->shape->bounds.MaxY;
    AndRectRegion(newshape, &rect); 
  }

  if (dh > 0)
  {
    rect.MinX = l->shape->bounds.MinX;
    rect.MinY = l->shape->bounds.MaxY + 1;
    rect.MaxX = l->shape->bounds.MaxX+dw;
    rect.MaxY = rect.MinY + dh - 1;
    OrRectRegion(newshape, &rect); 
  }
  else
  if (dh < 0)
  {
    rect.MinX = l->shape->bounds.MinX;
    rect.MinY = l->shape->bounds.MinY;
    rect.MaxX = l->shape->bounds.MaxX;
    rect.MaxY = l->shape->bounds.MaxY+dh;
    AndRectRegion(newshape, &rect); 
  }
  
  newshape->bounds.MinX += dx; 
  newshape->bounds.MinY += dy;
  newshape->bounds.MaxX += dx; 
  newshape->bounds.MaxY += dy;

  first = _FindFirstFamilyMember(l);
kprintf("%s called for layer %p!\n",__FUNCTION__);
  
  /*
   * First back up parts of layers that are behind the layer
   * family. Only need to do this if layer is moving or
   * getting bigger in size.
   */

kprintf("\t\t%s: Backing up parts of layers that are behind the layer!\n",
        __FUNCTION__);
  if (0 != dx || 0 != dy || dw > 0 || dh > 0)
  {
    _l = l->back;
    while (1)
    {
      if (IS_VISIBLE(_l))
        _BackupPartsOfLayer(_l, newshape, 0, FALSE);
      if (_l == l->parent)
        break;
      
      _l = _l->back;      
    }  
  }

  /*
   * Now I need to move the layer and all its familiy to the new 
   * location.
   */
  _l = first;
  r = NewRegion();
  br = NewRegion();
  OrRegionRegion(first->VisibleRegion, r);
  oldshape = l->shape;
  l->shape = newshape;
 
  while (1)
  {
    struct ClipRect * cr;
    
    ClearRegion(br);
    OrRegionRegion(_l->shape, br);
    AndRegionRegion(_l->parent->shape, br);
kprintf("\t\t%s: BACKING up parts of THE LAYER TO BE MOVED!\n",
        __FUNCTION__);
    
    if (_l == l)
      l->shape = oldshape;    
    
    if (IS_VISIBLE(_l))
      _BackupPartsOfLayer(_l, br, dx, TRUE);

    /*
     * Effectively move the layer...
     */
    _l->bounds.MinX += dx;
    _l->bounds.MinY += dy;
    _l->bounds.MaxX += dx;
    _l->bounds.MaxY += dy;

    /*
     * and also its cliprects.
     */
    cr = _l->ClipRect;
    while (cr)
    {
      cr->bounds.MinX += dx;
      cr->bounds.MinY += dy;
      cr->bounds.MaxX += dx;
      cr->bounds.MaxY += dy;
      cr = cr->Next;
    }
    
    if (_l == l)
    {
kprintf("\t\t%s: SHOWING parts of THE LAYER TO BE MOVED!\n",
        __FUNCTION__);
      l->shape = newshape;
      if (IS_VISIBLE(l))
      {
        ClearRegion(l->VisibleRegion);
        _ShowPartsOfLayer(l, r);
      }
      break;
    }

    _l->shape->bounds.MinX += dx;
    _l->shape->bounds.MinY += dy;
    _l->shape->bounds.MaxX += dx;
    _l->shape->bounds.MaxY += dy;

    if (IS_VISIBLE(l))
    {
kprintf("\t\t%s: SHOWING parts of THE LAYER TO BE MOVED!\n",
        __FUNCTION__);
      _ShowPartsOfLayer(_l, r);
      ClearRegionRegion(_l->shape, r);
    }
      
    _l = _l->back;
  }
  DisposeRegion(br);
  DisposeRegion(r);
    
   
  /*
   * Now make those parts of the layers after l up to and including
   * its parent visible.
   * Only need to do this if layer was moved or became smaller.
   */
  if (0 != dx || 0 != dy || dw < 0 || dh < 0)
  {
    struct Region * r = NewRegion();
    OrRegionRegion(l->VisibleRegion, r);
    ClearRegionRegion(l->shape, r);
    _l = l->back;
    
    while (1)
    {
kprintf("\t\t%s: SHOWING parts of the layers behind the layer to be moved!\n",
        __FUNCTION__);
      if (IS_VISIBLE(_l))
      {
        ClearRegion(_l->VisibleRegion);
        _ShowPartsOfLayer(_l, r);
        AndRegionRegion(_l->VisibleRegion, oldshape);
      }
      
      if (_l == l->parent)
        break;
      
      if (IS_VISIBLE(_l))    
        ClearRegionRegion(_l->shape, r);
      
      _l = _l->back;
    }
    
    DisposeRegion(r);

    /*  
     * Now I need to clear the old layer at its previous place..
     * But I may only clear those parts where no layer has become
     * visible in the meantime.
     */
    if (!IS_EMPTYREGION(oldshape))
      _BackFillRegion(l->parent, oldshape);
  }
  
  DisposeRegion(oldshape);
  
  UnlockLayers(l->LayerInfo);
  
  return TRUE;
   
  AROS_LIBFUNC_EXIT
} /* MoveSizeLayer */
