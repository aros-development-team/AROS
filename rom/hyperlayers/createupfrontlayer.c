/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <aros/libcall.h>
#include <graphics/gfx.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/hooks.h>
#include <proto/layers.h>
#include "layers_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH8(struct Layer *, CreateUpfrontLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
	AROS_LHA(LONG               , x0, D0),
	AROS_LHA(LONG               , y0, D1),
	AROS_LHA(LONG               , x1, D2),
	AROS_LHA(LONG               , y1, D3),
	AROS_LHA(LONG               , flags, D4),
	AROS_LHA(struct BitMap     *, bm2, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 6, Layers)

/*  FUNCTION

    INPUTS

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

  D(bug("CreateUpfrontLayer(li@$lx, bm@$lx, x0 %ld, y0 %ld, x1 %ld, y1 %ld, flags %ld, bm2@$lx)\n",
     li, bm, x0, y0, x1, y1, flags, bm2));

  return CreateUpfrontHookLayer(li, bm, x0, y0, x1, y1, flags, NULL, bm2);

  AROS_LIBFUNC_EXIT
} /* CreateUpfrontLayer */
