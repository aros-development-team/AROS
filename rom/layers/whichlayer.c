/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH3(struct Layer *, WhichLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(LONG               , x,  D0),
	AROS_LHA(LONG               , y,  D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 22, Layers)

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
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Layer *l;

  D(bug("WhichLayer(li @ $%lx, x %ld, y %ld)\n", li, x, y));

  for(l = li->top_layer; l != NULL; l = l->back)
    if(x >= l->bounds.MinX && x <= l->bounds.MaxX &&
       y >= l->bounds.MinY && y <= l->bounds.MaxY)
       return l;

  return NULL;

  AROS_LIBFUNC_EXIT
} /* WhichLayer */
