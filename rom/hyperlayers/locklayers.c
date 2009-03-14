/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

	AROS_LH1(void, LockLayers,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 18, Layers)

/*  FUNCTION

        First locks the Layer_Info then all the layers that are
        found in the list of layers.

    INPUTS
        li - pointer to a Layer_Info structure

    RESULT

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
 
  struct Layer * l;

  D(bug("LockLayers(li @ $%lx)\n", li));

  LockLayerInfo(li);

  l = li->top_layer;
  
  while (NULL != l)
  {
    LockLayer(0, l);
    l = l->back;
  }
    

  AROS_LIBFUNC_EXIT
} /* LockLayers */

