/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <graphics/clip.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include "basicfuncs.h"
#include "layers_intern.h"

int _MoveLayerBehind(struct Layer *l,
                     struct Layer *lfront,
                     LIBBASETYPEPTR LayersBase)
{
  struct Layer * lbackold, *_l, *first;
  struct Region * hide = NewRegion(), show;
  InitRegion(&show);

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
    if (l->parent == _l->parent && IS_VISIBLE(_l))
      OrRegionRegion(_l->visibleshape, hide);

    if (_l == lfront)
      break;
      
    _l = _l->back;
  }

  SetRegion(first->VisibleRegion, &show);

  /*
   * First back up the family ... this is like moving them behind the
   * other layers.
   */
  
  _l = first;
  while (1)
  {
    if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->visibleshape->bounds, &hide->bounds))
      _BackupPartsOfLayer(_l, hide, 0, FALSE, LayersBase);
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
    if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->visibleshape->bounds, &show.bounds))
    {
      ClearRegion(_l->VisibleRegion);
      _ShowPartsOfLayer(_l, &show, LayersBase);
    }
    else
      SetRegion(&show, _l->VisibleRegion);
      
    if (_l == lfront)
      break;

    if (IS_VISIBLE(_l))
      ClearRegionRegion(_l->visibleshape, &show);

    _l = _l->back; 
  }

  ClearRegion(&show);

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
                      LIBBASETYPEPTR LayersBase)
{
  struct Layer * lfront, * first, * _l;
  struct Region r, * backupr = NULL;
  int backupr_allocated = FALSE;

  InitRegion(&r);
    
  first = GetFirstFamilyMember(l);

  lfront = lbehind->front;

  /*
   * Unlink the family of layers from its old place.
   */
  first->front->back = l->back;
  l->back->front = first->front;

  /*
   * I need exactly that layers visible region later on.
   */

  SetRegion(lbehind->VisibleRegion,&r);
    
  /*
   * if the layer l is visible then I will have to backup
   * its visible region in all layers behind it. Otherwise
   * I will have to collect the visible shape of its whole
   * family.
   */
  if (IS_VISIBLE(l))
    backupr = l->visibleshape;
  else
  {
    _l = l;
    while (1)
    {
      if (IS_VISIBLE(_l))
      {
        if (NULL == backupr)
        {
          backupr = NewRegion();
          backupr_allocated = TRUE;
        }
        if (backupr)
          OrRegionRegion(_l->visibleshape, backupr);
      }
      if (_l == first)
        break;
        
      _l = _l->front;
    }    
  }

  _l = lbehind;


  if (backupr)
  {
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
      if (IS_VISIBLE(_l) && DO_OVERLAP(&backupr->bounds, &_l->visibleshape->bounds))
        _BackupPartsOfLayer(_l, backupr, 0, FALSE, LayersBase);
      else
        ClearRegionRegion(backupr, _l->VisibleRegion);

      _l = _l->back;
    }
    while (_l != l->back /* this is l->back now since the family has
                          been unlinked already!*/ );

    if (TRUE == backupr_allocated)
      DisposeRegion(backupr);

    /*
     * Now I must make the layer family of l visible 
     * (lfirst to and including l)
     */
    _l = first;
    
    while (1)
    {
      if (IS_VISIBLE(_l) && DO_OVERLAP(&_l->visibleshape->bounds, &r.bounds))
      {
        ClearRegion(_l->VisibleRegion);
        _ShowPartsOfLayer(_l, &r, LayersBase);
      }
      else
        SetRegion(&r, _l->VisibleRegion);      

      if (_l == l)
        break;

      if (IS_VISIBLE(_l))
        ClearRegionRegion(_l->visibleshape, &r);

      _l = _l->back;
    }
  }
  ClearRegion(&r);

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

