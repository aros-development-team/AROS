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
        CreateUpfrontLayer() CreateUpfrontHookLayer() BehindLayer()
        CreateBehindLayer() CreateBehindHookLayer()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer * first, * _l, * lbehind = NULL;
  int found = FALSE;
  struct Region * r = NewRegion();

kprintf("\t\t%s called!\n",__FUNCTION__);
  LockLayers(l->LayerInfo);

  /*
   * Move the layer in front of that layer with the same
   * priority.
   * Also keep its children in front of itself.
   */
  first = _FindFirstFamilyMember(l);

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

  /*
   * Remember the layer that will come behind l
   */
  lbehind = _l;

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
   * Now I have to move the layer family in front of layer _l.
   * Nothing changes for the layers in front of layer _l, but
   * the layers behind (including _l) I must back up some of 
   * their parts that the new one might be hiding.
   * Once I step on the layer that was behind the layer l
   * I can stop because those layers are already hidden well
   * enough.
   */
  do
  {
kprintf("\t\t%s: backing up parts of layer %p!\n",__FUNCTION__,_l);
    _BackupPartsOfLayer(_l, 
                        l->shape);
    _l = _l->back;
  }  
  while (_l != l->back);

  /*
   * Now I must make the layer family of l visible and l itself
   */
  _l = first;
  
    
  while (1)
  {
kprintf("\t\t%s: Showing parts of layer %p\n",__FUNCTION__,_l);
    _ShowPartsOfLayer(_l, r);
    ClearRegionRegion(_l->shape, r);

    if (_l == l)
      break;

    _l = _l->back;
  }

  DisposeRegion(r);

  /*
   * Link the family into its new place.
   * First the frontmost kid and then l.
   */
  if (NULL != lbehind->front)
    lbehind->front->back = first;
  else
    l->LayerInfo->top_layer = first;
    
  first->front = lbehind->front;

  l->back = lbehind;
  lbehind->front = l;

  UnlockLayers(l->LayerInfo);

  return TRUE;
  
  AROS_LIBFUNC_EXIT
} /* UpfrontLayer */
