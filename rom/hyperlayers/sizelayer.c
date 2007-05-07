/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>


/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH4(LONG, SizeLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),
	AROS_LHA(LONG          , dw   , D0),
	AROS_LHA(LONG          , dh   , D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 11, Layers)

/*  FUNCTION
        Resizes the given layer by adding dw to its width and dh
        to its height.
        If parts of simple layers become visible those parts are
        added to the damage list and a refresh is triggered for
        those layers. 
        If the new layer is bigger than before the additional parts
        are added to a damage list if the layer is a non-super-
        bitmap layer. Refresh is also triggered for this layer.

    INPUTS
        l    - pointer to layer to be resized
        dw   - delta to be added to the width
        dh   - delta to be added to the height

    RESULT
        TRUE  - layer could be resized
        FALSE - error occurred (out of memory)

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

  /* 
    In this case I will create a layer behind the current layer and
    I will try to clear as little of the layer that was above it as
    possible.
  */    
  return MoveSizeLayer(l,0,0,dw,dh);

   
  AROS_LIBFUNC_EXIT

} /* SizeLayer */
