/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Implementation of MoveLayer()
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH4(LONG, MoveLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),
	AROS_LHA(LONG          , dx   , D0),
	AROS_LHA(LONG          , dy   , D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 10, Layers)

/*  FUNCTION
        Move the layer to a specified position in the bitmap.
        Parts of simple layers that become visible are added to
        the damage list and a refresh is triggered.

    INPUTS
        dummy - unused
        l     - pointer to layer to be moved
        dx    - delta to add to current x position
        dy    - delta to add to current y position

    RESULT
        result - TRUE everyting went alright
                 FALSE an error occurred (out of memory)

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

  return MoveSizeLayer(l,dx,dy,0,0);

  AROS_LIBFUNC_EXIT
} /* MoveLayer */
