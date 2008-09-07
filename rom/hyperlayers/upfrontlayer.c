/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include "layers_intern.h"


/*****************************************************************************

    NAME */

	AROS_LH2(LONG, UpfrontLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 8, Layers)

/*  FUNCTION
        Brings a layer to the front. If this layer is a backdrop layer
        it is brought in front of all backdrop layers and behind the
        last non-backdrop layer. By clearing the BACKDROP flag of a layer
        a backdrop layer can be brought in front of all other layers.
        Parts of a simple layer that become visible are added to the 
        damage list and the REFRESH flag is set.

    INPUTS
        dummy - unused
        L     - pointer to layer

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved (probably out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateUpfrontLayer(), CreateUpfrontHookLayer(), BehindLayer()
        CreateBehindLayer(), CreateBehindHookLayer()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct Layer * first, * _l;
  int found = FALSE;
  LONG ret;

//kprintf("\t\t%s called!\n",__FUNCTION__);
  LockLayers(l->LayerInfo);

  /*
   * Move the layer in front of that layer with the same
   * priority.
   * Also keep its children in front of itself.
   */
  first = GetFirstFamilyMember(l);

  /*
   * If there is nobody in front of the first family member
   * I don't have to do anything.
   * first can also be l.
   */
  if (NULL == first->front)
  {
    UnlockLayers(l->LayerInfo);
    return TRUE;
  }
  /*
   * Search for the new place
   * search all layers that have the same priority.
   * If I find another layer with the same nesting
   * as the one to be moved I have a place to move.
   * Stop at the frontmost layer
   */
  _l = first->front;
  
  
  if (_l -> priority == l->priority)
  {
    while (1)
    {
      if (_l->nesting == l->nesting)
        found = TRUE;
      if (NULL == _l->front || _l->front->priority != l->priority)
        break;
      _l = _l->front;
    }
  }
  
  if (FALSE == found)
  {
    UnlockLayers(l->LayerInfo);
    return TRUE;
  }

  ret = _MoveLayerToFront(l,_l, LayersBase);


  /*
   * Unlock all locked layers.
   */
  UnlockLayers(l->LayerInfo);

  return TRUE;
  
  AROS_LIBFUNC_EXIT
} /* UpfrontLayer */
