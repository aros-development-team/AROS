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
#include "basicfuncs.h"
#include "layers_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(LONG, BehindLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 9, Layers)

/*  FUNCTION
       If the layer is a backdrop layer it will be moved to the most
       behind position. If it is a non-backdrop layer it will be moved
       in front of the first backdrop layer.
       The areas of simple layers, that become visible by moving this
       layer, are added to the damagelist and the LAYERREFRESH flag
       is set.  

    INPUTS
       dummy - nothing
       L     - pointer to layer 

    RESULT
       TRUE  - layer was successfully moved
       FALSE - layer could not be moved (probably out of memory)
  
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

  struct Layer * _l;
  LONG ret;

//kprintf("\t\t%s called!\n",__FUNCTION__);
  LockLayers(l->LayerInfo);

  /*
   * The layer in front of the family is the layer in
   * front of the parent of my layer
   */
  if (!l->parent)
  {
    UnlockLayers(l->LayerInfo);
    return FALSE;
  }  
  _l = l->parent->front;

  while (_l->priority < l->priority)
    _l = _l->front;
  
  if (_l == l)
  {
    UnlockLayers(l->LayerInfo);
    return TRUE;
  }
  ret = _MoveLayerBehind(l,_l,LayersBase);

  UnlockLayers(l->LayerInfo);

  return ret;

  AROS_LIBFUNC_EXIT
} /* BehindLayer */
