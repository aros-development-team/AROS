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
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer * first, * _l, * linfront = NULL;
  int found = FALSE;
  struct Region * r = NewRegion();
  struct Region * r2 = NewRegion();

kprintf("\t\t%s called!\n",__FUNCTION__);
  /*
   * Move the layer behind the layer with the same
   * priority.
   * Keep its children in front of itself.
   */
  first = _FindFirstFamilyMember(l);

  /*
   * Search for the new place
   * search all layers that have the same priority.
   * If I find another layer with the same nesting
   * as the one to be moved I have a place to move.
   */
  _l = l->back;
  
  
  if (_l->priority == l->priority)
  {
    while (1)
    {
      if (_l->nesting == l->nesting)
        found = TRUE;
      if (NULL == _l->back || _l->back->priority != l->priority)
        break;
      _l = _l->back;
    }
  }
  
  if (FALSE == found)
    return TRUE;

  if (_l->nesting != l->nesting)
  {
    kprintf("%s: Something is wrong with the order of the layers!\n",__FUNCTION__);
    return FALSE;
  }


  /*
   * Take the family out of its old place.
   */
  if (NULL != first->front)
    first->front->back = l->back;
  else
    l->LayerInfo->top_layer = l->back;
    
  l->back->front = first->front;

  /*
   * Remember the layer that will come in front of l
   */
  linfront = _l;

  OrRegionRegion(first->VisibleRegion, r2);

  OrRegionRegion(first->VisibleRegion, r);
  _l = l->back;
  while (1)
  {
kprintf("Taking outr!\n");
    ClearRegionRegion(_l->shape, r);
    if (_l == linfront)
      break;
    
    _l = _l->back;
  }
  
  /*
   * First back up the family ...
   */
  
  _l = first;
  while (1)
  {
    ClearRegion(_l->VisibleRegion);
    _ShowPartsOfLayer(_l, r);
    if (_l == l)
      break;
  
     ClearRegionRegion(_l->shape, r);
    
    _l = _l -> back;
  }
  DisposeRegion(r);

  
  /*
   * Show the layers that appear now.
   * Start with the layer that is behind the layer l.
   */
  _l = l->back;

  while (!IS_EMPTYREGION(r))
  {
    _ShowPartsOfLayer(_l, r2);
    ClearRegionRegion(_l->shape, r2);
    if (_l == linfront)
      break;
    _l = _l->back; 
  }

  /*
   * Link the familiy into its new place.
   */
  first->front = linfront;
  linfront->back->front = l;
  l->back = linfront->back;
  linfront->back = first;

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BehindLayer */
