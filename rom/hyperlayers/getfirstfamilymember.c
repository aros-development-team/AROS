/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH1(struct Layer *, GetFirstFamilyMember,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 38, Layers)

/*  FUNCTION
        Gets the first member of a layer family. If the layer
        has no children at all this function returns the
        pointer to the same layer as given.
        If the layer has children which again have children etc.
        this function returns the frontmost child.

    INPUTS
        l - pointer to a layer structure.                  
        
    RESULT
        Pointer to a layer structure that represents the front
        most child of the given layer or the layer itself if it
        has no children.
        
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

  struct Layer * lastgood = l, *_l = l->front;
  
  while ((NULL != _l) && (_l->nesting > l->nesting))
  {
    lastgood = _l;
    _l = _l->front;
  }
  return lastgood;

  AROS_LIBFUNC_EXIT
} /* GetFirstFamilyMember */
