#include <exec/types.h>
#include <graphics/clip.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include "basicfuncs.h"

int _MoveLayerBehind(struct Layer *l,
                     struct Layer *lfront,
                     struct LayersBase * LayersBase)
{
  struct Layer * lbackold, *_l, *first;
  struct Region * hide = NewRegion(), * show = NewRegion();

  first = GetFirstFamilyMember(l);

  lbackold = l->back;

  /*
   * Take the family out of its old place.
   */
  if (NULL != first->front)
    first->front->back = lbackold;
  else
    l->LayerInfo->top_layer = lbackold;
    
  lbackold->front = first->front;

  _l = lbackold;
  /*
   * Collect the additional parts that have to be hidden
   * on the layers that are to be moved. I need only
   * collect those parts of layers that have the same
   * parent as the layer l. Stop when I found lfront.
   */
  while (1)
  {
    /*
     * Must not add my own shape to the hide region...
     */
    if (_l == l)
      break;
    
    /*
     * Must add EVERY shape since it will be subtracted from
     * the visible region
     */
    if (l->parent == _l->parent)
    {
kprintf("Adding a part!\n");
      OrRegionRegion(_l->shape, hide);
    }

    if (_l == lfront)
      break;
      
    _l = _l->back;
  }

  OrRegionRegion(first->VisibleRegion, show);

  /*
   * First back up the family ... this is like moving them behind the
   * other layers.
   */
  
  _l = first;
  while (1)
  {
    if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->shape->bounds, &hide->bounds))
      _BackupPartsOfLayer(_l, hide, 0, FALSE);
    else
      ClearRegionRegion(hide, _l->VisibleRegion);
    
    if (_l == l)
      break;
  
    _l = _l->back;
  }
  DisposeRegion(hide);

  /*
   * Show the layers that appear now.
   * Start with the layer that is behind the layer l.
   */
  _l = lbackold;

  while (1)
  {
    ClearRegion(_l->VisibleRegion);
    if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->shape->bounds, &show->bounds))
      _ShowPartsOfLayer(_l, show);
    else
      OrRegionRegion(show, _l->VisibleRegion);
      
    if (_l == lfront)
      break;

    ClearRegionRegion(_l->shape, show);
    
    _l = _l->back; 
  }

  DisposeRegion(show);

  /*
   * Link the family into its new place.
   */
  if (lfront->back)
    lfront->back->front = l;
  l->back = lfront->back;

  first->front = lfront;
  lfront->back = first;

  return TRUE;
}

int _MoveLayerToFront(struct Layer * l,
                      struct Layer * lbehind,
                      struct LayersBase * LayersBase)
{
  struct Layer * lfront, * first, * _l;
  struct Region * r = NewRegion();
  
  first = GetFirstFamilyMember(l);

  lfront = lbehind->front;

  _l = lbehind;
  /*
   * Unlink the family of layers from its old place.
   */
  first->front->back = l->back;
  l->back->front = first->front;

  /*
   * I need exactly that layers visible region later on.
   */

  OrRegionRegion(lbehind->VisibleRegion,r);
    
  /*
   * Now I have to move the layer family in front of layer lbehind.
   * Nothing changes for the layers in front of layer lbehind, but on
   * the layers behind (including first->front) I must back up some of 
   * their parts that the new family might be hiding no.
   * Once I step on the layer that was behind the layer l
   * I can stop because those layers are already hidden well
   * enough.
   */
  do
  {
    if (IS_VISIBLE(_l) && DO_OVERLAP(&l->shape->bounds, &_l->shape->bounds))
      _BackupPartsOfLayer(_l, l->shape, 0, FALSE);
    else
      ClearRegionRegion(l->shape, _l->VisibleRegion);

    _l = _l->back;
  }
  while (_l != l->back /* this is l->back now since the family has
                          been unlinked already!*/ );

  /*
   * Now I must make the layer family of l visible 
   * (lfirst to and including l)
   */
  _l = first;
    
  while (1)
  {
    ClearRegion(_l->VisibleRegion);
//kprintf("\t\t%s: Showing parts of layer %p\n",__FUNCTION__,_l);

    if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->shape->bounds, &r->bounds))
      _ShowPartsOfLayer(_l, r);
    else
      OrRegionRegion(r, _l->VisibleRegion);      
      

    if (_l == l)
      break;

    ClearRegionRegion(_l->shape, r);

    _l = _l->back;
  }

  DisposeRegion(r);

  /*
   * Link the family into its new place.
   * First the frontmost kid and then l.
   */
  if (NULL != lfront)
    lfront->back = first;
  else
    l->LayerInfo->top_layer = first;
    
  first->front = lfront;

  l->back = lbehind;
  lbehind->front = l;

  return TRUE;
}