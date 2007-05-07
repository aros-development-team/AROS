/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH2(LONG, MoveLayerInFrontOf,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer_to_move, A0),
	AROS_LHA(struct Layer *, other_layer, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 28, Layers)

/*  FUNCTION
        Moves layer directly in front of another layer. Other layers
        might become visible. You cannot move a backdrop layer in front
        of a non-backdrop layer. You can also not move a layer in front
        of a layer with different relationship to the root layer. Boot
        have to be children of grandchildren or grandgrandchildren etc.
        of the root layer. The root layer is not visible to you and
        should never be accessed.
        If parts of a simple layer become visible these areas are added
        to the damage list.

    INPUTS
        layer_to_move - pointer to layer that is to be moved
        other_layer   - pointer to other layer that will be behind the
                        layer_to_move.

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved. (probably out of memory)

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

  struct Layer * first, *_l;
  int toback = TRUE;
  LONG ret;

  if (layer_to_move->parent  != other_layer->parent ||
      layer_to_move->priority < other_layer->priority)
    return FALSE;

  LockLayers(layer_to_move->LayerInfo);

  first = GetFirstFamilyMember(layer_to_move);
  
  _l = layer_to_move->parent->front;
  while (1)
  {
    /*
     * If I run into the other layer before I find l
     * then I know I have to move it to the back.
     */ 
    if (_l == other_layer)
      break;
      
    if (_l == layer_to_move)
    {
      toback = FALSE;
      break;
    }
    _l = _l->front;
    if (NULL == _l)
    {
      UnlockLayers(layer_to_move->LayerInfo);
      return FALSE;
    }
  }

  _l = GetFirstFamilyMember(other_layer);
  
  if (TRUE == toback)
  {
    /*
     * If the topmost child of the other layer is
     * behind my layer I don't have to do anything.
     */
    if (layer_to_move->back == _l)
    {
      UnlockLayers(layer_to_move->LayerInfo);
      return TRUE;
    }
    ret = _MoveLayerBehind(layer_to_move, _l, LayersBase);
  }
  else
  {
    ret = _MoveLayerToFront(layer_to_move, _l, LayersBase);
  }
  
  UnlockLayers(layer_to_move->LayerInfo);
  return ret;
  
  AROS_LIBFUNC_EXIT
} /* MoveLayerInFrontOf */
