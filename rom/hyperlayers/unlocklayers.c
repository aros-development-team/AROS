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

	AROS_LH1(void, UnlockLayers,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 19, Layers)

/*  FUNCTION
        First unlocks all layers found in the list, then
        unlocks the Layer_Info itself.

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

  struct Layer * l = li->top_layer;

  D(bug("UnlockLayers(li @ $%lx)\n", li));

  while (NULL != l)
  {
    UnlockLayer(l);
    l = l->back;  
  } 

  UnlockLayerInfo(li);   

  AROS_LIBFUNC_EXIT
} /* UnlockLayers */
